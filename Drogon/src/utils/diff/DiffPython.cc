#include "DiffPython.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <vector>

using namespace diff_utils;

namespace
{
    // Structure to hold line with context information
    struct LineWithContext
    {
        std::string content;
        int         lineNumber;
        std::string context;  // Current function/class name

        LineWithContext(const std::string &c, int line, const std::string &ctx) :
            content(c),
            lineNumber(line),
            context(ctx)
        {
        }
    };

    // Parse content and extract lines with context information
    std::vector<LineWithContext> parsePythonContent(const std::string &content)
    {
        std::istringstream           stream(content);
        std::vector<LineWithContext> lines;
        std::string                  line;
        int                          lineNumber = 1;
        std::string                  currentContext;

        // Regular expressions to detect function and class definitions
        std::regex  classRegex(R"(^\s*class\s+(\w+))");
        std::regex  funcRegex(R"(^\s*def\s+(\w+))");
        std::smatch match;

        while (std::getline(stream, line)) {
            // Update context if we find a class or function definition
            if (std::regex_search(line, match, classRegex)) {
                currentContext = "class " + match[1].str();
            } else if (std::regex_search(line, match, funcRegex)) {
                std::string funcName = match[1].str();
                // If we're already in a class, append the function name
                if (currentContext.find("class ") == 0) {
                    currentContext = currentContext + "." + funcName;
                } else {
                    currentContext = "function " + funcName;
                }
            }

            lines.emplace_back(line, lineNumber, currentContext);
            lineNumber++;
        }

        return lines;
    }

    // Simple diff algorithm (Myers' diff algorithm - simplified version)
    struct DiffOp
    {
        std::string type;  // "equal", "delete", "insert"
        int         oldIdx;
        int         newIdx;
        std::string oldLine;
        std::string newLine;
        std::string context;
    };

    std::vector<DiffOp> computeDiff(const std::vector<LineWithContext> &linesA,
                                    const std::vector<LineWithContext> &linesB)
    {
        std::vector<DiffOp> ops;

        // Simple line-by-line comparison (can be enhanced with LCS algorithm)
        size_t i = 0, j = 0;

        while (i < linesA.size() || j < linesB.size()) {
            if (i >= linesA.size()) {
                // Remaining lines in B are additions
                ops.push_back({"insert", -1, (int)j + 1, "", linesB[j].content, linesB[j].context});
                j++;
            } else if (j >= linesB.size()) {
                // Remaining lines in A are deletions
                ops.push_back({"delete", (int)i + 1, -1, linesA[i].content, "", linesA[i].context});
                i++;
            } else if (linesA[i].content == linesB[j].content) {
                // Lines are equal
                ops.push_back({"equal",
                               (int)i + 1,
                               (int)j + 1,
                               linesA[i].content,
                               linesB[j].content,
                               linesA[i].context});
                i++;
                j++;
            } else {
                // Check if this is a modification or insert/delete
                // Look ahead to see if we can find a match
                bool foundMatch = false;

                // Look ahead in B for current A line
                for (size_t k = j + 1; k < std::min(j + 5, linesB.size()); k++) {
                    if (linesA[i].content == linesB[k].content) {
                        // Found a match - lines between are insertions
                        while (j < k) {
                            ops.push_back({"insert",
                                           -1,
                                           (int)j + 1,
                                           "",
                                           linesB[j].content,
                                           linesB[j].context});
                            j++;
                        }
                        foundMatch = true;
                        break;
                    }
                }

                if (!foundMatch) {
                    // Look ahead in A for current B line
                    for (size_t k = i + 1; k < std::min(i + 5, linesA.size()); k++) {
                        if (linesA[k].content == linesB[j].content) {
                            // Found a match - lines between are deletions
                            while (i < k) {
                                ops.push_back({"delete",
                                               (int)i + 1,
                                               -1,
                                               linesA[i].content,
                                               "",
                                               linesA[i].context});
                                i++;
                            }
                            foundMatch = true;
                            break;
                        }
                    }
                }

                if (!foundMatch) {
                    // This is a modification
                    ops.push_back({"modified",
                                   (int)i + 1,
                                   (int)j + 1,
                                   linesA[i].content,
                                   linesB[j].content,
                                   linesA[i].context});
                    i++;
                    j++;
                }
            }
        }

        return ops;
    }

}  // anonymous namespace

Json::Value PythonDiffEntry::toJson() const
{
    Json::Value result(Json::objectValue);
    result["type"]       = type;
    result["lineNumber"] = lineNumber;
    result["oldContent"] = oldContent;
    result["newContent"] = newContent;
    result["context"]    = context;
    return result;
}

std::vector<PythonDiffEntry> DiffPython::compareFiles(const std::string &contentA,
                                                      const std::string &contentB)
{
    auto linesA = parsePythonContent(contentA);
    auto linesB = parsePythonContent(contentB);

    auto diffOps = computeDiff(linesA, linesB);

    std::vector<PythonDiffEntry> diffs;

    for (const auto &op : diffOps) {
        if (op.type == "equal") {
            continue;  // Skip equal lines
        }

        PythonDiffEntry entry;
        if (op.type == "insert") {
            entry.type       = "added";
            entry.lineNumber = op.newIdx;
            entry.oldContent = "";
            entry.newContent = op.newLine;
            entry.context    = op.context;
        } else if (op.type == "delete") {
            entry.type       = "removed";
            entry.lineNumber = op.oldIdx;
            entry.oldContent = op.oldLine;
            entry.newContent = "";
            entry.context    = op.context;
        } else {  // modified
            entry.type       = "modified";
            entry.lineNumber = op.oldIdx;
            entry.oldContent = op.oldLine;
            entry.newContent = op.newLine;
            entry.context    = op.context;
        }

        diffs.push_back(entry);
    }

    return diffs;
}

Json::Value DiffPython::generateResult(const std::vector<PythonDiffEntry> &diffs,
                                       const std::string                  &nameA,
                                       const std::string                  &nameB)
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
    statistics["fileType"]     = "python";
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
