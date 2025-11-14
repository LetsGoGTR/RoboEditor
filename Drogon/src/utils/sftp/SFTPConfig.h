#pragma once
#include <string>

class SFTPConfig
{
  public:
    std::string host;
    int         port;
    std::string user;
    std::string password;
    int         timeout;  // 초 단위

    SFTPConfig(const std::string &h   = "localhost",
               int                p   = 22,
               const std::string &u   = "",
               const std::string &pwd = "",
               int                t   = 60) :
        host(h),
        port(p),
        user(u),
        password(pwd),
        timeout(t)
    {
    }
};
