#pragma once

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <iostream>
#include <json/json.h>
#include <set>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace diff_utils
{
    // 상수 정의
    namespace Constants
    {
        constexpr const char *CONVERSION_ERROR     = "[conversion_error]";
        constexpr const char *UNKNOWN_TYPE         = "[unknown]";
        constexpr const char *YAML_ERROR_PREFIX    = "YAML error: ";
        constexpr const char *GENERAL_ERROR_PREFIX = "Error: ";
        constexpr int         LINE_NUMBER_OFFSET   = 1;
    }  // namespace Constants

    // libyaml의 라인 번호를 안전하게 가져오는 헬퍼 함수
    int getNodeLineNumber(const YAML::Node &node);

    // 라인 넘버와 함께 키-값 쌍을 저장하는 구조체
    struct KeyValueWithLine
    {
        std::string key;
        YAML::Node  value;
        int         lineNumber;

        KeyValueWithLine(const std::string &k, const YAML::Node &v, int line) :
            key(k),
            value(v),
            lineNumber(line)
        {
        }
    };

    // 차이점 저장 구조체
    struct DiffEntry
    {
        std::string type;  // "added", "removed", "modified"
        std::string path;
        Json::Value oldValue;
        Json::Value newValue;
        int         oldLineNumber = -1;
        int         newLineNumber = -1;
        int         oldLineCount  = 0;  // 이전 값의 라인 수
        int         newLineCount  = 0;  // 새 값의 라인 수

        // 기본 생성자
        DiffEntry() = default;

        // 매개변수 생성자
        DiffEntry(const std::string &t,
                  const std::string &p,
                  const Json::Value &old,
                  const Json::Value &newVal,
                  int                oldLine,
                  int                newLine,
                  int                oldCount,
                  int                newCount) :
            type(t),
            path(p),
            oldValue(old),
            newValue(newVal),
            oldLineNumber(oldLine),
            newLineNumber(newLine),
            oldLineCount(oldCount),
            newLineCount(newCount)
        {
        }

        Json::Value toJson() const;
    };

    // 두 YAML 노드를 비교하는 독립 함수
    void compareNodes(const YAML::Node       &nodeA,
                      const YAML::Node       &nodeB,
                      const std::string      &path,
                      std::vector<DiffEntry> &diffs);

    // YAML 노드의 라인 수를 계산하는 독립 함수
    int calculateLineCount(const YAML::Node &node, int startLine = -1);

    // YAML 노드를 Json::Value로 변환하는 독립 함수
    Json::Value nodeToJson(const YAML::Node &node);

    // Map을 JSON으로 변환하는 독립 함수
    Json::Value convertMapToJson(const YAML::Node &node);

    // 키의 라인 번호를 찾는 독립 함수
    int findKeyLineNumber(const YAML::Node &node, const std::string &key);

    // 추가된 차이점을 생성하는 독립 함수
    void addAddedDiff(const YAML::Node       &node,
                      const std::string      &key,
                      const std::string      &path,
                      std::vector<DiffEntry> &diffs);

    // 삭제된 차이점을 생성하는 독립 함수
    void addRemovedDiff(const YAML::Node       &node,
                        const std::string      &key,
                        const std::string      &path,
                        std::vector<DiffEntry> &diffs);

    // 수정된 차이점을 생성하는 독립 함수
    void addModifiedDiff(const YAML::Node       &nodeA,
                         const YAML::Node       &nodeB,
                         const std::string      &path,
                         std::vector<DiffEntry> &diffs);

    // 스칼라 값 비교 독립 함수
    void compareScalars(const YAML::Node       &nodeA,
                        const YAML::Node       &nodeB,
                        const std::string      &path,
                        std::vector<DiffEntry> &diffs);

    // Map 비교 독립 함수
    void compareMaps(const YAML::Node       &nodeA,
                     const YAML::Node       &nodeB,
                     const std::string      &path,
                     std::vector<DiffEntry> &diffs);

    // Sequence 비교 독립 함수
    void compareSequences(const YAML::Node       &nodeA,
                          const YAML::Node       &nodeB,
                          const std::string      &path,
                          std::vector<DiffEntry> &diffs);

    // 통계 계산 독립 함수
    Json::Value calculateStatistics(const std::vector<DiffEntry> &diffs,
                                    const std::string            &fileType);

    // 결과 생성 독립 함수
    Json::Value generateResult(const std::vector<DiffEntry> &diffs,
                               const std::string            &fileType,
                               const std::string            &fileName1,
                               const std::string            &fileName2);

    // DiffYaml 클래스 정의
    class DiffYaml
    {
      public:
        // 두 YAML 파일을 비교하여 차이점을 반환하는 static 메서드
        static std::vector<DiffEntry> compareFiles(const std::string &contentA,
                                                   const std::string &contentB);

        // 결과 생성 static 메서드
        static Json::Value generateResult(const std::vector<DiffEntry> &diffs,
                                          const std::string            &fileType,
                                          const std::string            &fileName1,
                                          const std::string            &fileName2);
    };

    // 기존 독립 함수들 (하위 호환성을 위해 유지)
    Json::Value compareFiles(const std::string &file1, const std::string &file2);

    Json::Value fileToJson(const std::string &content);

}  // namespace diff_utils

