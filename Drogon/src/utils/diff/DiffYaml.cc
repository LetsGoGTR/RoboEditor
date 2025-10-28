#include "DiffYaml.h"

// libyaml의 라인 번호를 안전하게 가져오는 헬퍼 함수
int diff_utils::getNodeLineNumber(const YAML::Node &node)
{
    if (node.IsNull())
        return -1;
    return node.Mark().line + diff_utils::Constants::LINE_NUMBER_OFFSET;
}

// DiffEntry의 toJson() 메서드 구현
Json::Value diff_utils::DiffEntry::toJson() const
{
    Json::Value result;
    result["type"]     = type;
    result["path"]     = path;
    result["oldValue"] = oldValue;
    result["newValue"] = newValue;

    if (oldLineNumber != -1) {
        result["oldLineNumber"] = oldLineNumber;
    } else {
        result["oldLineNumber"] = Json::Value::null;
    }

    if (newLineNumber != -1) {
        result["newLineNumber"] = newLineNumber;
    } else {
        result["newLineNumber"] = Json::Value::null;
    }

    result["oldLineCount"] = oldLineCount;
    result["newLineCount"] = newLineCount;

    return result;
}

// YAML 노드의 라인 수를 계산하는 독립 함수 (키 포함하여 정확한 라인 번호 계산)
int diff_utils::calculateLineCount(const YAML::Node &node, int startLine)
{
    if (node.IsNull())
        return 0;
    if (node.IsScalar())
        return 1;

    // 시작 라인이 제공되지 않으면 노드의 라인 번호를 사용
    if (startLine == -1) {
        startLine = getNodeLineNumber(node);
    }
    int maxLine = startLine;

    std::function<void(const YAML::Node &)> findMaxLine = [&](const YAML::Node &n) {
        if (n.IsNull())
            return;

        // libyaml의 Mark 정보에서 정확한 라인 번호를 가져옴
        // 이는 YAML 파일의 실제 라인 번호와 일치함
        int line = getNodeLineNumber(n);
        if (line != -1) {
            maxLine = std::max(maxLine, line);
        }

        if (n.IsMap()) {
            for (const auto &it : n) {
                findMaxLine(it.second);
            }
        } else if (n.IsSequence()) {
            for (std::size_t i = 0; i < n.size(); ++i) {
                findMaxLine(n[i]);
            }
        }
    };

    findMaxLine(node);
    return maxLine - startLine + 1;
}

// YAML 노드를 Json::Value로 재귀 변환 (라인 넘버로 정렬)
Json::Value diff_utils::nodeToJson(const YAML::Node &node)
{
    if (node.IsNull()) {
        return Json::Value::null;
    }
    if (node.IsScalar()) {
        try {
            if (node.Type() == YAML::NodeType::Scalar) {
                // 각 타입으로 변환을 시도해보고 성공하는 첫 번째 타입을 사용
                try {
                    // 불린 시도 (먼저 체크)
                    bool boolValue = node.as<bool>();
                    return Json::Value(boolValue);
                } catch (...) {
                    try {
                        // 정수 시도
                        int intValue = node.as<int>();
                        return Json::Value(intValue);
                    } catch (...) {
                        try {
                            // 실수 시도
                            double doubleValue = node.as<double>();
                            return Json::Value(doubleValue);
                        } catch (...) {
                            // 모든 변환 실패 시 문자열로
                            return Json::Value(node.as<std::string>());
                        }
                    }
                }
            }

            return Json::Value(node.as<std::string>());

        } catch (...) {
            return Json::Value(diff_utils::Constants::CONVERSION_ERROR);
        }
    }
    if (node.IsSequence()) {
        Json::Value arr(Json::arrayValue);
        for (std::size_t i = 0; i < node.size(); ++i) {
            arr.append(diff_utils::nodeToJson(node[i]));
        }
        return arr;
    }
    if (node.IsMap()) {
        return diff_utils::convertMapToJson(node);
    }
    return Json::Value(diff_utils::Constants::UNKNOWN_TYPE);
}

// Map을 JSON으로 변환하는 헬퍼 함수
Json::Value diff_utils::convertMapToJson(const YAML::Node &node)
{
    // 라인 넘버와 함께 키-값 쌍을 수집
    std::vector<KeyValueWithLine> keyValuePairs;
    for (const auto &it : node) {
        std::string key        = it.first.as<std::string>();
        int         lineNumber = getNodeLineNumber(it.first);
        keyValuePairs.emplace_back(key, it.second, lineNumber);
    }

    // 라인 넘버로 정렬
    std::sort(keyValuePairs.begin(),
              keyValuePairs.end(),
              [](const KeyValueWithLine &a, const KeyValueWithLine &b) {
                  return a.lineNumber < b.lineNumber;
              });

    // 정렬된 순서로 JSON 객체 생성
    Json::Value obj(Json::objectValue);
    for (const auto &kv : keyValuePairs) {
        obj[kv.key] = diff_utils::nodeToJson(kv.value);
    }
    return obj;
}

// 키의 라인 번호를 찾는 헬퍼 함수
int diff_utils::findKeyLineNumber(const YAML::Node &node, const std::string &key)
{
    for (auto it = node.begin(); it != node.end(); ++it) {
        if (it->first.as<std::string>() == key) {
            return diff_utils::getNodeLineNumber(it->first);
        }
    }
    return -1;
}

// 추가된 차이점을 생성하는 헬퍼 함수
void diff_utils::addAddedDiff(const YAML::Node                   &node,
                              const std::string                  &key,
                              const std::string                  &path,
                              std::vector<diff_utils::DiffEntry> &diffs)
{
    int keyLineNumber = diff_utils::findKeyLineNumber(node, key);
    // 키 라인부터 시작하여 전체 라인 수 계산
    int newLineCount = diff_utils::calculateLineCount(node[key], keyLineNumber);

    Json::Value addedItem(Json::objectValue);
    addedItem[key] = diff_utils::nodeToJson(node[key]);
    diffs.push_back(
            {"added", path, Json::Value::null, addedItem, -1, keyLineNumber, 0, newLineCount});
}

// 삭제된 차이점을 생성하는 헬퍼 함수
void diff_utils::addRemovedDiff(const YAML::Node                   &node,
                                const std::string                  &key,
                                const std::string                  &path,
                                std::vector<diff_utils::DiffEntry> &diffs)
{
    int keyLineNumber = diff_utils::findKeyLineNumber(node, key);
    // 키 라인부터 시작하여 전체 라인 수 계산
    int oldLineCount = diff_utils::calculateLineCount(node[key], keyLineNumber);

    Json::Value removedItem(Json::objectValue);
    removedItem[key] = diff_utils::nodeToJson(node[key]);
    diffs.push_back(
            {"removed", path, removedItem, Json::Value::null, keyLineNumber, -1, oldLineCount, 0});
}

// 수정된 차이점을 생성하는 헬퍼 함수
void diff_utils::addModifiedDiff(const YAML::Node                   &nodeA,
                                 const YAML::Node                   &nodeB,
                                 const std::string                  &path,
                                 std::vector<diff_utils::DiffEntry> &diffs)
{
    int oldLineCount = diff_utils::calculateLineCount(nodeA, -1);
    int newLineCount = diff_utils::calculateLineCount(nodeB, -1);
    diffs.push_back({"modified",
                     path,
                     diff_utils::nodeToJson(nodeA),
                     diff_utils::nodeToJson(nodeB),
                     diff_utils::getNodeLineNumber(nodeA),
                     diff_utils::getNodeLineNumber(nodeB),
                     oldLineCount,
                     newLineCount});
}

// 스칼라 값 비교 함수
void diff_utils::compareScalars(const YAML::Node                   &nodeA,
                                const YAML::Node                   &nodeB,
                                const std::string                  &path,
                                std::vector<diff_utils::DiffEntry> &diffs)
{
    Json::Value valA = diff_utils::nodeToJson(nodeA);
    Json::Value valB = diff_utils::nodeToJson(nodeB);
    if (valA != valB) {
        diffs.push_back({"modified",
                         path,
                         valA,
                         valB,
                         diff_utils::getNodeLineNumber(nodeA),
                         diff_utils::getNodeLineNumber(nodeB),
                         1,
                         1});
    }
}

// Map 비교 함수
void diff_utils::compareMaps(const YAML::Node                   &nodeA,
                             const YAML::Node                   &nodeB,
                             const std::string                  &path,
                             std::vector<diff_utils::DiffEntry> &diffs)
{
    std::set<std::string> keys;
    for (auto it = nodeA.begin(); it != nodeA.end(); ++it)
        keys.insert(it->first.as<std::string>());
    for (auto it = nodeB.begin(); it != nodeB.end(); ++it)
        keys.insert(it->first.as<std::string>());

    for (const auto &key : keys) {
        YAML::Node  valA        = nodeA[key];
        YAML::Node  valB        = nodeB[key];
        std::string currentPath = path.empty() ? key : path + "." + key;

        if (!valA) {
            diff_utils::addAddedDiff(nodeB, key, currentPath, diffs);
        } else if (!valB) {
            diff_utils::addRemovedDiff(nodeA, key, currentPath, diffs);
        } else {
            diff_utils::compareNodes(valA, valB, currentPath, diffs);
        }
    }
}

// Sequence 비교 함수
void diff_utils::compareSequences(const YAML::Node                   &nodeA,
                                  const YAML::Node                   &nodeB,
                                  const std::string                  &path,
                                  std::vector<diff_utils::DiffEntry> &diffs)
{
    size_t maxSize = std::max(nodeA.size(), nodeB.size());
    for (size_t i = 0; i < maxSize; ++i) {
        std::string currentPath = path + "[" + std::to_string(i) + "]";
        if (i >= nodeA.size()) {
            int newLineCount = diff_utils::calculateLineCount(nodeB[i], -1);
            diffs.push_back({"added",
                             currentPath,
                             Json::Value::null,
                             diff_utils::nodeToJson(nodeB[i]),
                             -1,
                             diff_utils::getNodeLineNumber(nodeB[i]),
                             0,
                             newLineCount});
        } else if (i >= nodeB.size()) {
            int oldLineCount = diff_utils::calculateLineCount(nodeA[i], -1);
            diffs.push_back({"removed",
                             currentPath,
                             diff_utils::nodeToJson(nodeA[i]),
                             Json::Value::null,
                             diff_utils::getNodeLineNumber(nodeA[i]),
                             -1,
                             oldLineCount,
                             0});
        } else {
            diff_utils::compareNodes(nodeA[i], nodeB[i], currentPath, diffs);
        }
    }
}

// 메인 비교 함수
void diff_utils::compareNodes(const YAML::Node                   &nodeA,
                              const YAML::Node                   &nodeB,
                              const std::string                  &path,
                              std::vector<diff_utils::DiffEntry> &diffs)
{
    if (nodeA.Type() != nodeB.Type()) {
        diff_utils::addModifiedDiff(nodeA, nodeB, path, diffs);
        return;
    }

    if (nodeA.IsScalar() && nodeB.IsScalar()) {
        diff_utils::compareScalars(nodeA, nodeB, path, diffs);
        return;
    }

    if (nodeA.IsMap() && nodeB.IsMap()) {
        diff_utils::compareMaps(nodeA, nodeB, path, diffs);
        return;
    }

    if (nodeA.IsSequence() && nodeB.IsSequence()) {
        diff_utils::compareSequences(nodeA, nodeB, path, diffs);
        return;
    }

    // 기타 타입 비교
    Json::Value valA = diff_utils::nodeToJson(nodeA);
    Json::Value valB = diff_utils::nodeToJson(nodeB);
    if (valA != valB) {
        diff_utils::addModifiedDiff(nodeA, nodeB, path, diffs);
    }
}

// 통계 계산 함수
Json::Value diff_utils::calculateStatistics(const std::vector<diff_utils::DiffEntry> &diffs,
                                            const std::string                        &fileType)
{
    int addedCount = 0, removedCount = 0, modifiedCount = 0;
    for (const auto &d : diffs) {
        if (d.type == "added")
            addedCount++;
        else if (d.type == "removed")
            removedCount++;
        else if (d.type == "modified")
            modifiedCount++;
    }

    Json::Value result(Json::objectValue);
    result["fileType"]     = fileType;
    result["added"]        = addedCount;
    result["removed"]      = removedCount;
    result["modified"]     = modifiedCount;
    result["totalChanges"] = diffs.size();

    return result;
}

// 결과 생성 함수
Json::Value diff_utils::generateResult(const std::vector<diff_utils::DiffEntry> &diffs,
                                       const std::string                        &fileType,
                                       const std::string                        &fileName1,
                                       const std::string                        &fileName2)
{
    Json::Value result(Json::objectValue);

    result["file1"]      = fileName1;
    result["file2"]      = fileName2;
    result["statistics"] = diff_utils::calculateStatistics(diffs, fileType);

    result["changes"] = Json::Value(Json::arrayValue);
    for (const auto &d : diffs) {
        result["changes"].append(d.toJson());
    }

    return result;
}

// 두 YAML 파일을 비교하여 차이점을 반환
Json::Value diff_utils::compareFiles(const std::string &file1, const std::string &file2)
{
    try {
        YAML::Node yaml1 = YAML::Load(file1);
        YAML::Node yaml2 = YAML::Load(file2);

        std::vector<diff_utils::DiffEntry> diffs;
        compareNodes(yaml1, yaml2, "", diffs);

        return generateResult(diffs, "yaml", file1, file2);

    } catch (const YAML::Exception &e) {
        std::cerr << diff_utils::Constants::YAML_ERROR_PREFIX << e.what() << std::endl;
        return Json::Value::null;
    } catch (const std::exception &e) {
        std::cerr << diff_utils::Constants::GENERAL_ERROR_PREFIX << e.what() << std::endl;
        return Json::Value::null;
    }
}

// DiffYaml 클래스의 static 메서드 구현
std::vector<diff_utils::DiffEntry> diff_utils::DiffYaml::compareFiles(const std::string &contentA,
                                                                      const std::string &contentB)
{
    std::vector<diff_utils::DiffEntry> diffs;

    try {
        YAML::Node yaml1 = YAML::Load(contentA);
        YAML::Node yaml2 = YAML::Load(contentB);

        diff_utils::compareNodes(yaml1, yaml2, "", diffs);

        return diffs;

    } catch (const YAML::Exception &e) {
        std::cerr << diff_utils::Constants::YAML_ERROR_PREFIX << e.what() << std::endl;
        return diffs;
    } catch (const std::exception &e) {
        std::cerr << diff_utils::Constants::GENERAL_ERROR_PREFIX << e.what() << std::endl;
        return diffs;
    }
}

Json::Value diff_utils::DiffYaml::generateResult(const std::vector<diff_utils::DiffEntry> &diffs,
                                                 const std::string                        &fileType,
                                                 const std::string &fileName1,
                                                 const std::string &fileName2)
{
    return diff_utils::generateResult(diffs, fileType, fileName1, fileName2);
}
