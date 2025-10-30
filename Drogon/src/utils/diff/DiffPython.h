#pragma once

#include <string>
#include <vector>

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
    std::string body;
    int rank;
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

private:
    // 한 줄을[start, endExcl) 정규화.
    static NormalizedLine normalizeOne(const std::string& content,
                                       size_t start,
                                       size_t endExcl,
                                       std::vector<int>& deep);

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
};
