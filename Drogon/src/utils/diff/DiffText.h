#pragma once

#include <drogon/drogon.h>
#include <string>
#include <vector>

namespace diff_utils
{

    struct TextDiffEntry
    {
        std::string type;        // "added", "removed", "modified"
        int         lineNumber;  // Line number in original file
        std::string oldLine;     // Original line content
        std::string newLine;     // New line content

        Json::Value toJson() const;
    };

    class DiffText
    {
      public:
        static std::vector<TextDiffEntry> compareFiles(const std::string &contentA,
                                                        const std::string &contentB);
        static Json::Value                generateResult(const std::vector<TextDiffEntry> &diffs,
                                                          const std::string                &nameA,
                                                          const std::string                &nameB);
    };

}  // namespace diff_utils