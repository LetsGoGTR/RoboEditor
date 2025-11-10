#pragma once

#include <json/json.h>
#include <string>

#include "../utils/sftp/SFTPClient.h"
#include "../utils/sftp/SFTPConfig.h"
#include "ServiceResult.h"

class FileTransferService
{
  public:
    // 원격에서 백업 다운로드 (SFTP)
    services::ServiceResult backupFromRemote(const SFTPConfig  &sftpConfig,
                                             const std::string &user,
                                             const std::string &remotePath,
                                             const std::string &localPath);

    // 워크스페이스 복원 (HTTP 업로드)
    services::ServiceResult applyWorkspace(const std::string &uploadedFilePath,
                                           const std::string &user,
                                           const std::string &password);

  private:
    // Workspace API 호출 메서드
    bool extractWorkspace(const std::string &user,
                         const std::string &password,
                         const std::string &archivePath);
};
