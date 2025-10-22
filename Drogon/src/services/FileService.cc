#include "FileService.h"

#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace services;

const std::vector<std::string> FileService::supportedFormats_ = {
        ".zip", ".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tbz2", ".tar.xz", ".7z", ".rar"};

bool FileService::isSupportedArchive(const std::string &filename)
{
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (const auto &format : supportedFormats_) {
        if (lower.size() >= format.size() &&
            lower.compare(lower.size() - format.size(), format.size(), format) == 0) {
            return true;
        }
    }
    return false;
}

bool FileService::createDirectory(const std::string &path)
{
    try {
        return fs::create_directories(path);
    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to create directory: " << e.what();
        return false;
    }
}

bool FileService::removeFile(const std::string &path)
{
    try {
        return fs::remove(path);
    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to remove file: " << e.what();
        return false;
    }
}

bool FileService::removeDirectory(const std::string &path)
{
    try {
        return fs::remove_all(path) > 0;
    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to remove directory: " << e.what();
        return false;
    }
}

ExtractResult FileService::extractArchive(const std::string &archivePath,
                                          const std::string &extractTo)
{
    ExtractResult result;
    result.success = false;

    // 압축 형식 확인
    if (!isSupportedArchive(archivePath)) {
        result.errorMessage = "Unsupported archive format";
        return result;
    }

    // 파일 존재 확인
    if (!fs::exists(archivePath)) {
        result.errorMessage = "Archive file not found";
        return result;
    }

    // 압축 해제 디렉토리 생성
    if (!createDirectory(extractTo)) {
        result.errorMessage = "Failed to create extraction directory";
        return result;
    }

    struct archive       *a;
    struct archive       *ext;
    struct archive_entry *entry;
    int                   r;

    a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME);
    archive_write_disk_set_standard_lookup(ext);

    if ((r = archive_read_open_filename(a, archivePath.c_str(), 10240))) {
        result.errorMessage = "Failed to open archive: " + std::string(archive_error_string(a));
        archive_read_free(a);
        archive_write_free(ext);
        return result;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        // 추출 경로 설정
        std::string currentFile    = archive_entry_pathname(entry);
        std::string fullOutputPath = extractTo + "/" + currentFile;

        archive_entry_set_pathname(entry, fullOutputPath.c_str());

        LOG_DEBUG << "Extracting: " << currentFile << " to " << fullOutputPath;

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            LOG_WARN << "Write header failed: " << archive_error_string(ext);
        } else {
            if (archive_entry_size(entry) > 0) {
                const void *buff;
                size_t      size;
                int64_t     offset;

                while (true) {
                    r = archive_read_data_block(a, &buff, &size, &offset);
                    if (r == ARCHIVE_EOF) {
                        break;
                    }
                    if (r != ARCHIVE_OK) {
                        LOG_ERROR << "Read data failed: " << archive_error_string(a);
                        break;
                    }
                    r = archive_write_data_block(ext, buff, size, offset);
                    if (r != ARCHIVE_OK) {
                        LOG_ERROR << "Write data failed: " << archive_error_string(ext);
                        break;
                    }
                }
            }
        }

        archive_write_finish_entry(ext);
        result.extractedFiles.push_back(currentFile);
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    result.success     = true;
    result.extractPath = extractTo;

    LOG_INFO << "Extraction completed: " << result.extractedFiles.size() << " files extracted to "
             << extractTo;

    return result;
}
