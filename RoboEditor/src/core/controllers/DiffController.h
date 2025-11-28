#pragma once

#include <json/json.h>
#include <string>

#include "ControllerHelper.h"

namespace core
{
    class Diff
    {
      public:
        // 두 파일 간의 diff 수행
        static helpers::CoreResult diffFiles(const std::string &filePathA,
                                             const std::string &filePathB);

        // 두 워크스페이스 간의 diff 수행
        static helpers::CoreResult diffWorkspaces(const std::string &dirPathA,
                                                  const std::string &dirPathB);
    };
}  // namespace core
