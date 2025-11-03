#pragma once

#include <json/json.h>
#include <string>

#include "ControllerHelper.h"

namespace core
{
    class Workspace
    {
      public:
        // 워크스페이스 목록 조회 (선택적으로 deviceId로 필터링)
        static helpers::CoreResult listWorkspaces(const std::string &deviceId = "");

        // 워크스페이스 생성
        static helpers::CoreResult createWorkspace(const std::string &name,
                                                   const std::string &target      = "",
                                                   const std::string &description = "");

        // 워크스페이스 정보 조회
        static helpers::CoreResult getWorkspaceInfo(const std::string &workspaceId);

        // 워크스페이스 업데이트
        static helpers::CoreResult updateWorkspace(const std::string &workspaceId,
                                                   const std::string &name        = "",
                                                   const std::string &target      = "",
                                                   const std::string &description = "");

        // 워크스페이스 삭제
        static helpers::CoreResult deleteWorkspace(const std::string &workspaceId);

        // 워크스페이스 가져오기 (파일에서)
        static helpers::CoreResult importWorkspace(const std::string &filePath,
                                                   const std::string &name        = "",
                                                   const std::string &target      = "",
                                                   const std::string &description = "");

        // 워크스페이스 내보내기
        // Returns: { path: 출력 파일 경로 }
        static helpers::CoreResult exportWorkspace(const std::string &workspaceId);
    };
}  // namespace core
