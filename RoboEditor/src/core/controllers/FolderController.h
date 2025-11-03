#pragma once

#include <json/json.h>
#include <string>

#include "ControllerHelper.h"

namespace core
{
    class Folder
    {
      public:
        // 폴더 내용 읽기
        static helpers::CoreResult readFolder(const std::string &path);

        // 폴더 생성
        static helpers::CoreResult createFolder(const std::string &path);

        // 폴더 이름 변경/이동
        static helpers::CoreResult updateFolder(const std::string &path,
                                                const std::string &newPath);

        // 폴더 삭제
        static helpers::CoreResult deleteFolder(const std::string &path);
    };
}  // namespace core
