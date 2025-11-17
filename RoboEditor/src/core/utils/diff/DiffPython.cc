#include "DiffPython.h"

#include <algorithm>
#include <cctype>
#include <json/json.h>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
//////////////////////////////////////////////////////////////////////////////
//                                  퍼블릭                                  //
//////////////////////////////////////////////////////////////////////////////
Json::Value DiffPython::runFromText(const std::string &contentA,
                                    const std::string &contentB,
                                    const std::string &nameA,
                                    const std::string &nameB)
{
    // 1) 정규화 (라인 분할 + 공백 축약 + rank 산출)
    std::vector<NormalizedLine> aNorm = normalizeAll(contentA);
    std::vector<NormalizedLine> bNorm = normalizeAll(contentB);

    // 2) LCS 기반 diff 생성
    std::vector<Op>   ops   = compute(aNorm, bNorm);
    std::vector<Diff> diffs = foldOpsToDiffs(ops, aNorm, bNorm, contentA, contentB);

    // 3) changes 배열 + 통계 집계
    Json::Value changes(Json::arrayValue);
    DiffStats   stats{};
    buildChangesJson(diffs, changes, stats);

    // 4) 루트 JSON 조립
    Json::Value root(Json::objectValue);

    Json::Value base(Json::objectValue);
    base["name"] = nameA;
    root["base"] = std::move(base);

    Json::Value compare(Json::objectValue);
    compare["name"] = nameB;
    root["compare"] = std::move(compare);

    Json::Value        jstats(Json::objectValue);
    const Json::UInt64 total =
            static_cast<Json::UInt64>(stats.added + stats.deleted + stats.modified);
    jstats["added"]        = static_cast<Json::UInt64>(stats.added);
    jstats["deleted"]      = static_cast<Json::UInt64>(stats.deleted);
    jstats["modified"]     = static_cast<Json::UInt64>(stats.modified);
    jstats["totalChanges"] = total;
    root["statistics"]     = std::move(jstats);

    root["changes"] = std::move(changes);

    return root;
}

std::vector<std::pair<std::string, int>> DiffPython::normalizeBodies(const std::string &content)
{
    std::vector<std::pair<std::string, int>> out;
    auto                                     lines = normalizeAll(content);
    out.reserve(lines.size());

    for (auto &l : lines)
        out.emplace_back(std::move(l.body), l.rank);

    return out;
}

//////////////////////////////////////////////////////////////////////////////
//                                  프라이빗                                 //
//////////////////////////////////////////////////////////////////////////////

// 공백 문자 판정
static inline bool isWs(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\f' || c == '\v';
}
// 삼중따옴표 판정
static inline bool startsWith3(const std::string &s, size_t pos, size_t endExcl, char q)
{
    return pos + 2 < endExcl && s[pos] == q && s[pos + 1] == q && s[pos + 2] == q;
}

// 한 줄[start, endExcl) 정규화
// 양끝 공백 제거 + 내부 연속 공백 축약 + 주석 처리 + rank 처리
// inSpace = 공백
// inQuote = ""
// prevEscape = \
// quoteChar = " or ', default = \0
// rank = 들여쓰기 (-1은 공백 or 주석)
NormalizedLine DiffPython::normalizeOne(const std::string &content,
                                        size_t             start,
                                        size_t             endExcl,
                                        std::vector<int>  &deep,
                                        bool              &inTriple,
                                        char              &tripleQuoteChar)
{
    std::string body;
    bool        inSpace    = false;
    bool        inQuote    = false;
    bool        prevEscape = false;
    char        quoteChar  = '\0';
    int         rank       = -1;
    size_t      i = start, j = endExcl;

    while (i < j && isWs(static_cast<unsigned char>(content[i])))
        ++i;
    while (j > i && isWs(static_cast<unsigned char>(content[j - 1])))
        --j;

    body.reserve(j - i);

    for (size_t k = i; k < j; ++k) {
        unsigned char c = static_cast<unsigned char>(content[k]);

        if (inTriple) {
            if (startsWith3(content, k, j, tripleQuoteChar)) {
                inTriple        = false;
                tripleQuoteChar = '\0';
                k += 2;
            }
            continue;
        }

        if ((c == '"' || c == '\'') && startsWith3(content, k, j, c)) {
            inSpace         = false;
            inTriple        = true;
            tripleQuoteChar = static_cast<char>(c);
            k += 2;
            continue;
        }

        if (inQuote) {
            if (prevEscape)
                prevEscape = false;
            else if (c == '\\')
                prevEscape = true;
            else if (static_cast<char>(c) == quoteChar) {
                inQuote   = false;
                quoteChar = '\0';
            }
            body.push_back(static_cast<char>(c));
            continue;
        }

        if (c == '"' || c == '\'') {
            inSpace    = false;
            inQuote    = true;
            prevEscape = false;
            quoteChar  = static_cast<char>(c);
            body.push_back(static_cast<char>(c));
            continue;
        }

        if (c == '#') {
            while (!body.empty() && body.back() == ' ')
                body.pop_back();
            break;
        }

        if (isWs(c)) {
            if (!inSpace) {
                inSpace = true;
                body.push_back(' ');
            }
        } else {
            inSpace = false;
            body.push_back(static_cast<char>(c));
        }
    }

    if (!body.empty()) {
        const int lead = static_cast<int>(i - start);
        while (!deep.empty() && lead < deep.back()) {
            deep.pop_back();
        }
        if (deep.empty() || lead > deep.back()) {
            deep.push_back(lead);
        }
        rank = static_cast<int>(deep.size()) - 1;
    }

    return NormalizedLine{start, endExcl - start, rank, std::move(body)};
}

// 텍스트 파일을 라인 단위로 정규화
// 라인으로 분해, 주석처리, 개행 처리 (\r, \n)
// inTriple = 삼중따옴표 (주석)
// tripleQuoteChar = 삼중따옴표 문자 (' or ")
// spans = 원본 라인 수 (128 예상)
// deep = rank 깊이 (64 예상)
std::vector<NormalizedLine> DiffPython::normalizeAll(const std::string &content)
{
    std::vector<NormalizedLine> out;
    std::vector<size_t>         spans;
    std::vector<int>            deep;
    bool                        inTriple        = false;
    char                        tripleQuoteChar = '\0';

    spans.reserve(std::max<size_t>(128, content.size() / 48 + 4));
    deep.reserve(64);

    spans.push_back(0);
    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\n') {
            spans.push_back(i + 1);
        }
    }
    if (spans.back() != content.size()) {
        spans.push_back(content.size());
    }

    out.reserve(spans.size() - 1);

    for (size_t i = 1; i < spans.size(); ++i) {
        size_t start   = spans[i - 1];
        size_t endExcl = spans[i];

        while (endExcl > start) {
            char ch = content[endExcl - 1];
            if (ch == '\n' || ch == '\r')
                --endExcl;
            else
                break;
        }
        out.emplace_back(normalizeOne(content, start, endExcl, deep, inTriple, tripleQuoteChar));
    }

    return out;
}

// 정규화된 라인을 비교해 라인 간 관계를 표현한 목록 반환
// 정규화 정보 정수화
// → 트림으로 비교 구간 단축 + 공통 접두 ops에 추가
// → 앵커와 연산
// → 공통 접미 ops에 추가
// → 원본 문자열 라인으로 복원
// ops = A와 B의 add, same, del 정보를 담은 벡터
// Ai, Bi = 문자열을 정수화한 벡터
// origA, origB = 원본 문자열 라인을 저장한 벡터
// a0, b0 = A와 B의 비교 시작 인덱스(라인)
// a1, b1 = A와 B의 비교 끝 인덱스(라인)
// vocabCap = A와 B에서 정규화된 라인의 총 개수 + 2(여유), 앵커에서 버킷 만들 시 할당 되는 값
std::vector<Op> DiffPython::compute(const std::vector<NormalizedLine> &aNorm,
                                    const std::vector<NormalizedLine> &bNorm)
{
    std::vector<Op>  ops;
    std::vector<int> Ai, Bi;
    std::vector<int> origA, origB;
    int              a0, a1, b0, b1;

    Ai.reserve(aNorm.size());
    Bi.reserve(bNorm.size());
    origA.reserve(aNorm.size());
    origB.reserve(bNorm.size());

    const int vocabCap = buildInternAndIdsRankBuckets(aNorm, bNorm, Ai, Bi, origA, origB);

    ops.reserve(Ai.size() + Bi.size());

    trimCommon(Ai, Bi, a0, a1, b0, b1, ops);

    if (a0 < a1 || b0 < b1) {
        solveWithAnchors(Ai, Bi, aNorm, bNorm, origA, origB, vocabCap, a0, a1, b0, b1, ops);
    }

    appendSuffixSame(Ai, Bi, a1, b1, ops);

    for (Op &op : ops) {
        if (op.ai >= 0)
            op.ai = origA[op.ai];
        if (op.bj >= 0)
            op.bj = origB[op.bj];
    }

    return ops;
}

// body와 rank로 정수화 + 원본 문자열 라인 저장
// nextId = 정수화 된 종류수 (전역 유일 보장)
int DiffPython::buildInternAndIdsRankBuckets(const std::vector<NormalizedLine> &aNorm,
                                             const std::vector<NormalizedLine> &bNorm,
                                             /*out*/ std::vector<int>          &Ai,
                                             /*out*/ std::vector<int>          &Bi,
                                             /*out*/ std::vector<int>          &origA,
                                             /*out*/ std::vector<int>          &origB)
{
    struct SvHash
    {
        size_t operator()(std::string_view v) const noexcept
        {
            return std::hash<std::string_view>{}(v);
        }
    };
    struct SvEq
    {
        bool operator()(std::string_view a, std::string_view b) const noexcept
        {
            return a == b;
        }
    };

    std::unordered_map<std::string_view, int, SvHash, SvEq> dict;
    dict.reserve((aNorm.size() + bNorm.size()) * 2 + 64);

    int nextId = 0;

    auto internSide = [&](const auto &src, std::vector<int> &I, std::vector<int> &orig) {
        for (size_t i = 0; i < src.size(); ++i) {
            if (src[i].rank < 0 || src[i].body.empty())
                continue;
            std::string_view sv(src[i].body);
            auto             it = dict.find(sv);
            if (it == dict.end())
                it = dict.emplace(sv, ++nextId).first;
            I.push_back(it->second);

            orig.push_back(static_cast<int>(i));
        }
    };

    internSide(aNorm, Ai, origA);
    internSide(bNorm, Bi, origB);

    return nextId + 2;
}

// 공통 접두/접미 트림 + 접두 SAME push
// a0, b0 전과 a1, b1 후는 비교 x
void DiffPython::trimCommon(const std::vector<int>    &Ai,
                            const std::vector<int>    &Bi,
                            /*out*/ int               &a0,
                            /*out*/ int               &a1,
                            /*out*/ int               &b0,
                            /*out*/ int               &b1,
                            /*inout*/ std::vector<Op> &ops)
{
    const int n = static_cast<int>(Ai.size());
    const int m = static_cast<int>(Bi.size());
    a0          = 0;
    b0          = 0;
    while (a0 < n && b0 < m && Ai[a0] == Bi[b0]) {
        ++a0;
        ++b0;
    }
    a1 = n;
    b1 = m;
    while (a1 > a0 && b1 > b0 && Ai[a1 - 1] == Bi[b1 - 1]) {
        --a1;
        --b1;
    }

    for (int i = 0; i < a0; ++i) {
        ops.emplace_back(Op::SAME, i, i);
    }
}

// 앵커를 찾아 구간을 분할 및 해결
// 유니크 라인 기반 앵커 / 히스토그램 앵커
// → 앵커 순회하며 구간 myers 계산
// → 앵커 이후 myers 계산
// 유니크 라인의 효율성이 떨어진다면 히스토그램으로 대체
// 가드에 걸리면 해당 구간부터 myers 계산
// MOVE_GUARD_ABS = 절대 상한
// MOVE_GUARD_RATIO = 거부 기준(% 초과 시)
// DIAG_GUARD = 대각선 기준 거부
// pa, pb = 해결된 라인 A, B
// qa, qb = 앵커가 집은 라인 A, B
// preA, preB = 해결된 라인 ~ 앵커가 집은 라인 구간 A, B
// segA, segB = 앵커가 집은 라인 ~ 비교가 끝나는 라인 구간 A, B
void DiffPython::solveWithAnchors(const std::vector<int>            &Ai,
                                  const std::vector<int>            &Bi,
                                  const std::vector<NormalizedLine> &aNorm,
                                  const std::vector<NormalizedLine> &bNorm,
                                  const std::vector<int>            &origA,
                                  const std::vector<int>            &origB,
                                  int                                vocabCap,
                                  int                                a0,
                                  int                                a1,
                                  int                                b0,
                                  int                                b1,
                                  /*inout*/ std::vector<Op>         &ops)
{
    const int    MOVE_GUARD_ABS   = 8;
    const double MOVE_GUARD_RATIO = 0.05;
    const int    DIAG_GUARD       = 64;
    int          pa = a0, pb = b0;

    std::vector<std::pair<int, int>> anchors = buildAnchorsUnique(Ai, Bi, a0, a1, b0, b1, vocabCap);
    if (anchors.size() < 2) {
        std::vector<std::pair<int, int>> hist =
                buildAnchorsHistogram(aNorm, bNorm, Ai, Bi, origA, origB, a0, a1, b0, b1, vocabCap);
        if (hist.size() > anchors.size())
            anchors = std::move(hist);
    }

    for (const std::pair<int, int> &anc : anchors) {
        int qa = anc.first, qb = anc.second;
        if (qa < pa || qb < pb || qa < a0 || qa >= a1 || qb < b0 || qb >= b1)
            continue;

        int preA = qa - pa;
        int preB = qb - pb;
        int segA = a1 - pa;
        int segB = b1 - pb;

        int absCost = (qa - qb) > 0 ? (qa - qb) : (qb - qa);
        int preCost = preA + preB;
        int segMin  = std::min(segA, segB);

        bool tooFarFromDiag = absCost > DIAG_GUARD;
        bool tooLargeAbs    = preCost > MOVE_GUARD_ABS;
        bool tooLargeRatio  = segMin > 0 && preCost > static_cast<int>(MOVE_GUARD_RATIO * segMin);

        if (tooFarFromDiag || tooLargeAbs || tooLargeRatio) {
            dcMyers(Ai, Bi, pa, a1, pb, b1, ops);
            return;
        }

        if (pa < qa || pb < qb)
            dcMyers(Ai, Bi, pa, qa, pb, qb, ops);
        ops.emplace_back(Op::SAME, qa, qb);
        pa = qa + 1;
        pb = qb + 1;
    }
    if (pa < a1 || pb < b1)
        dcMyers(Ai, Bi, pa, a1, pb, b1, ops);
}

// 유니크 라인(패이션스) 기반 앵커
// 정수화된 라인의 빈도 계산
// → B에서 빈도가 1인 위치 확보
// → A에서 빈도가 1인 라인이 B에서도 빈도가 1이라면 두 위치 저장
// → B를 기준으로 LIS 계산
// anchors = 최종 앵커의 위치
// fA, fB = A, B에서 정수화된 라인 개수
// posB = B에서 유니크 라인의 위치 기록
std::vector<std::pair<int, int>> DiffPython::buildAnchorsUnique(const std::vector<int> &Ai,
                                                                const std::vector<int> &Bi,
                                                                int                     x0,
                                                                int                     x1,
                                                                int                     y0,
                                                                int                     y1,
                                                                int                     vocabCap)
{
    std::vector<std::pair<int, int>> anchors;
    std::vector<int>                 fA(vocabCap, 0), fB(vocabCap, 0);
    std::vector<int>                 posB(vocabCap, -1);
    std::vector<std::pair<int, int>> pairs;

    for (int i = x0; i < x1; ++i)
        ++fA[Ai[i]];
    for (int j = y0; j < y1; ++j)
        ++fB[Bi[j]];

    for (int j = y0; j < y1; ++j) {
        const int id = Bi[j];
        if (fB[id] == 1)
            posB[id] = j;
    }

    pairs.reserve(std::min(x1 - x0, y1 - y0));
    for (int i = x0; i < x1; ++i) {
        const int id = Ai[i];
        if (fA[id] == 1) {
            const int j = posB[id];
            if (j != -1)
                pairs.emplace_back(i, j);
        }
    }
    if (pairs.empty())
        return {};

    anchors = extractAnchorsLIS(pairs);

    return anchors;
}

// buildAnchorsHistogram 헬퍼함수
// 각 라인의 단어 분해
static inline std::vector<std::string> tokenizeWordsSv(std::string_view s)
{
    std::vector<std::string> out;
    out.reserve(16);
    std::string cur;
    for (unsigned char ch : s) {
        if (std::isalnum(ch) || ch == '_')
            cur.push_back((char)ch);
        else if (!cur.empty()) {
            out.push_back(cur);
            cur.clear();
        }
    }
    if (!cur.empty())
        out.push_back(cur);
    return out;
}

// 히스토그램 기반 앵커
// freq에 단어 빈도 누적 (countSide)
// → 단어의 빈도 수에 따른 점수 계산 (score)
// → 상위 점수 라인 저장 (keepTopPct)
// → 단어가 많이 겹치는 라인을 저장
// → A를 정렬한 후 B를 기준으로 LIS 계산
// freq = 단어 빈도 맵
// ca, cb = 점수가 높은 라인
// buckA, buckB = 같은 단어를 가진 라인들 저장
std::vector<std::pair<int, int>>
DiffPython::buildAnchorsHistogram(const std::vector<NormalizedLine> &aNorm,
                                  const std::vector<NormalizedLine> &bNorm,
                                  const std::vector<int>            &Ai,
                                  const std::vector<int>            &Bi,
                                  const std::vector<int>            &origA,
                                  const std::vector<int>            &origB,
                                  int                                x0,
                                  int                                x1,
                                  int                                y0,
                                  int                                y1,
                                  int                                vocabCap)
{
    struct Cand
    {
        int    pos;
        double sc;
    };
    std::vector<std::pair<int, int>>     anchors;
    std::unordered_map<std::string, int> freq;
    std::vector<Cand>                    ca;
    std::vector<Cand>                    cb;
    std::vector<std::vector<int>>        buckA;
    std::vector<std::vector<int>>        buckB;
    std::vector<std::pair<int, int>>     pairs;

    freq.reserve((size_t)((x1 - x0) + (y1 - y0)) * 6 + 32);

    auto countSide = [&](int l0, int l1, const auto &norm, const auto &orig) {
        for (int i = l0; i < l1; ++i) {
            int                idx = orig[i];
            const std::string &s   = norm[idx].body;
            if (s.empty())
                continue;
            for (const std::string &t : tokenizeWordsSv(std::string_view(s))) {
                if (t.size() > 1)
                    ++freq[t];
            }
        }
    };

    auto score = [&](const auto &norm, int idx) {
        double             sc = 0.0;
        const std::string &s  = norm[idx].body;
        if (s.empty())
            return 0.0;
        for (const std::string &t : tokenizeWordsSv(std::string_view(s))) {
            if (t.size() <= 1)
                continue;
            auto it = freq.find(t);
            if (it != freq.end() && it->second > 0)
                sc += 1.0 / (double)it->second;
        }
        return sc;
    };

    auto keepTopPct = [](std::vector<Cand> &v, double pct) {
        if (v.empty())
            return;
        size_t k = std::max<size_t>(1, (size_t)(v.size() * pct));
        std::nth_element(v.begin(), v.begin() + k - 1, v.end(), [](const Cand &a, const Cand &b) {
            return a.sc > b.sc;
        });
        v.resize(k);
        std::sort(v.begin(), v.end(), [](const Cand &a, const Cand &b) { return a.pos < b.pos; });
    };

    countSide(x0, x1, aNorm, origA);
    countSide(y0, y1, bNorm, origB);

    ca.reserve((size_t)(x1 - x0));
    cb.reserve((size_t)(y1 - y0));

    for (int k = x0; k < x1; ++k) {
        int    idxA = origA[k];
        double sc   = score(aNorm, idxA);
        if (sc > 0)
            ca.push_back({k, sc});
    }
    for (int k = y0; k < y1; ++k) {
        int    idxB = origB[k];
        double sc   = score(bNorm, idxB);
        if (sc > 0)
            cb.push_back({k, sc});
    }
    if (ca.empty() || cb.empty())
        return {};

    keepTopPct(ca, 0.25);
    keepTopPct(cb, 0.25);

    buckA.resize(vocabCap);
    buckB.resize(vocabCap);

    for (const Cand &c : ca)
        buckA[Ai[c.pos]].push_back(c.pos);
    for (const Cand &c : cb)
        buckB[Bi[c.pos]].push_back(c.pos);

    pairs.reserve(std::min(ca.size(), cb.size()));

    for (int id = 0; id < vocabCap; ++id) {
        auto &va = buckA[id];
        auto &vb = buckB[id];
        if (va.empty() || vb.empty())
            continue;
        std::sort(va.begin(), va.end());
        std::sort(vb.begin(), vb.end());
        size_t i = 0, j = 0;
        while (i < va.size() && j < vb.size()) {
            int pa = va[i], pb = vb[j];
            pairs.emplace_back(pa, pb);
            if (pa < pb)
                ++i;
            else
                ++j;
        }
    }
    if (pairs.empty())
        return {};

    std::sort(pairs.begin(),
              pairs.end(),
              [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
                  return a.first < b.first;
              });

    anchors = extractAnchorsLIS(pairs);

    return anchors;
}

// 앵커로 나온 순서 쌍 LIS 적용
// second를 기준으로 LIS 계산
// tails = LIS로 나온 값
// tailIdx = 나온 값의 pairs 벡터 위치
// prevIdx = tailIdx의 이전 pairs 벡터 위치
std::vector<std::pair<int, int>>
DiffPython::extractAnchorsLIS(const std::vector<std::pair<int, int>> &pairs)
{
    if (pairs.empty())
        return {};

    std::vector<std::pair<int, int>> anchors;
    std::vector<int>                 tails;
    std::vector<int>                 tailIdx;
    std::vector<int>                 prevIdx;

    anchors.reserve(pairs.size());
    tails.reserve(pairs.size());
    tailIdx.reserve(pairs.size());
    prevIdx.resize(pairs.size(), -1);

    for (size_t idx = 0; idx < pairs.size(); ++idx) {
        const int    val = pairs[idx].second;
        auto         it  = std::lower_bound(tails.begin(), tails.end(), val);
        const size_t k   = static_cast<size_t>(it - tails.begin());
        if (it == tails.end()) {
            tails.push_back(val);
            tailIdx.push_back(static_cast<int>(idx));
        } else {
            *it        = val;
            tailIdx[k] = static_cast<int>(idx);
        }
        if (k > 0)
            prevIdx[idx] = tailIdx[k - 1];
    }

    if (!tails.empty()) {
        int p = tailIdx.back();
        while (p != -1) {
            anchors.push_back(pairs[p]);
            p = prevIdx[p];
        }
        std::reverse(anchors.begin(), anchors.end());
    }
    return anchors;
}

// LCS DP 폴백 + 소구간 push
// ADD / DEL 처리
// LCS 계산 후 ops에 push
void DiffPython::dpFallback(const std::vector<int>    &Ai,
                            const std::vector<int>    &Bi,
                            int                        ax,
                            int                        ay,
                            int                        bx,
                            int                        by,
                            /*inout*/ std::vector<Op> &ops)
{
    int n = ay - ax, m = by - bx;
    if (n == 0) {
        for (int j = 0; j < m; ++j)
            ops.emplace_back(Op::ADD, -1, bx + j);
        return;
    }
    if (m == 0) {
        for (int i = 0; i < n; ++i)
            ops.emplace_back(Op::DEL, ax + i, -1);
        return;
    }

    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    std::vector<Op>               tmp;
    int                           i, j;
    tmp.reserve(n + m);

    for (i = 1; i <= n; ++i)
        for (j = 1; j <= m; ++j)
            dp[i][j] = (Ai[ax + i - 1] == Bi[bx + j - 1]) ? dp[i - 1][j - 1] + 1
                                                          : std::max(dp[i - 1][j], dp[i][j - 1]);
    i = n;
    j = m;

    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && Ai[ax + i - 1] == Bi[bx + j - 1]) {
            tmp.push_back({Op::SAME, ax + i - 1, bx + j - 1});
            --i;
            --j;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            tmp.push_back({Op::ADD, -1, bx + j - 1});
            --j;
        } else {
            tmp.push_back({Op::DEL, ax + i - 1, -1});
            --i;
        }
    }
    for (int t = static_cast<int>(tmp.size()) - 1; t >= 0; --t)
        ops.emplace_back(tmp[t]);
}

// 분할정복 Myers (선형 메모리) + 결과 push
// 작은 구간은 DP 폴백 / ADD / DEL 처리
// → forward, reverse 교차 탐색
// → 중간 스네이크 길이 계산
// → 스네이크 구간 push + 나머지 구간 재귀
// n = A의 구간 길이 (ax, ay)
// m = B의 구간 길이 (bx, by)
// delta = 대각선 길이 차
// odd = 홀짝 여부 (교차 판정에 사용)
// Dmax = 최대 편집거리 상한
// OFF = 중앙 오프셋
// xF, yF = forward 탐색의 교차점 좌표
// xR, yR = reverse 탐색의 교차점 좌표
// Vf = forward 탐색에서 대각선 상 가장 큰 진행도 (x)
// Vr = reverse 탐색에서 대각선 상 가장 큰 역방향 진행도 (x)
void DiffPython::dcMyers(const std::vector<int>    &Ai,
                         const std::vector<int>    &Bi,
                         int                        ax,
                         int                        ay,
                         int                        bx,
                         int                        by,
                         /*inout*/ std::vector<Op> &ops)
{
    const int n = ay - ax, m = by - bx;
    if (n == 0) {
        for (int j = 0; j < m; ++j)
            ops.emplace_back(Op::ADD, -1, bx + j);
        return;
    }
    if (m == 0) {
        for (int i = 0; i < n; ++i)
            ops.emplace_back(Op::DEL, ax + i, -1);
        return;
    }
    if (n <= 32 || m <= 32) {
        dpFallback(Ai, Bi, ax, ay, bx, by, ops);
        return;
    }

    const int        delta = n - m;
    const bool       odd   = (delta & 1);
    const int        Dmax  = (n + m + 1) / 2;
    const int        OFF   = Dmax + 1;
    int              xF = 0, yF = 0, xR = 0, yR = 0;
    bool             found = false;
    int              snake = 0;
    std::vector<int> Vf(2 * Dmax + 3, -1), Vr(2 * Dmax + 3, -1);
    Vf[OFF] = Vr[OFF] = 0;

    for (int d = 0; d <= Dmax && !found; ++d) {
        // forward
        for (int k = -d; k <= d; k += 2) {
            int idx = k + OFF;
            int x   = (k == -d || (k != d && Vf[idx - 1] < Vf[idx + 1])) ? Vf[idx + 1]
                                                                         : (Vf[idx - 1] + 1);
            int y   = x - k;
            while (x < n && y < m && Ai[ax + x] == Bi[bx + y]) {
                ++x;
                ++y;
            }
            Vf[idx] = x;

            if (odd && (k >= delta - d) && (k <= delta + d)) {
                int kr = k - delta, idxr = kr + OFF;
                if (Vr[idxr] != -1 && Vf[idx] + Vr[idxr] >= n) {
                    xF    = x;
                    yF    = y;
                    xR    = n - Vr[idxr];
                    yR    = m - (Vr[idxr] - kr);
                    found = true;
                    break;
                }
            }
        }
        if (found)
            break;

        // reverse
        for (int k = -d; k <= d; k += 2) {
            int idx = k + OFF;
            int x   = (k == -d || (k != d && Vr[idx + 1] < Vr[idx - 1])) ? Vr[idx + 1]
                                                                         : (Vr[idx - 1] + 1);
            int y   = x - k;
            while (x < n && y < m && Ai[ay - 1 - x] == Bi[by - 1 - y]) {
                ++x;
                ++y;
            }
            Vr[idx] = x;

            if (!odd) {
                int kf = k + delta, idxf = kf + OFF;
                if (Vf[idxf] != -1 && Vf[idxf] + Vr[idx] >= n) {
                    xF    = Vf[idxf];
                    yF    = xF - kf;
                    xR    = n - Vr[idx];
                    yR    = m - (Vr[idx] - k);
                    found = true;
                    break;
                }
            }
        }
    }
    if (!found) {
        dpFallback(Ai, Bi, ax, ay, bx, by, ops);
        return;
    }

    while (xF + snake < xR && yF + snake < yR && Ai[ax + xF + snake] == Bi[bx + yF + snake])
        ++snake;

    dcMyers(Ai, Bi, ax, ax + xF, bx, bx + yF, ops);
    for (int t = 0; t < snake; ++t)
        ops.emplace_back(Op::SAME, ax + xF + t, bx + yF + t);
    dcMyers(Ai, Bi, ax + xF + snake, ay, bx + yF + snake, by, ops);
}

// 접미 SAME push
void DiffPython::appendSuffixSame(const std::vector<int>    &Ai,
                                  const std::vector<int>    &Bi,
                                  int                        a1,
                                  int                        b1,
                                  /*inout*/ std::vector<Op> &ops)
{
    const int n = static_cast<int>(Ai.size());
    const int m = static_cast<int>(Bi.size());
    for (int i = a1, j = b1; i < n && j < m; ++i, ++j) {
        ops.emplace_back(Op::SAME, i, j);
    }
}

// Diff.Kind를 문자열로 변환(enum -> string)
const char *DiffPython::kindToStr(Diff::Kind kind)
{
    switch (kind) {
    case Diff::ADD:
        return "added";
    case Diff::DEL:
        return "deleted";
    case Diff::MOD:
        return "modified";
    case Diff::SAME:
        return "same";
    }
    return "unknown";
}

// Diff 벡터로부터 JSON 변경사항과 통계를 작성
void DiffPython::buildChangesJson(const std::vector<Diff> &diffs,
                                  /*out*/ Json::Value     &changes,
                                  /*out*/ DiffStats       &stats)
{
    changes = Json::Value(Json::arrayValue);

    for (const auto &d : diffs) {
        // 통계 집계
        switch (d.kind) {
        case Diff::ADD:
            ++stats.added;
            break;
        case Diff::DEL:
            ++stats.deleted;
            break;
        case Diff::MOD:
            ++stats.modified;
            break;
        case Diff::SAME:
            break;
        }

        if (d.kind == Diff::SAME) {
            continue;
        }

        Json::Value item(Json::objectValue);
        item["type"] = kindToStr(d.kind);

        if (d.baseLine >= 0) {
            item["baseValue"]      = d.base;
            item["baseLineNumber"] = d.baseLine + 1;
            item["baseLineCount"]  = 1;
        } else {
            item["baseValue"]      = Json::nullValue;
            item["baseLineNumber"] = Json::nullValue;
            item["baseLineCount"]  = 0;
        }

        if (d.compareLine >= 0) {
            item["compareValue"]      = d.compare;
            item["compareLineNumber"] = d.compareLine + 1;
            item["compareLineCount"]  = 1;
        } else {
            item["compareValue"]      = Json::nullValue;
            item["compareLineNumber"] = Json::nullValue;
            item["compareLineCount"]  = 0;
        }

        item["path"] = Json::nullValue;
        changes.append(std::move(item));
    }
}

// Op → Diff + DEL/ADD → MOD 접기
std::vector<Diff> DiffPython::foldOpsToDiffs(const std::vector<Op>             &ops,
                                             const std::vector<NormalizedLine> &aNorm,
                                             const std::vector<NormalizedLine> &bNorm,
                                             const std::string                 &contentA,
                                             const std::string                 &contentB)
{
    auto getLine = [](const std::string &s, const NormalizedLine &L) -> std::string {
        return std::string(s.data() + L.start, L.len);
    };

    std::vector<Diff> out;
    out.reserve(ops.size());

    for (size_t p = 0; p < ops.size();) {
        if (ops[p].k == Op::SAME) {
            int ai = ops[p].ai, bj = ops[p].bj;
            out.push_back({ai,
                           bj,
                           getLine(contentA, aNorm[ai]),
                           getLine(contentB, bNorm[bj]),
                           Diff::SAME});
            ++p;
            continue;
        }
        size_t              q = p;
        std::vector<size_t> dels, adds;
        while (q < ops.size() && ops[q].k != Op::SAME) {
            (ops[q].k == Op::DEL ? dels : adds).push_back(q);
            ++q;
        }
        size_t pair = std::min(dels.size(), adds.size());
        for (size_t t = 0; t < pair; ++t) {
            const Op &d1 = ops[dels[t]];
            const Op &d2 = ops[adds[t]];
            out.push_back({d1.ai,
                           d2.bj,
                           getLine(contentA, aNorm[d1.ai]),
                           getLine(contentB, bNorm[d2.bj]),
                           Diff::MOD});
        }
        for (size_t t = pair; t < dels.size(); ++t) {
            const Op &d1 = ops[dels[t]];
            out.push_back({d1.ai, -1, getLine(contentA, aNorm[d1.ai]), std::string(), Diff::DEL});
        }
        for (size_t t = pair; t < adds.size(); ++t) {
            const Op &d2 = ops[adds[t]];
            out.push_back({-1, d2.bj, std::string(), getLine(contentB, bNorm[d2.bj]), Diff::ADD});
        }
        p = q;
    }
    return out;
}
