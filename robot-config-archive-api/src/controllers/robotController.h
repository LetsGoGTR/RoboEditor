#pragma once

#include <string>

namespace controllers {

class RobotController {
 public:
  // GET /api/robot/running
  void handleRunning(int client);
};

}  // namespace controllers
