#include "DiffPython.h"

#include <algorithm>
#include <cctype>
#include <json/json.h>
#include <utility>
#include <vector>

// 공백 문자 판정
static inline bool isWs(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\f' || c == '\v';
}

static inline bool startsWith3(const std::string& s, size_t pos, size_t endExcl, char q) {
    return pos + 2 < endExcl && s[pos] == q && s[pos + 1] == q && s[pos + 2] == q;
}

// 정규화(양끝 공백 제거 + 내부 연속 공백 축약 + 주석 처라) + rank 계산
// inSpace = 공백
// inQuote = ""
// prevEscape = \ 
// quoteChar = " or ', default = \0
// rank = -1은 공백 or 주석
NormalizedLine DiffPython::normalizeOne(const std::string& content,
                                        size_t start,
                                        size_t endExcl,
                                        std::vector<int>& deep,
                                        bool& inTriple,
                                        char& tripleQuoteChar)
{
    std::string body;
    bool inSpace = false;
    bool inQuote = false;
    bool prevEscape = false;
    char quoteChar = '\0';
    int rank = -1;
    size_t i = start, j = endExcl;

    while (i < j && isWs(static_cast<unsigned char>(content[i]))) ++i;
    while (j > i && isWs(static_cast<unsigned char>(content[j - 1]))) --j;

    body.reserve(j - i);

    for (size_t k = i; k < j; ++k) {
        unsigned char c = static_cast<unsigned char>(content[k]);
        
        if (inTriple) {
            if (startsWith3(content, k, j, tripleQuoteChar)) {
                inTriple = false;
                tripleQuoteChar = '\0';
                k += 2;
            }
            continue;
        }

        if ((c == '"' || c == '\'') && startsWith3(content, k, j, c)) {
            inSpace = false;
            inTriple = true;
            tripleQuoteChar = static_cast<char>(c);
            k += 2;
            continue;
        }

        if (inQuote) {
            if (prevEscape) prevEscape = false;
            else if (c == '\\') prevEscape = true;
            else if (static_cast<char>(c) == quoteChar) { inQuote = false; quoteChar = '\0'; }
            body.push_back(static_cast<char>(c));
            continue;
        }

       if (c == '"' || c == '\'') {
            inSpace = false;    
            inQuote = true; 
            prevEscape = false;
            quoteChar = static_cast<char>(c);
            body.push_back(static_cast<char>(c));
            continue;
        }

        if (c == '#') {
            while (!body.empty() && body.back() == ' ') body.pop_back();
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

    return NormalizedLine{
        start,
        endExcl - start,
        rank,
        std::move(body)
    };
}

// 모든 라인 정규화
// 원본 라인 수(spans) 128 예상, 깊이(deep) 64 예상
// 인터널
std::vector<NormalizedLine> DiffPython::normalizeAll(const std::string& content)
{
    std::vector<NormalizedLine> out;
    std::vector<size_t> spans;
    std::vector<int> deep;
    bool inTriple = false;
    char tripleQuoteChar = '\0';

    spans.reserve(128);
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
        size_t start = spans[i - 1];
        size_t endExcl = spans[i];

        while (endExcl > start) {
            char ch = content[endExcl - 1];
            if (ch == '\n' || ch == '\r') --endExcl;
            else break;
        }
        out.emplace_back(normalizeOne(content, start, endExcl, deep, inTriple, tripleQuoteChar));
    }

    return out;
}

const char* DiffPython::kindToStr(Diff::Kind kind)
{
    switch (kind) {
        case Diff::ADD: return "added";
        case Diff::DEL: return "deleted";
        case Diff::MOD: return "modified";
        case Diff::SAME: return "same";
    }
    return "unknown";
}

// LCS 기반 엔진 구현
std::vector<Diff> DiffPython::compute(const std::vector<NormalizedLine>& aNorm,
                                      const std::vector<NormalizedLine>& bNorm,
                                      const std::string& contentA,
                                      const std::string& contentB)
{
    std::vector<int> Ai, Bi;
    std::vector<int> origA, origB;
    const int K = buildInternAndIdsRankBuckets(aNorm, bNorm, Ai, Bi, origA, origB);

    std::vector<Op> ops;
    ops.reserve(Ai.size() + Bi.size());

    int a0, a1, b0, b1;
    trimCommon(Ai, Bi, a0, a1, b0, b1, ops);

    if (a0 < a1 || b0 < b1) {
        solveWithAnchors(Ai, Bi, aNorm, bNorm, origA, origB, K, a0, a1, b0, b1, ops);
    }

    appendSuffixSame(Ai, Bi, a1, b1, ops);

    for (auto& op : ops) {
        if (op.ai >= 0) op.ai = origA[op.ai];
        if (op.bj >= 0) op.bj = origB[op.bj];
    }

    return foldOpsToDiffs(ops, aNorm, bNorm, contentA, contentB);
}


void DiffPython::buildChangesJson(const std::vector<Diff>& diffs,
                                  /*out*/ Json::Value& changes,
                                  /*out*/ DiffStats& stats)
{
    changes = Json::Value(Json::arrayValue);

    for (const auto& d : diffs) {
        // 통계 집계
        switch (d.kind) {
            case Diff::ADD: ++stats.added; break;
            case Diff::DEL: ++stats.deleted; break;
            case Diff::MOD: ++stats.modified; break;
            case Diff::SAME: break;
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

Json::Value DiffPython::runFromText(const std::string& contentA,
                                    const std::string& contentB,
                                    const std::string& nameA,
                                    const std::string& nameB)
{
    // 1) 정규화 (라인 분할 + 공백 축약 + rank 산출)
    std::vector<NormalizedLine> aNorm = normalizeAll(contentA);
    std::vector<NormalizedLine> bNorm = normalizeAll(contentB);

    // 2) LCS 기반 diff 생성
    std::vector<Diff> diffs = compute(aNorm, bNorm, contentA, contentB);

    // 3) changes 배열 + 통계 집계
    Json::Value changes(Json::arrayValue);
    DiffStats stats{};
    buildChangesJson(diffs, changes, stats);

    // 4) 루트 JSON 조립
    Json::Value root(Json::objectValue);

    Json::Value base(Json::objectValue);
    base["name"] = nameA;
    root["base"] = std::move(base);

    Json::Value compare(Json::objectValue);
    compare["name"] = nameB;
    root["compare"] = std::move(compare);

    Json::Value jstats(Json::objectValue);
    const Json::UInt64 total = static_cast<Json::UInt64>(stats.added + stats.deleted + stats.modified);
    jstats["added"]        = static_cast<Json::UInt64>(stats.added);
    jstats["deleted"]      = static_cast<Json::UInt64>(stats.deleted);
    jstats["modified"]     = static_cast<Json::UInt64>(stats.modified);
    jstats["totalChanges"] = total;
    root["statistics"]     = std::move(jstats);

    root["changes"] = std::move(changes);

    return root;
}

int DiffPython::buildInternAndIdsRankBuckets(const std::vector<NormalizedLine>& aNorm,
                                             const std::vector<NormalizedLine>& bNorm,
                                             /*out*/ std::vector<int>& Ai,
                                             /*out*/ std::vector<int>& Bi,
                                             /*out*/ std::vector<int>& origA,
                                             /*out*/ std::vector<int>& origB)
{
    
    struct SvHash { size_t operator()(std::string_view v) const noexcept { return std::hash<std::string_view>{}(v); } };
    struct SvEq   { bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; } };

    std::unordered_map<std::string_view,int,SvHash,SvEq> dict;
    dict.reserve((aNorm.size() + bNorm.size()) * 2 + 64);
    Ai.clear(); Bi.clear(); 
    origA.clear(); origB.clear();
    Ai.reserve(aNorm.size());
    Bi.reserve(bNorm.size());
    origA.reserve(aNorm.size());
    origB.reserve(bNorm.size());
    int nextId = 0;

    auto internSide = [&](const auto& src, std::vector<int>& I, std::vector<int>& orig){
        for (int i = 0; i < (int)src.size(); ++i) {
            if (src[i].rank < 0 || src[i].body.empty()) continue;
            std::string_view sv(src[i].body);
            int id;
            if (auto it = dict.find(sv); it == dict.end()) {
                id = ++nextId;
                dict.emplace(sv, id);
            } else {
                id = it->second;
            }
            I.push_back(id);
            orig.push_back(i);
        }
    };

    internSide(aNorm, Ai, origA);
    internSide(bNorm, Bi, origB);

    // K(어휘수) = 최대 ID 값 (정규화/intern 단계에서 전역 유일 보장)
    // 앵커/DP에서 sentinel 여유를 원하면 +2 유지
    return nextId + 2;
}


void DiffPython::trimCommon(const std::vector<int>& Ai, const std::vector<int>& Bi,
                            /*out*/ int& a0, /*out*/ int& a1,
                            /*out*/ int& b0, /*out*/ int& b1,
                            /*inout*/ std::vector<Op>& ops) {
    const int n = static_cast<int>(Ai.size());
    const int m = static_cast<int>(Bi.size());
    a0 = 0; b0 = 0;
    while (a0 < n && b0 < m && Ai[a0] == Bi[b0]) { ++a0; ++b0; }
    a1 = n; b1 = m;
    while (a1 > a0 && b1 > b0 && Ai[a1 - 1] == Bi[b1 - 1]) { --a1; --b1; }

    for (int i = 0; i < a0 && i < b0; ++i) {
        ops.push_back({Op::SAME, i, i});
    }
}

std::vector<std::pair<int,int>>
DiffPython::buildAnchorsUnique(const std::vector<int>& Ai, const std::vector<int>& Bi,
                               int x0,int x1,int y0,int y1, int K) {
                                
    // 1) 서브구간 빈도 계산
    std::vector<int> fA(K, 0), fB(K, 0);
    for (int i = x0; i < x1; ++i) ++fA[Ai[i]];
    for (int j = y0; j < y1; ++j) ++fB[Bi[j]];

    // 2) B 구간의 유니크 id → 위치 사전 (O(y1-y0))
    std::vector<int> posB(K, -1);
    for (int j = y0; j < y1; ++j) {
        int id = Bi[j];
        if (fB[id] == 1) posB[id] = j;
    }

    // 3) A에서 유니크 + B에서도 유니크인 라인만 매칭 (O(x1-x0))
    std::vector<std::pair<int,int>> pairs;
    pairs.reserve(std::min(x1 - x0, y1 - y0));
    for (int i = x0; i < x1; ++i) {
        int id = Ai[i];
        if (fA[id] == 1) {
            int j = posB[id];
            if (j != -1) pairs.emplace_back(i, j);
        }
    }
    if (pairs.empty()) return {};

    // 4) B 인덱스 기준 LIS로 순서 보존 앵커만 남기기 (O(u log u))
    std::vector<int> tails;               
    std::vector<int> tail_idx;
    std::vector<int> prev_idx(pairs.size(), -1);
    tail_idx.reserve(pairs.size());
    tails.reserve(pairs.size());

    for (int idx = 0; idx < (int)pairs.size(); ++idx) {
        int val = pairs[idx].second;
        auto it = std::lower_bound(tails.begin(), tails.end(), val);
        int k = int(it - tails.begin());
        if (it == tails.end()) {
            tails.push_back(val);
            tail_idx.push_back(idx);
        } else {
            *it = val;
            tail_idx[k] = idx;
        }
        if (k > 0) prev_idx[idx] = tail_idx[k - 1];
    }

    std::vector<std::pair<int,int>> anchors;
    if (!tails.empty()) {
        int p = tail_idx.back();
        while (p != -1) { anchors.push_back(pairs[p]); p = prev_idx[p]; }
        std::reverse(anchors.begin(), anchors.end());
    }
    return anchors;
}

static inline std::vector<std::string> tokenizeWords_sv(std::string_view s) {
    std::vector<std::string> out; out.reserve(16);
    std::string cur;
    for (unsigned char ch : s) {
        if (std::isalnum(ch) || ch=='_') cur.push_back((char)ch);
        else if (!cur.empty()) { out.push_back(cur); cur.clear(); }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::vector<std::pair<int,int>>
DiffPython::buildAnchorsHistogram(const std::vector<NormalizedLine>& aNorm,
                                  const std::vector<NormalizedLine>& bNorm,
                                  const std::vector<int>& Ai, 
                                  const std::vector<int>& Bi,
                                  const std::vector<int>& origA,
                                  const std::vector<int>& origB,
                                  int x0,int x1,int y0,int y1, int K)
{
    std::unordered_map<std::string,int> freq;
    freq.reserve((size_t)((x1-x0)+(y1-y0))*4 + 16);

    auto countSide = [&](int l0,int l1, const auto& norm, const auto& orig) {
        for (int i = l0; i < l1; ++i) {
            int idx = orig[i];
            const std::string& s = norm[idx].body;
            if (s.empty()) continue;
            for (auto& t : tokenizeWords_sv(std::string_view(s))) {
                if (t.size() > 1) ++freq[t];
            }
        }
    };
    countSide(x0,x1,aNorm,origA);
    countSide(y0,y1,bNorm,origB);

    auto score = [&](const auto& norm, int idx) {
        double sc = 0.0;
        const std::string& s = norm[idx].body;
        if (s.empty()) return 0.0;
        for (auto& t : tokenizeWords_sv(std::string_view(s))) {
            if (t.size() <= 1) continue;
            auto it = freq.find(t);
            if (it != freq.end() && it->second > 0) sc += 1.0 / (double)it->second;
        }
        return sc;
    };

    struct Cand { int pos; double sc; };
    std::vector<Cand> ca; ca.reserve((size_t)(x1-x0));
    std::vector<Cand> cb; cb.reserve((size_t)(y1-y0));
    for (int k = x0; k < x1; ++k) {
        int idxA = origA[k];
        double sc = score(aNorm, idxA);
        if (sc > 0) ca.push_back({k, sc});
    }
    for (int k = y0; k < y1; ++k) {
        int idxB = origB[k];
        double sc = score(bNorm, idxB);
        if (sc > 0) cb.push_back({k, sc});
    }
    if (ca.empty() || cb.empty()) return {};

    auto keepTopPct = [](std::vector<Cand>& v, double pct) {
        if (v.empty()) return;
        size_t k = std::max<size_t>(1, (size_t)(v.size() * pct));
        std::nth_element(v.begin(), v.begin() + k - 1, v.end(),
                         [](const Cand& a,const Cand& b){ return a.sc > b.sc; });
        v.resize(k);
        std::sort(v.begin(), v.end(), [](auto& a, auto& b){ return a.pos < b.pos; });
    };
    keepTopPct(ca, 0.25);
    keepTopPct(cb, 0.25);

    std::vector<std::vector<int>> buckA(K), buckB(K);
    for (auto& c : ca) buckA[Ai[c.pos]].push_back(c.pos);
    for (auto& c : cb) buckB[Bi[c.pos]].push_back(c.pos);

    std::vector<std::pair<int,int>> pairs; 
    pairs.reserve(std::min(ca.size(), cb.size()));
    for (int id = 0; id < K; ++id) {
        auto& va = buckA[id];
        auto& vb = buckB[id];
        if (va.empty() || vb.empty()) continue;
        std::sort(va.begin(), va.end());
        std::sort(vb.begin(), vb.end());
        size_t i = 0, j = 0;
        while (i < va.size() && j < vb.size()) {
            int pa = va[i], pb = vb[j];
            pairs.emplace_back(pa, pb);
            if (pa < pb) ++i; else ++j;
        }
    }
    if (pairs.empty()) return {};
    
    // LIS on posB
    std::sort(pairs.begin(), pairs.end(), [](auto& a, auto& b){ return a.first < b.first; });
    std::vector<int> tails, tail_idx, prev_idx(pairs.size(), -1);
    for (int i = 0; i < (int)pairs.size(); ++i) {
        int val = pairs[i].second;
        auto it = std::lower_bound(tails.begin(), tails.end(), val);
        int k = (int)(it - tails.begin());
        if (it == tails.end()) { tails.push_back(val); tail_idx.push_back(i); }
        else { *it = val; tail_idx[k] = i; }
        if (k > 0) prev_idx[i] = tail_idx[k - 1];
    }
    std::vector<std::pair<int,int>> anchors;
    if (!tails.empty()) {
        int p = tail_idx.back();
        while (p != -1) { anchors.push_back(pairs[p]); p = prev_idx[p]; }
        std::reverse(anchors.begin(), anchors.end());
    }
    return anchors;
}

void DiffPython::dpFallback(const std::vector<int>& Ai, const std::vector<int>& Bi,
                            int ax,int ay,int bx,int by,
                            /*inout*/ std::vector<Op>& ops) {
    int n = ay - ax, m = by - bx;
    if (n == 0) { for (int j = 0; j < m; ++j) ops.push_back({Op::ADD, -1, bx + j}); return; }
    if (m == 0) { for (int i = 0; i < n; ++i) ops.push_back({Op::DEL, ax + i, -1}); return; }

    std::vector<std::vector<int>> dp(n+1, std::vector<int>(m+1, 0));
    for (int i = 1; i <= n; ++i)
        for (int j = 1; j <= m; ++j)
            dp[i][j] = (Ai[ax+i-1] == Bi[bx+j-1]) ? dp[i-1][j-1] + 1
                                                  : std::max(dp[i-1][j], dp[i][j-1]);

    std::vector<Op> tmp; tmp.reserve(n + m);
    int i = n, j = m;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && Ai[ax+i-1] == Bi[bx+j-1]) { tmp.push_back({Op::SAME, ax+i-1, bx+j-1}); --i; --j; }
        else if (j > 0 && (i == 0 || dp[i][j-1] >= dp[i-1][j])) { tmp.push_back({Op::ADD, -1, bx+j-1}); --j; }
        else { tmp.push_back({Op::DEL, ax+i-1, -1}); --i; }
    }
    for (int t = static_cast<int>(tmp.size()) - 1; t >= 0; --t) ops.push_back(tmp[t]);
}

void DiffPython::dcMyers(const std::vector<int>& Ai, const std::vector<int>& Bi,
                         int ax,int ay,int bx,int by,
                         /*inout*/ std::vector<Op>& ops) {
    const int n = ay - ax, m = by - bx;
    if (n == 0) { for (int j = 0; j < m; ++j) ops.push_back({Op::ADD, -1, bx + j}); return; }
    if (m == 0) { for (int i = 0; i < n; ++i) ops.push_back({Op::DEL, ax + i, -1}); return; }
    if (n <= 32 || m <= 32) { dpFallback(Ai, Bi, ax, ay, bx, by, ops); return; }

    const int delta = n - m;            // 길이 차
    const bool odd  = (delta & 1);      // 홀짝 여부(교차 판정에 사용)
    const int Dmax  = (n + m + 1) / 2;  // 최댓 편집거리 상한
    const int OFF   = Dmax + 1;
    std::vector<int> Vf(2*Dmax+3, -1), Vr(2*Dmax+3, -1);
    Vf[OFF] = 0; Vr[OFF] = 0;           // 출발점(로컬 좌표)

    int xF = 0, yF = 0, xR = 0, yR = 0; bool found = false;
    for (int d = 0; d <= Dmax && !found; ++d) {
        // forward
        for (int k = -d; k <= d; k += 2) {
            int idx = k + OFF;
            int x = (k == -d || (k != d && Vf[idx-1] < Vf[idx+1])) ? Vf[idx+1] : (Vf[idx-1] + 1);
            int y = x - k;
            while (x < n && y < m && Ai[ax+x] == Bi[bx+y]) { ++x; ++y; }
            Vf[idx] = x;

            if (odd && (k >= delta - d) && (k <= delta + d)) {
                int kr = k - delta, idxr = kr + OFF;
                if (Vr[idxr] != -1 && Vf[idx] + Vr[idxr] >= n) {
                    xF = x; yF = y; xR = n - Vr[idxr]; yR = m - (Vr[idxr] - kr); found = true; break;
                }
            }
        }
        if (found) break;

        // reverse
        for (int k = -d; k <= d; k += 2) {
            int idx = k + OFF;
            int x = (k == -d || (k != d && Vr[idx+1] < Vr[idx-1])) ? Vr[idx+1] : (Vr[idx-1] + 1);
            int y = x - k;
            while (x < n && y < m && Ai[ay-1-x] == Bi[by-1-y]) { ++x; ++y; }
            Vr[idx] = x;

            if (!odd) {
                int kf = k + delta, idxf = kf + OFF;
                if (Vf[idxf] != -1 && Vf[idxf] + Vr[idx] >= n) {
                    xF = Vf[idxf]; yF = xF - kf; xR = n - Vr[idx]; yR = m - (Vr[idx] - k);
                    found = true; break;
                }
            }
        }
    }
    if (!found) { dpFallback(Ai, Bi, ax, ay, bx, by, ops); return; }

    int snake = 0;
    while (xF + snake < xR && yF + snake < yR && Ai[ax+xF+snake] == Bi[bx+yF+snake]) ++snake;

    dcMyers(Ai, Bi, ax, ax + xF, bx, bx + yF, ops);           // left
    for (int t = 0; t < snake; ++t) ops.push_back({Op::SAME, ax + xF + t, bx + yF + t}); // middle
    dcMyers(Ai, Bi, ax + xF + snake, ay, bx + yF + snake, by, ops); // right
}

void DiffPython::solveWithAnchors(const std::vector<int>& Ai, const std::vector<int>& Bi,
                                  const std::vector<NormalizedLine>& aNorm,
                                  const std::vector<NormalizedLine>& bNorm,
                                  const std::vector<int>& origA,
                                  const std::vector<int>& origB,
                                  int K,
                                  int a0,int a1,int b0,int b1,
                                  /*inout*/ std::vector<Op>& ops)
{
    auto anchors = buildAnchorsUnique(Ai, Bi, a0, a1, b0, b1, K);
    if (anchors.size() < 2) {
        auto hist = buildAnchorsHistogram(aNorm, bNorm, Ai, Bi, origA, origB, a0, a1, b0, b1, K);
        if (hist.size() > anchors.size()) anchors = std::move(hist);
    }

    const int MOVE_GUARD_ABS = 8;                       // 절대 상한(작게)
    const double MOVE_GUARD_RATIO = 0.05;               // 구간 5% 초과면 거부
    const int DIAG_GUARD = 64;                          // 대각선에서 너무 멀면 거부(|qa-qb|>64)

    int pa = a0, pb = b0;
    for (auto& anc : anchors) {
        int qa = anc.first, qb = anc.second;
        if (qa < pa || qb < pb || qa < a0 || qa >= a1 || qb < b0 || qb >= b1) continue;

        // 앵커 앞 구간 크기(= 이 앵커를 채택하면 앞에서 발생할 ADD/DEL 대략치)
        int preA = qa - pa;
        int preB = qb - pb;
        int preCost = preA + preB;

        int segA = a1 - pa;
        int segB = b1 - pb;
        int segMin = std::min(segA, segB);

        bool tooFarFromDiag = (qa - qb > DIAG_GUARD) || (qb - qa > DIAG_GUARD);
        bool tooLargeAbs    = preCost > MOVE_GUARD_ABS;
        bool tooLargeRatio  = segMin > 0 && preCost > int(MOVE_GUARD_RATIO * segMin);

        // 가드: 손해 큰 앵커는 거부해서 전역 Myers로 해결
        if (tooFarFromDiag || tooLargeAbs || tooLargeRatio) {
            // 이 구간 전체를 한 번에 풉니다 → 전역 최소에 가까운 해를 얻음
            dcMyers(Ai, Bi, pa, a1, pb, b1, ops);
            return;
        }

        if (pa < qa || pb < qb) dcMyers(Ai, Bi, pa, qa, pb, qb, ops);
        ops.push_back({Op::SAME, qa, qb});
        pa = qa + 1; 
        pb = qb + 1;
    }
    if (pa < a1 || pb < b1) dcMyers(Ai, Bi, pa, a1, pb, b1, ops);
}



void DiffPython::appendSuffixSame(const std::vector<int>& Ai, const std::vector<int>& Bi,
                                  int a1,int b1,
                                  /*inout*/ std::vector<Op>& ops) {
    const int n = static_cast<int>(Ai.size());
    const int m = static_cast<int>(Bi.size());
    for (int i = a1, j = b1; i < n && j < m; ++i, ++j) {
        ops.push_back({Op::SAME, i, j});
    }
}

std::vector<Diff> DiffPython::foldOpsToDiffs(const std::vector<Op>& ops,
                                             const std::vector<NormalizedLine>& aNorm,
                                             const std::vector<NormalizedLine>& bNorm,
                                             const std::string& contentA,
                                             const std::string& contentB)
{
    auto getLine = [](const std::string& s, const NormalizedLine& L) -> std::string {
        return std::string(s.data() + L.start, L.len);
    };
    
    std::vector<Diff> out;
    out.reserve(ops.size());

    for (size_t p = 0; p < ops.size();) {
        if (ops[p].k == Op::SAME) {
            int ai = ops[p].ai, bj = ops[p].bj;
            out.push_back({ ai, bj,
                    getLine(contentA, aNorm[ai]),
                    getLine(contentB, bNorm[bj]),
                    Diff::SAME });
            ++p; 
            continue;
        }
        size_t q = p;
        std::vector<size_t> dels, adds;
        while (q < ops.size() && ops[q].k != Op::SAME) {
            (ops[q].k == Op::DEL ? dels : adds).push_back(q);
            ++q;
        }
        size_t pair = std::min(dels.size(), adds.size());
        for (size_t t = 0; t < pair; ++t) {
            auto& d1 = ops[dels[t]];
            auto& d2 = ops[adds[t]];
            out.push_back({ d1.ai, d2.bj,
                            getLine(contentA, aNorm[d1.ai]),
                            getLine(contentB, bNorm[d2.bj]),
                            Diff::MOD });
        }
        for (size_t t = pair; t < dels.size(); ++t) {
            auto& d1 = ops[dels[t]];
            out.push_back({ d1.ai, -1,
                            getLine(contentA, aNorm[d1.ai]),
                            std::string(),
                            Diff::DEL });
        }
        for (size_t t = pair; t < adds.size(); ++t) {
            auto& d2 = ops[adds[t]];
            out.push_back({ -1, d2.bj,
                            std::string(),
                            getLine(contentB, bNorm[d2.bj]),
                            Diff::ADD });
        }
        p = q;
    }
    return out;
}