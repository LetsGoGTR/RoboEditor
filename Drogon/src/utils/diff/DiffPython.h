#pragma once

#include <string>
#include <vector>

namespace Json
{
    class Value;
}

// 두 라인의 차이를 담은 구조체.
struct Diff
{
    enum Kind { SAME, ADD, DEL, MOD };

    int         baseLine;
    int         compareLine;
    std::string base;
    std::string compare;
    Kind        kind;
};

// diff 통계치를 담은 구조체.
struct DiffStats
{
    size_t added    = 0;
    size_t deleted  = 0;
    size_t modified = 0;
};

// 정규화된 한 줄에 대한 메타데이터.
struct NormalizedLine
{
    size_t      start;
    size_t      len;
    int         rank;
    std::string body;
};

// 라인 간 관계를 담은 구조체.
struct Op
{
    enum K { SAME, ADD, DEL } k;
    int ai;
    int bj;

    Op(K k, int a, int b) : k(k), ai(a), bj(b) {}
};

class DiffPython
{
  public:
    // 텍스트파일 A, B에 대해 diff를 수행하고 JSON 결과를 반환.
    // 입력: contentA, contentB, nameA, nameB
    // 출력: Json::Value (changes/통계)
    static Json::Value runFromText(const std::string &contentA,
                                   const std::string &contentB,
                                   const std::string &nameA,
                                   const std::string &nameB);

    // 텍스트파일의 정규화(주석, 공백제거) 반환.
    // 입력: content
    // 출력: vector<pair<string, int>> (정규화/rank)
    static std::vector<std::pair<std::string, int>> normalizeBodies(const std::string &content);

  private:
    // 한 줄[start, endExcl) 정규화.
    // 양끝 공백 제거 + 내부 연속 공백 축약 + 주석 처리 + rank 처리
    static NormalizedLine normalizeOne(const std::string &content,
                                       size_t             start,
                                       size_t             endExcl,
                                       std::vector<int>  &deep,
                                       bool              &inTriple,
                                       char              &tripleQuoteChar);
    // 텍스트 파일을 라인 단위로 정규화.
    // 라인으로 분해, 주석처리, 개행 처리
    static std::vector<NormalizedLine> normalizeAll(const std::string &content);

    // 정규화된 라인을 비교해 라인 간 관계를 표현한 목록 반환.
    // 정규화 정보 정수화 → 트림으로 비교 구간 단축 + 공통 접두 ops에 추가
    //  → 앵커와 연산 → 공통 접미 ops에 추가 → 원본 문자열 라인으로 복원
    static std::vector<Op> compute(const std::vector<NormalizedLine> &aNorm,
                                   const std::vector<NormalizedLine> &bNorm);

    // body와 rank로 정수화 + 원본 문자열 라인 저장.
    static int buildInternAndIdsRankBuckets(const std::vector<NormalizedLine> &aNorm,
                                            const std::vector<NormalizedLine> &bNorm,
                                            /*out*/ std::vector<int>          &Ai,
                                            /*out*/ std::vector<int>          &Bi,
                                            /*out*/ std::vector<int>          &origA,
                                            /*out*/ std::vector<int>          &origB);

    // 공통 접두/접미 트림 + 접두 SAME push.
    static void trimCommon(const std::vector<int>    &Ai,
                           const std::vector<int>    &Bi,
                           /*out*/ int               &a0,
                           /*out*/ int               &a1,
                           /*out*/ int               &b0,
                           /*out*/ int               &b1,
                           /*inout*/ std::vector<Op> &ops);

    // 앵커를 찾아 구간을 분할 및 해결.
    static void solveWithAnchors(const std::vector<int>            &Ai,
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
                                 /*inout*/ std::vector<Op>         &ops);

    // 유니크 라인(패이션스) 기반 앵커.
    static std::vector<std::pair<int, int>> buildAnchorsUnique(const std::vector<int> &Ai,
                                                               const std::vector<int> &Bi,
                                                               int                     x0,
                                                               int                     x1,
                                                               int                     y0,
                                                               int                     y1,
                                                               int                     vocabCap);

    // 히스토그램 기반 앵커.
    static std::vector<std::pair<int, int>>
    buildAnchorsHistogram(const std::vector<NormalizedLine> &aNorm,
                          const std::vector<NormalizedLine> &bNorm,
                          const std::vector<int>            &Ai,
                          const std::vector<int>            &Bi,
                          const std::vector<int>            &origA,
                          const std::vector<int>            &origB,
                          int                                x0,
                          int                                x1,
                          int                                y0,
                          int                                y1,
                          int                                vocabCap);

    // 앵커로 나온 순서 쌍 LIS 적용.
    static std::vector<std::pair<int, int>>
    extractAnchorsLIS(const std::vector<std::pair<int, int>> &pairs);

    // LCS DP 폴백 + 소구간 push.
    static void dpFallback(const std::vector<int>    &Ai,
                           const std::vector<int>    &Bi,
                           int                        ax,
                           int                        ay,
                           int                        bx,
                           int                        by,
                           /*inout*/ std::vector<Op> &ops);

    // 분할정복 Myers (선형 메모리) + 결과 push.
    static void dcMyers(const std::vector<int>    &Ai,
                        const std::vector<int>    &Bi,
                        int                        ax,
                        int                        ay,
                        int                        bx,
                        int                        by,
                        /*inout*/ std::vector<Op> &ops);

    // 접미 SAME push.
    static void appendSuffixSame(const std::vector<int>    &Ai,
                                 const std::vector<int>    &Bi,
                                 int                        a1,
                                 int                        b1,
                                 /*inout*/ std::vector<Op> &ops);

    // Diff.Kind를 문자열로 변환(enum -> string).
    static const char *kindToStr(Diff::Kind kind);

    // Diff 벡터로부터 JSON 변경사항과 통계를 작성.
    static void buildChangesJson(const std::vector<Diff> &diffs,
                                 /*out*/ Json::Value     &changes,
                                 /*out*/ DiffStats       &stats);

    // Op → Diff + DEL/ADD → MOD 접기.
    static std::vector<Diff> foldOpsToDiffs(const std::vector<Op>             &ops,
                                            const std::vector<NormalizedLine> &aNorm,
                                            const std::vector<NormalizedLine> &bNorm,
                                            const std::string                 &contentA,
                                            const std::string                 &contentB);
};
