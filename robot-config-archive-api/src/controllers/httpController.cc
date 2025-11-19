#include "httpController.h"

#include <unistd.h>

#include <iostream>
#include <sstream>

#include "../utils/config.h"
#include "../utils/utils.h"

namespace controllers {

void HttpController::routeGetRequest(int client, const std::string& path) {
  // Route to RobotController
  if (path == "/api/robot/running") {
    robotController.handleRunning(client);
    return;
  }

  // No matching route
  utils::sendHttpResponse(client, 404, utils::jsonMsg(false, "Not found"));
}

void HttpController::routePostRequest(int client, const std::string& path,
                                      const std::string& body) {
  // Route to WorkspaceController
  if (path == "/api/workspace/compress") {
    workspaceController.handleCompress(client, body);
    return;
  }

  if (path == "/api/workspace/extract") {
    workspaceController.handleExtract(client, body);
    return;
  }

  // No matching route
  utils::sendHttpResponse(client, 404, utils::jsonMsg(false, "Not found"));
}

void HttpController::handleRequest(int client, const std::string& client_ip) {
  char buf[Config::REQUEST_BUFFER_SIZE] = {0};
  if (read(client, buf, sizeof(buf) - 1) <= 0) return;

  try {
    std::string req(buf);
    std::istringstream stream(req);
    std::string method, path, line;

    // Parse request line
    std::getline(stream, line);
    std::istringstream(line) >> method >> path;

    std::cout << client_ip << " - " << method << " " << path << '\n';

    // Route based on HTTP method
    if (method == "GET") {
      routeGetRequest(client, path);
      return;
    }

    if (method == "POST") {
      // Skip headers
      while (std::getline(stream, line) && line != "\r" && !line.empty()) {
      }

      // Read body
      std::string body;
      std::getline(stream, body, '\0');

      routePostRequest(client, path, body);
      return;
    }

    // Unsupported method
    utils::sendHttpResponse(client, 404, utils::jsonMsg(false, "Not found"));
  } catch (const std::exception& e) {
    utils::sendHttpResponse(client, 500, utils::jsonMsg(false, e.what()));
  }
}

}  // namespace controllers
