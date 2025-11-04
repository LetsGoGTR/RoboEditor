#pragma once

#include <functional>
#include <json/json.h>
#include <string>

#include "ControllerHelper.h"

namespace core
{
    class Device
    {
      public:
        // 모든 디바이스 목록 조회
        static helpers::CoreResult listDevices();

        // 새 디바이스 생성
        static helpers::CoreResult createDevice(const std::string &id,
                                                const std::string &name,
                                                const std::string &description = "",
                                                const std::string &ip          = "");

        // 디바이스 정보 조회
        static helpers::CoreResult getDeviceInfo(const std::string &deviceId);

        // 디바이스 정보 업데이트
        static helpers::CoreResult updateDevice(const std::string &deviceId,
                                                const std::string &name        = "",
                                                const std::string &description = "",
                                                const std::string &ip          = "");

        // 디바이스 삭제
        static helpers::CoreResult deleteDevice(const std::string &deviceId);

        // 워크스페이스를 디바이스에 적용 (비동기)
        static void applyToDevice(const std::string                       &deviceId,
                                  const std::string                       &password,
                                  const std::string                       &workspaceId,
                                  std::function<void(helpers::CoreResult)> callback);

        // 디바이스에서 워크스페이스 백업 (비동기)
        static void backupFromDevice(const std::string                       &deviceId,
                                     std::function<void(helpers::CoreResult)> callback);

      private:
        // 로봇 상태 확인 헬퍼 함수
        static void checkRobotStatus(const std::string                       &ip,
                                     std::function<void()>                    onNotRunning,
                                     std::function<void(helpers::CoreResult)> onError);
    };
}  // namespace core
