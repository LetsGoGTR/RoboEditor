#pragma once

#include <json/json.h>
#include <string>
#include <utility> 

#include "../utils/sftp/SFTPClient.h"
#include "../utils/sftp/SFTPConfig.h"
#include "ServiceResult.h"

class FileTransferService
{
  public:
    // 원격에서 백업 다운로드 (output.tgz 고정)
    static services::ServiceResult backupFromRemote(const std::string &deviceId);

    // 워크스페이스 적용 (input.tgz 고정)
    static services::ServiceResult applyWorkspace(const std::string &workspaceId,
                                           const std::string &deviceId,
                                           const std::string &password);

    // 비밀번호 검증
    static bool verifyPassword(const std::string &password);

    // 비밀번호 변경
    static services::ServiceResult changePassword(const std::string &oldPassword,
                                                  const std::string &newPassword);

  private:
    // Workspace API 호출 메서드
    static std::pair<bool, std::string> extractWorkspace(const std::string &user,
                                                  const std::string &api);

    // 비밀번호 파일 경로 가져오기
    static std::string getPasswordFilePath();

    // 비밀번호 해시 읽기
    static std::string readPasswordHash();

    // 비밀번호 해시 쓰기
    static bool writePasswordHash(const std::string &hash);

    // 초기 비밀번호 생성 (0000)
    static void ensurePasswordFileExists();
};
