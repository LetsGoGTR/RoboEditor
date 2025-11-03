#include "DiffPython.h"

#include <algorithm>
#include <cctype>
#include <json/json.h>
#include <utility>

// 공백 문자 판정
static inline bool isWs(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\n' ||
           c == '\r' || c == '\f' || c == '\v';
}

// 공백 정규화(양끝 공백 제거 + 내부 연속 공백 하나로 축약) 및 rank 계산
NormalizedLine DiffPython::normalizeOne(const std::string& content,
                                        size_t start,
                                        size_t endExcl,
                                        std::vector<int>& deep)
{
    size_t i = start, j = endExcl;

    while (i < j && isWs(static_cast<unsigned char>(content[i]))) ++i;
    while (j > i && isWs(static_cast<unsigned char>(content[j - 1]))) --j;

    std::string body;
    body.reserve(j - i);
    bool inSpace = false;

    for (size_t k = i; k < j; ++k) {
        unsigned char c = static_cast<unsigned char>(content[k]);
        if (isWs(c)) {
            if (!inSpace) {
                body.push_back(' ');
                inSpace = true;
            }
        } else {
            body.push_back(static_cast<char>(c));
            inSpace = false;
        }
    }

    const int lead = static_cast<int>(i - start);
    while (!deep.empty() && lead < deep.back()) {
        deep.pop_back();
    }
    if (deep.empty() || lead > deep.back()) {
        deep.push_back(lead);
    }
    const int rank = static_cast<int>(deep.size()) - 1;

    return NormalizedLine{
        start,
        endExcl - start,
        std::move(body),
        rank
    };
}

// 모든 라인 정규화
// 원본 라인(spans) 128 예상, 깊이(deep) 64 예상
std::vector<NormalizedLine> DiffPython::normalizeAll(const std::string& content)
{
    std::vector<NormalizedLine> out;
    std::vector<size_t> spans;
    std::vector<int> deep;

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
        size_t endExcl = spans[i] - 1; // 기존 로직 보존

        if (endExcl > start && content[endExcl - 1] == '\r') {
            --endExcl;
        }

        out.push_back(normalizeOne(content, start, endExcl, deep));
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
    const int n = static_cast<int>(aNorm.size());
    const int m = static_cast<int>(bNorm.size());

    // LCS DP
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    for (int i = n - 1; i >= 0; --i) {
        for (int j = m - 1; j >= 0; --j) {
            if (aNorm[i].rank == bNorm[j].rank && aNorm[i].body == bNorm[j].body) {
                dp[i][j] = dp[i + 1][j + 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i + 1][j], dp[i][j + 1]);
            }
        }
    }

    // 경로 복원
    struct Op { enum Kind { SAME, ADD, DEL } k; int i; int j; };

    std::vector<Op> ops;
    ops.reserve(n + m);
    int i = 0, j = 0;

    while (i < n && j < m) {
        if (aNorm[i].rank == bNorm[j].rank && aNorm[i].body == bNorm[j].body) {
            ops.push_back({Op::SAME, i, j});
            ++i; ++j;
        } else if (dp[i + 1][j] >= dp[i][j + 1]) {
            ops.push_back({Op::DEL, i, j});
            ++i;
        } else {
            ops.push_back({Op::ADD, i, j});
            ++j;
        }
    }
    while (i < n) { ops.push_back({Op::DEL, i, m}); ++i; }
    while (j < m) { ops.push_back({Op::ADD, n, j}); ++j; }

    // DEL/ADD 블록을 MOD로 페어링
    std::vector<Diff> diffs;
    diffs.reserve(ops.size());

    for (size_t p = 0; p < ops.size(); ) {
        if (ops[p].k == Op::SAME) {
            int ai = ops[p].i;
            int bj = ops[p].j;
            const NormalizedLine& A = aNorm[ai];
            const NormalizedLine& B = bNorm[bj];
            diffs.push_back({
                ai, bj,
                contentA.substr(A.start, A.len),
                contentB.substr(B.start, B.len),
                Diff::SAME
            });
            ++p;
            continue;
        }

        size_t q = p;
        std::vector<int> dels;
        std::vector<int> adds;

        while (q < ops.size() && ops[q].k != Op::SAME) {
            if (ops[q].k == Op::DEL) {
                dels.push_back(ops[q].i);
            } else {
                adds.push_back(ops[q].j);
            }
            ++q;
        }

        size_t pairCnt = std::min(dels.size(), adds.size());
        for (size_t t = 0; t < pairCnt; ++t) {
            int ai = dels[t];
            int bj = adds[t];
            const NormalizedLine& A = aNorm[ai];
            const NormalizedLine& B = bNorm[bj];
            diffs.push_back({
                ai, bj,
                contentA.substr(A.start, A.len),
                contentB.substr(B.start, B.len),
                Diff::MOD
            });
        }

        for (size_t t = pairCnt; t < dels.size(); ++t) {
            int ai = dels[t];
            const NormalizedLine& A = aNorm[ai];
            diffs.push_back({
                ai, -1,
                contentA.substr(A.start, A.len),
                "",
                Diff::DEL
            });
        }

        for (size_t t = pairCnt; t < adds.size(); ++t) {
            int bj = adds[t];
            const NormalizedLine& B = bNorm[bj];
            diffs.push_back({
                -1, bj,
                "",
                contentB.substr(B.start, B.len),
                Diff::ADD
            });
        }

        p = q;
    }

    return diffs;
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
