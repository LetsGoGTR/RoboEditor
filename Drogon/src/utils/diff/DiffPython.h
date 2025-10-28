#pragma once

#include <drogon/drogon.h>
#include <string>
#include <vector>

namespace diff_utils
{

    struct PythonDiffEntry
    {
        std::string type;        // "added", "removed", "modified"
        int         lineNumber;  // Line number in original file
        std::string oldContent;  // Original line content
        std::string newContent;  // New line content
        std::string context;     // Function/class context if available

        Json::Value toJson() const;
    };

    class DiffPython
    {
      public:
        static std::vector<PythonDiffEntry> compareFiles(const std::string &contentA,
                                                         const std::string &contentB);
        static Json::Value generateResult(const std::vector<PythonDiffEntry> &diffs,
                                          const std::string                  &nameA,
                                          const std::string                  &nameB);
    };

}  // namespace diff_utils