#include "DiffYaml.h"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <set>
#include <yaml-cpp/yaml.h>

using namespace diff_utils;

namespace
{
    // Constants
    constexpr const char *CONVERSION_ERROR   = "[conversion_error]";
    constexpr const char *UNKNOWN_TYPE       = "[unknown]";
    constexpr int         LINE_NUMBER_OFFSET = 1;

    // Helper structure for line number tracking
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

    // Forward declarations
    Json::Value convertMapToJson(const YAML::Node &node);
    void        compareNodes(const YAML::Node       &nodeA,
                             const YAML::Node       &nodeB,
                             const std::string      &path,
                             std::vector<DiffEntry> &diffs);

    // Calculate line count for a YAML node
    int calculateLineCount(const YAML::Node &node)
    {
        if (node.IsNull())
            return 0;
        if (node.IsScalar())
            return 1;

        int startLine = node.Mark().line;
        int maxLine   = startLine;

        std::function<void(const YAML::Node &)> findMaxLine = [&](const YAML::Node &n) {
            if (n.IsNull())
                return;

            int line = n.Mark().line + LINE_NUMBER_OFFSET;
            maxLine  = std::max(maxLine, line);

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

    // Convert YAML node to JSON
    Json::Value nodeToJson(const YAML::Node &node)
    {
        if (node.IsNull()) {
            return Json::Value(Json::nullValue);
        }
        if (node.IsScalar()) {
            try {
                return Json::Value(node.as<std::string>());
            } catch (...) {
                return Json::Value(CONVERSION_ERROR);
            }
        }
        if (node.IsSequence()) {
            Json::Value arr(Json::arrayValue);
            for (std::size_t i = 0; i < node.size(); ++i) {
                arr.append(nodeToJson(node[i]));
            }
            return arr;
        }
        if (node.IsMap()) {
            return convertMapToJson(node);
        }
        return Json::Value(UNKNOWN_TYPE);
    }

    // Convert map to JSON (sorted by line number)
    Json::Value convertMapToJson(const YAML::Node &node)
    {
        std::vector<KeyValueWithLine> keyValuePairs;
        for (const auto &it : node) {
            std::string key        = it.first.as<std::string>();
            int         lineNumber = it.first.Mark().line + LINE_NUMBER_OFFSET;
            keyValuePairs.emplace_back(key, it.second, lineNumber);
        }

        std::sort(keyValuePairs.begin(),
                  keyValuePairs.end(),
                  [](const KeyValueWithLine &a, const KeyValueWithLine &b) {
                      return a.lineNumber < b.lineNumber;
                  });

        Json::Value obj(Json::objectValue);
        for (const auto &kv : keyValuePairs) {
            obj[kv.key] = nodeToJson(kv.value);
        }
        return obj;
    }

    // Find line number of a key
    int findKeyLineNumber(const YAML::Node &node, const std::string &key)
    {
        for (auto it = node.begin(); it != node.end(); ++it) {
            if (it->first.as<std::string>() == key) {
                return it->first.Mark().line + LINE_NUMBER_OFFSET;
            }
        }
        return -1;
    }

    // Helper functions for adding diffs
    void addAddedDiff(const YAML::Node       &node,
                      const std::string      &key,
                      const std::string      &path,
                      std::vector<DiffEntry> &diffs)
    {
        int         lineNumber   = findKeyLineNumber(node, key);
        int         newLineCount = calculateLineCount(node[key]);
        Json::Value addedItem(Json::objectValue);
        addedItem[key] = nodeToJson(node[key]);

        DiffEntry entry;
        entry.type          = "added";
        entry.path          = path;
        entry.oldValue      = Json::Value(Json::nullValue);
        entry.newValue      = addedItem;
        entry.oldLineNumber = -1;
        entry.newLineNumber = lineNumber;
        entry.oldLineCount  = 0;
        entry.newLineCount  = newLineCount;
        diffs.push_back(entry);
    }

    void addRemovedDiff(const YAML::Node       &node,
                        const std::string      &key,
                        const std::string      &path,
                        std::vector<DiffEntry> &diffs)
    {
        int         oldLineCount = calculateLineCount(node[key]);
        int         lineNumber   = findKeyLineNumber(node, key);
        Json::Value removedItem(Json::objectValue);
        removedItem[key] = nodeToJson(node[key]);

        DiffEntry entry;
        entry.type          = "removed";
        entry.path          = path;
        entry.oldValue      = removedItem;
        entry.newValue      = Json::Value(Json::nullValue);
        entry.oldLineNumber = lineNumber;
        entry.newLineNumber = -1;
        entry.oldLineCount  = oldLineCount;
        entry.newLineCount  = 0;
        diffs.push_back(entry);
    }

    void addModifiedDiff(const YAML::Node       &nodeA,
                         const YAML::Node       &nodeB,
                         const std::string      &path,
                         std::vector<DiffEntry> &diffs)
    {
        int oldLineCount = calculateLineCount(nodeA);
        int newLineCount = calculateLineCount(nodeB);

        DiffEntry entry;
        entry.type          = "modified";
        entry.path          = path;
        entry.oldValue      = nodeToJson(nodeA);
        entry.newValue      = nodeToJson(nodeB);
        entry.oldLineNumber = nodeA.Mark().line + LINE_NUMBER_OFFSET;
        entry.newLineNumber = nodeB.Mark().line + LINE_NUMBER_OFFSET;
        entry.oldLineCount  = oldLineCount;
        entry.newLineCount  = newLineCount;
        diffs.push_back(entry);
    }

    // Comparison functions
    void compareScalars(const YAML::Node       &nodeA,
                        const YAML::Node       &nodeB,
                        const std::string      &path,
                        std::vector<DiffEntry> &diffs)
    {
        Json::Value valA = nodeToJson(nodeA);
        Json::Value valB = nodeToJson(nodeB);
        if (valA != valB) {
            DiffEntry entry;
            entry.type          = "modified";
            entry.path          = path;
            entry.oldValue      = valA;
            entry.newValue      = valB;
            entry.oldLineNumber = nodeA.Mark().line + LINE_NUMBER_OFFSET;
            entry.newLineNumber = nodeB.Mark().line + LINE_NUMBER_OFFSET;
            entry.oldLineCount  = 1;
            entry.newLineCount  = 1;
            diffs.push_back(entry);
        }
    }

    void compareMaps(const YAML::Node       &nodeA,
                     const YAML::Node       &nodeB,
                     const std::string      &path,
                     std::vector<DiffEntry> &diffs)
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
                addAddedDiff(nodeB, key, currentPath, diffs);
            } else if (!valB) {
                addRemovedDiff(nodeA, key, currentPath, diffs);
            } else {
                compareNodes(valA, valB, currentPath, diffs);
            }
        }
    }

    void compareSequences(const YAML::Node       &nodeA,
                          const YAML::Node       &nodeB,
                          const std::string      &path,
                          std::vector<DiffEntry> &diffs)
    {
        size_t maxSize = std::max(nodeA.size(), nodeB.size());
        for (size_t i = 0; i < maxSize; ++i) {
            std::string currentPath = path + "[" + std::to_string(i) + "]";
            if (i >= nodeA.size()) {
                int newLineCount = calculateLineCount(nodeB[i]);

                DiffEntry entry;
                entry.type          = "added";
                entry.path          = currentPath;
                entry.oldValue      = Json::Value(Json::nullValue);
                entry.newValue      = nodeToJson(nodeB[i]);
                entry.oldLineNumber = -1;
                entry.newLineNumber = nodeB[i].Mark().line + LINE_NUMBER_OFFSET;
                entry.oldLineCount  = 0;
                entry.newLineCount  = newLineCount;
                diffs.push_back(entry);
            } else if (i >= nodeB.size()) {
                int oldLineCount = calculateLineCount(nodeA[i]);

                DiffEntry entry;
                entry.type          = "removed";
                entry.path          = currentPath;
                entry.oldValue      = nodeToJson(nodeA[i]);
                entry.newValue      = Json::Value(Json::nullValue);
                entry.oldLineNumber = nodeA[i].Mark().line + LINE_NUMBER_OFFSET;
                entry.newLineNumber = -1;
                entry.oldLineCount  = oldLineCount;
                entry.newLineCount  = 0;
                diffs.push_back(entry);
            } else {
                compareNodes(nodeA[i], nodeB[i], currentPath, diffs);
            }
        }
    }

    void compareNodes(const YAML::Node       &nodeA,
                      const YAML::Node       &nodeB,
                      const std::string      &path,
                      std::vector<DiffEntry> &diffs)
    {
        if (nodeA.Type() != nodeB.Type()) {
            addModifiedDiff(nodeA, nodeB, path, diffs);
            return;
        }

        if (nodeA.IsScalar() && nodeB.IsScalar()) {
            compareScalars(nodeA, nodeB, path, diffs);
            return;
        }

        if (nodeA.IsMap() && nodeB.IsMap()) {
            compareMaps(nodeA, nodeB, path, diffs);
            return;
        }

        if (nodeA.IsSequence() && nodeB.IsSequence()) {
            compareSequences(nodeA, nodeB, path, diffs);
            return;
        }

        Json::Value valA = nodeToJson(nodeA);
        Json::Value valB = nodeToJson(nodeB);
        if (valA != valB) {
            addModifiedDiff(nodeA, nodeB, path, diffs);
        }
    }

}  // anonymous namespace

Json::Value DiffEntry::toJson() const
{
    Json::Value result(Json::objectValue);
    result["type"]     = type;
    result["path"]     = path;
    result["oldValue"] = oldValue;
    result["newValue"] = newValue;

    if (oldLineNumber != -1)
        result["oldLineNumber"] = oldLineNumber;
    else
        result["oldLineNumber"] = Json::Value(Json::nullValue);

    if (newLineNumber != -1)
        result["newLineNumber"] = newLineNumber;
    else
        result["newLineNumber"] = Json::Value(Json::nullValue);

    result["oldLineCount"] = oldLineCount;
    result["newLineCount"] = newLineCount;

    return result;
}

std::vector<DiffEntry> DiffYaml::compareFiles(const std::string &contentA,
                                              const std::string &contentB)
{
    YAML::Node yaml1 = YAML::Load(contentA);
    YAML::Node yaml2 = YAML::Load(contentB);

    std::vector<DiffEntry> diffs;
    compareNodes(yaml1, yaml2, "", diffs);

    return diffs;
}

Json::Value DiffYaml::generateResult(const std::vector<DiffEntry> &diffs,
                                     const std::string            &nameA,
                                     const std::string            &nameB)
{
    Json::Value result(Json::objectValue);

    // File info
    Json::Value file1Info(Json::objectValue);
    file1Info["name"] = nameA;

    Json::Value file2Info(Json::objectValue);
    file2Info["name"] = nameB;

    result["file1"] = file1Info;
    result["file2"] = file2Info;

    // Statistics
    int addedCount    = 0;
    int removedCount  = 0;
    int modifiedCount = 0;

    for (const auto &d : diffs) {
        if (d.type == "added")
            addedCount++;
        else if (d.type == "removed")
            removedCount++;
        else if (d.type == "modified")
            modifiedCount++;
    }

    Json::Value statistics(Json::objectValue);
    statistics["fileType"]     = "yaml";
    statistics["added"]        = addedCount;
    statistics["removed"]      = removedCount;
    statistics["modified"]     = modifiedCount;
    statistics["totalChanges"] = (int)diffs.size();

    result["statistics"] = statistics;

    // Changes
    Json::Value changes(Json::arrayValue);
    for (const auto &d : diffs) {
        changes.append(d.toJson());
    }
    result["changes"] = changes;

    return result;
}
