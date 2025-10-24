#include "DiffText.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
    // Split content into lines
    std::vector<std::string> splitLines(const std::string &content)
    {
        std::vector<std::string> lines;
        std::string              line;
        std::istringstream       stream(content);

        while (std::getline(stream, line)) {
            lines.push_back(line);
        }

        return lines;
    }

    // Longest Common Subsequence (LCS) based diff
    struct DiffOp
    {
        std::string type;  // "equal", "delete", "insert", "modified"
        int         oldIdx;
        int         newIdx;
        std::string oldLine;
        std::string newLine;
    };

    // Compute LCS length table
    std::vector<std::vector<int>> computeLCS(const std::vector<std::string> &linesA,
                                             const std::vector<std::string> &linesB)
    {
        int                           m = linesA.size();
        int                           n = linesB.size();
        std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));

        for (int i = 1; i <= m; i++) {
            for (int j = 1; j <= n; j++) {
                if (linesA[i - 1] == linesB[j - 1]) {
                    dp[i][j] = dp[i - 1][j - 1] + 1;
                } else {
                    dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
                }
            }
        }

        return dp;
    }

    // Backtrack through LCS table to get diff operations
    std::vector<DiffOp> backtrackLCS(const std::vector<std::vector<int>> &dp,
                                     const std::vector<std::string>      &linesA,
                                     const std::vector<std::string>      &linesB,
                                     int                                  i,
                                     int                                  j)
    {
        std::vector<DiffOp> ops;

        while (i > 0 || j > 0) {
            if (i > 0 && j > 0 && linesA[i - 1] == linesB[j - 1]) {
                // Lines are equal
                ops.push_back({"equal", i, j, linesA[i - 1], linesB[j - 1]});
                i--;
                j--;
            } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
                // Line was inserted
                ops.push_back({"insert", -1, j, "", linesB[j - 1]});
                j--;
            } else if (i > 0 && (j == 0 || dp[i][j - 1] < dp[i - 1][j])) {
                // Line was deleted
                ops.push_back({"delete", i, -1, linesA[i - 1], ""});
                i--;
            }
        }

        std::reverse(ops.begin(), ops.end());
        return ops;
    }

    std::vector<DiffOp> computeDiff(const std::vector<std::string> &linesA,
                                    const std::vector<std::string> &linesB)
    {
        auto dp = computeLCS(linesA, linesB);
        return backtrackLCS(dp, linesA, linesB, linesA.size(), linesB.size());
    }

}  // anonymous namespace

Json::Value diff_utils::TextDiffEntry::toJson() const
{
    Json::Value result(Json::objectValue);
    result["type"]       = type;
    result["lineNumber"] = lineNumber;
    result["oldLine"]    = oldLine;
    result["newLine"]    = newLine;
    return result;
}

std::vector<diff_utils::TextDiffEntry> diff_utils::DiffText::compareFiles(const std::string &contentA,
                                                                           const std::string &contentB)
{
    auto linesA = splitLines(contentA);
    auto linesB = splitLines(contentB);

    auto diffOps = computeDiff(linesA, linesB);

    std::vector<TextDiffEntry> diffs;

    for (const auto &op : diffOps) {
        if (op.type == "equal") {
            continue;  // Skip equal lines
        }

        diff_utils::TextDiffEntry entry;
        if (op.type == "insert") {
            entry.type       = "added";
            entry.lineNumber = op.newIdx;
            entry.oldLine    = "";
            entry.newLine    = op.newLine;
        } else if (op.type == "delete") {
            entry.type       = "removed";
            entry.lineNumber = op.oldIdx;
            entry.oldLine    = op.oldLine;
            entry.newLine    = "";
        } else {
            entry.type       = "modified";
            entry.lineNumber = op.oldIdx;
            entry.oldLine    = op.oldLine;
            entry.newLine    = op.newLine;
        }

        diffs.push_back(entry);
    }

    return diffs;
}

Json::Value diff_utils::DiffText::generateResult(const std::vector<diff_utils::TextDiffEntry> &diffs,
                                                 const std::string                            &nameA,
                                                 const std::string                            &nameB)
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
    statistics["fileType"]     = "text";
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
