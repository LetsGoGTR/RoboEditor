#pragma once

#include <string>

namespace services
{
    class AuthService
    {
      public:
        static bool verifyDevicePassword(const std::string &password);

      private:
        static std::string getPasswordHashFromConfig();
    };

}  // namespace services
