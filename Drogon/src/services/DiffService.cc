#include "DiffService.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "../utils/diff/DiffPython.h"
#include "../utils/diff/DiffText.h"
#include "../utils/diff/DiffYaml.h"

namespace fs = std::filesystem;

const std::vector<std::string> services::DiffService::supportedFormats_ = {
        ".yaml", ".yml", ".srl", ".py", ".txt", ".log", ".cfg", ".conf", ".ini", ".md", ".json"};

std::string services::DiffService::detectFileType(const std::string &fileName)
{
    std::string ext = fs::path(fileName).extension().string();

    // Convert to lowercase for case-insensitive comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".yaml" || ext == ".yml") {
        return "yaml";
    } else if (ext == ".srl" || ext == ".py") {
        return "python";
    } else {
        // Default to text-based diff for other formats
        return "text";
    }
}

bool services::DiffService::isSupportedFormat(const std::string &fileName)
{
    std::string ext = fs::path(fileName).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return std::find(supportedFormats_.begin(), supportedFormats_.end(), ext) !=
           supportedFormats_.end();
}

services::DiffResult services::DiffService::diffYaml(const std::string &contentA,
                                                     const std::string &contentB,
                                                     const std::string &nameA,
                                                     const std::string &nameB)
{
    services::DiffResult result;
    try {
        auto diffs     = diff_utils::DiffYaml::compareFiles(contentA, contentB);
        result.data    = diff_utils::DiffYaml::generateResult(diffs, nameA, nameB);
        result.success = true;
    } catch (const std::exception &e) {
        result.success      = false;
        result.errorMessage = "YAML diff error: " + std::string(e.what());
        LOG_ERROR << "YAML diff failed: " << e.what();
    }
    return result;
}

services::DiffResult services::DiffService::diffPython(const std::string &contentA,
                                                       const std::string &contentB,
                                                       const std::string &nameA,
                                                       const std::string &nameB)
{
    services::DiffResult result;
    try {
        auto diffs     = diff_utils::DiffPython::compareFiles(contentA, contentB);
        result.data    = diff_utils::DiffPython::generateResult(diffs, nameA, nameB);
        result.success = true;
    } catch (const std::exception &e) {
        result.success      = false;
        result.errorMessage = "Python diff error: " + std::string(e.what());
        LOG_ERROR << "Python diff failed: " << e.what();
    }
    return result;
}

services::DiffResult services::DiffService::diffText(const std::string &contentA,
                                                     const std::string &contentB,
                                                     const std::string &nameA,
                                                     const std::string &nameB)
{
    services::DiffResult result;
    try {
        auto diffs     = diff_utils::DiffText::compareFiles(contentA, contentB);
        result.data    = diff_utils::DiffText::generateResult(diffs, nameA, nameB);
        result.success = true;
    } catch (const std::exception &e) {
        result.success      = false;
        result.errorMessage = "Text diff error: " + std::string(e.what());
        LOG_ERROR << "Text diff failed: " << e.what();
    }
    return result;
}

services::DiffResult services::DiffService::diff(const std::string &contentA,
                                                 const std::string &contentB,
                                                 const std::string &nameA,
                                                 const std::string &nameB)
{
    services::DiffResult result;
    result.success = false;

    // Validate content is not empty
    if (contentA.empty()) {
        result.errorMessage = "Content A is empty";
        LOG_ERROR << result.errorMessage;
        return result;
    }

    if (contentB.empty()) {
        result.errorMessage = "Content B is empty";
        LOG_ERROR << result.errorMessage;
        return result;
    }

    // Validate both files have same extension
    std::string extA = fs::path(nameA).extension().string();
    std::string extB = fs::path(nameB).extension().string();
    std::transform(extA.begin(), extA.end(), extA.begin(), ::tolower);
    std::transform(extB.begin(), extB.end(), extB.begin(), ::tolower);

    if (extA != extB) {
        result.errorMessage = "File extensions do not match: " + extA + " vs " + extB;
        LOG_WARN << result.errorMessage;
        // Continue anyway, but warn the user
    }

    // Check if format is supported
    if (!isSupportedFormat(nameA)) {
        result.errorMessage = "Unsupported file format: " + extA;
        LOG_ERROR << result.errorMessage;
        return result;
    }

    // Detect file type and route to appropriate handler
    std::string fileType = detectFileType(nameA);

    LOG_INFO << "Diffing files with type: " << fileType;
    LOG_INFO << "File A: " << nameA;
    LOG_INFO << "File B: " << nameB;

    if (fileType == "yaml") {
        return diffYaml(contentA, contentB, nameA, nameB);
    } else if (fileType == "python") {
        return diffPython(contentA, contentB, nameA, nameB);
    } else if (fileType == "text") {
        return diffText(contentA, contentB, nameA, nameB);
    } else {
        result.errorMessage = "Unknown file type: " + fileType;
        LOG_ERROR << result.errorMessage;
        return result;
    }
}
