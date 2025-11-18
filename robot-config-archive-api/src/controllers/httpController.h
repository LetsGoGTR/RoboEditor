#pragma once

#include <string>

#include "robotController.h"
#include "workspaceController.h"

namespace controllers {

class HttpController {
 public:
  void handleRequest(int client, const std::string& client_ip);

 private:
  void routeGetRequest(int client, const std::string& path);
  void routePostRequest(int client, const std::string& path,
                        const std::string& body);

  RobotController robotController;
  WorkspaceController workspaceController;
};

}  // namespace controllers
