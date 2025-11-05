#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace Json { class Value; }

// 두 라인의 차이를 담은 구조체.
struct Diff
{
    enum Kind { SAME, ADD, DEL, MOD };

    int baseLine;
    int compareLine;
    std::string base;
    std::string compare;
    Kind kind;
};

// diff 통계치를 담은 구조체.
struct DiffStats
{
    size_t added = 0;
    size_t deleted = 0;
    size_t modified = 0;
};

// 정규화된 한 줄에 대한 메타데이터.
struct NormalizedLine
{
    size_t start;
    size_t len;
    int rank;
    std::string body;
};

struct Op
{ 
    enum K { SAME, ADD, DEL } k; 
    int ai; 
    int bj; 
};

class DiffPython
{
public:
    // 텍스트파일 A, B에 대해 diff를 수행하고 JSON 결과를 반환.
    // 입력: contentA, contentB, nameA, nameB
    // 출력: Json::Value (changes/통계)
    static Json::Value runFromText(const std::string& contentA,
                                   const std::string& contentB,
                                   const std::string& nameA,
                                   const std::string& nameB);

// private:
    // 한 줄[start, endExcl) 정규화.
    static NormalizedLine normalizeOne(const std::string& content,
                                       size_t start,
                                       size_t endExcl,
                                       std::vector<int>& deep,
                                       bool& inTriple,
                                       char& tripleQuoteChar);
    // 텍스트파일을 라인 단위로 정규화.
    static std::vector<NormalizedLine> normalizeAll(const std::string& content);

    // Diff.Kind를 문자열로 변환(enum -> string).
    static const char* kindToStr(Diff::Kind kind);

    // 정규화된 라인 벡터를 기반으로 diff 벡터를 계산.
    static std::vector<Diff> compute(const std::vector<NormalizedLine>& aNorm,
                                    const std::vector<NormalizedLine>& bNorm,
                                    const std::string& contentA,
                                    const std::string& contentB);

    // diff 벡터로부터 JSON 변경사항과 통계를 작성.
    static void buildChangesJson(const std::vector<Diff>& diffs,
                                 /*out*/ Json::Value& changes,
                                 /*out*/ DiffStats& stats);



    // rank와 raw로 인터널
    static int buildInternAndIdsRankBuckets(const std::vector<NormalizedLine>& aNorm,
                                            const std::vector<NormalizedLine>& bNorm,
                                            /*out*/ std::vector<int>& Ai,
                                            /*out*/ std::vector<int>& Bi,
                                            /*out*/ std::vector<int>& origA,
                                            /*out*/ std::vector<int>& origB);

    // 공통 접두/접미 트림 + 접두 SAME push
    static void trimCommon(const std::vector<int>& Ai, const std::vector<int>& Bi,
                           /*out*/ int& a0, 
                           /*out*/ int& a1,
                           /*out*/ int& b0, 
                           /*out*/ int& b1,
                           /*inout*/ std::vector<Op>& ops);

    // 유니크 라인(패이션스) 기반 앵커
    static std::vector<std::pair<int,int>> buildAnchorsUnique(const std::vector<int>& Ai,
                                                              const std::vector<int>& Bi,
                                                              int x0, int x1, int y0, int y1, int K);

    // 히스토그램 기반 앵커
    static std::vector<std::pair<int,int>> buildAnchorsHistogram(const std::vector<NormalizedLine>& aNorm,
                                                                 const std::vector<NormalizedLine>& bNorm,
                                                                 const std::vector<int>& Ai,
                                                                 const std::vector<int>& Bi,
                                                                 const std::vector<int>& origA,
                                                                 const std::vector<int>& origB,
                                                                 int x0, int x1, int y0, int y1, int K);

    // LCS DP 폴백 + 소구간 push
    static void dpFallback(const std::vector<int>& Ai, const std::vector<int>& Bi,
                           int ax, int ay, int bx, int by,
                           /*inout*/ std::vector<Op>& ops);

    // Divide & Conquer Myers (선형 메모리) + 결과 push
    static void dcMyers(const std::vector<int>& Ai, const std::vector<int>& Bi,
                        int ax, int ay, int bx, int by,
                        /*inout*/ std::vector<Op>& ops);

    // 앵커 기반 구간 분할/해결
    static void solveWithAnchors(const std::vector<int>& Ai, const std::vector<int>& Bi,
                                 const std::vector<NormalizedLine>& aNorm,
                                 const std::vector<NormalizedLine>& bNorm,
                                 const std::vector<int>& origA,
                                 const std::vector<int>& origB,
                                 int K,
                                 int a0, int a1, int b0, int b1,
                                 /*inout*/ std::vector<Op>& ops);

    // 접미 SAME push
    static void appendSuffixSame(const std::vector<int>& Ai, const std::vector<int>& Bi,
                                 int a1, int b1,
                                 /*inout*/ std::vector<Op>& ops);

    // Op → Diff + DEL/ADD → MOD 접기
    static std::vector<Diff> foldOpsToDiffs(const std::vector<Op>& ops,
                                            const std::vector<NormalizedLine>& aNorm,
                                            const std::vector<NormalizedLine>& bNorm,
                                            const std::string& contentA,
                                            const std::string& contentB);
};
