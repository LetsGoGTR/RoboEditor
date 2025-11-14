#pragma once

#include <json/json.h>
#include <string>

#include "../utils/sftp/SFTPClient.h"
#include "../utils/sftp/SFTPConfig.h"
#include "ServiceResult.h"

class FileTransferService
{
  public:
    // 원격에서 백업 다운로드
    services::ServiceResult backupFromRemote(const std::string &deviceId,
                                             const std::string &password,
                                             const std::string &remotePath);

    // 워크스페이스 적용
    services::ServiceResult applyWorkspace(const std::string &workspaceId,
                                           const std::string &password);

  private:
    // Workspace API 호출 메서드
    std::pair<bool, std::string>
    extractWorkspace(const std::string &user, const std::string &password, const std::string &api);
};
