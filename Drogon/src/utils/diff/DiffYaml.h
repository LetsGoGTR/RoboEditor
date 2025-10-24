#pragma once

#include <drogon/drogon.h>
#include <string>
#include <vector>

namespace diff_utils
{

    struct DiffEntry
    {
        std::string type;  // "added", "removed", "modified"
        std::string path;
        Json::Value oldValue;
        Json::Value newValue;
        int         oldLineNumber = -1;
        int         newLineNumber = -1;
        int         oldLineCount  = 0;
        int         newLineCount  = 0;

        Json::Value toJson() const;
    };

    class DiffYaml
    {
      public:
        static std::vector<DiffEntry> compareFiles(const std::string &contentA,
                                                    const std::string &contentB);
        static Json::Value            generateResult(const std::vector<DiffEntry> &diffs,
                                                      const std::string            &nameA,
                                                      const std::string            &nameB);
    };

}  // namespace diff_utils
