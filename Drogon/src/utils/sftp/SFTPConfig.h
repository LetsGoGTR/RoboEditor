#pragma once
#include <string>

class SFTPConfig
{
  public:
    std::string host;
    int         port;
    std::string username;
    std::string password;
    int         timeout;  // 초 단위

    SFTPConfig(const std::string &h   = "localhost",
               int                p   = 22,
               const std::string &u   = "",
               const std::string &pwd = "",
               int                t   = 60) :
        host(h),
        port(p),
        username(u),
        password(pwd),
        timeout(t)
    {
    }
};
