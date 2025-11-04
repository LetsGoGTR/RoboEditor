#pragma once

#include <json/json.h>
#include <string>

#include "ControllerHelper.h"

namespace core
{
    class File
    {
      public:
        // 파일 읽기
        static helpers::CoreResult readFile(const std::string &path);

        // 파일 생성
        static helpers::CoreResult createFile(const std::string &path, const std::string &content);

        // 파일 업데이트 (내용 변경 또는 이동)
        static helpers::CoreResult updateFile(const std::string &path,
                                              const std::string &content = "",
                                              const std::string &newPath = "");

        // 파일 삭제
        static helpers::CoreResult deleteFile(const std::string &path);
    };
}  // namespace core
