#pragma once

#include <drogon/drogon.h>
#include <optional>
#include <string>
#include <vector>

using namespace drogon;

namespace services
{

    struct ExtractResult
    {
        bool                     success;
        std::string              extractPath;
        std::vector<std::string> extractedFiles;
        std::string              errorMessage;
    };

    class FileService
    {
      public:
        // 압축 파일 업로드 및 압축 해제
        static ExtractResult extractArchive(const std::string &archivePath,
                                            const std::string &extractTo);

        // 지원 압축 형식 확인
        static bool isSupportedArchive(const std::string &filename);

        // 디렉토리 생성
        static bool createDirectory(const std::string &path);

        // 파일 삭제
        static bool removeFile(const std::string &path);

        // 디렉토리 삭제
        static bool removeDirectory(const std::string &path);

      private:
        static const std::vector<std::string> supportedFormats_;
    };

}  // namespace services
