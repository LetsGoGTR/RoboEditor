#include "DiffService.h"

#include "../utils/diff/DiffPython.h"
#include "../utils/diff/DiffText.h"
#include "../utils/diff/DiffYaml.h"
#include "../utils/diff/DiffTree.h"
#include <drogon/drogon.h>

#include <algorithm>
#include <filesystem>
#include <fstream>


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


services::ServiceResult services::DiffService::diffYaml(const std::string &contentA,
                                                     const std::string &contentB,
                                                     const std::string &nameA,
                                                     const std::string &nameB)
{
    services::ServiceResult result;
    try {
        auto diffs     = diff_utils::DiffYaml::compareFiles(contentA, contentB);
        result.data    = diff_utils::DiffYaml::generateResult(diffs, "Yaml", nameA, nameB);
        result.success = true;
    } catch (const std::exception &e) {
        result.success      = false;
        result.errorMessage = "YAML diff error: " + std::string(e.what());
        LOG_ERROR << "YAML diff failed: " << e.what();
    }
    return result;
}


services::ServiceResult services::DiffService::diffPython(const std::string &contentA,
                                                       const std::string &contentB,
                                                       const std::string &nameA,
                                                       const std::string &nameB)
{
    services::ServiceResult result;
    try {
        // result.data    = DiffPython::runFromText(contentA, contentB, nameA, nameB);
        result.data    = DiffPython::runFromPythonAst(contentA, contentB, nameA, nameB);
        result.success = true;
    } catch (const std::exception &e) {
        result.success      = false;
        result.errorMessage = "Python diff error: " + std::string(e.what());
        LOG_ERROR << "Python diff failed: " << e.what();
    }
    return result;
}


services::ServiceResult services::DiffService::diffText(const std::string &contentA,
                                                     const std::string &contentB,
                                                     const std::string &nameA,
                                                     const std::string &nameB)
{
    services::ServiceResult result;
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


services::ServiceResult services::DiffService::diffTree(const std::string& dirA,
                                                              const std::string& dirB)
{
    services::ServiceResult result;
    result.success = false;


    fs::path rootA(dirA), rootB(dirB);


    try {
        if (!fs::exists(rootA) || !fs::is_directory(rootA)) {
            result.errorMessage = "dirA is not a directory: " + dirA;
            return result;
        }
        if (!fs::exists(rootB) || !fs::is_directory(rootB)) {
            result.errorMessage = "dirB is not a directory: " + dirB;
            return result;
        }
    } catch (const std::exception& e) {
        result.errorMessage = std::string("filesystem error: ") + e.what();
        return result;
    }


    Json::Value j;
    try {
        j = DiffTree::Run(rootA, rootB);
    } catch (const std::exception& e) {
        result.errorMessage = std::string("treediff failed: ") + e.what();
        return result;
    }


    result.success = true;
    result.data    = std::move(j);
    return result;
}


services::ServiceResult services::DiffService::diff(const std::string &contentA,
                                                 const std::string &contentB,
                                                 const std::string &nameA,
                                                 const std::string &nameB)
{
    services::ServiceResult result;
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
