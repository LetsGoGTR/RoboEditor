#include "robotController.h"

#include <stdexcept>

#include "../utils/utils.h"

namespace controllers {

void RobotController::handleRunning(int client) {
  utils::sendHttpResponse(client, 200, R"({"success":true,"data":false})");
}

}  // namespace controllers
