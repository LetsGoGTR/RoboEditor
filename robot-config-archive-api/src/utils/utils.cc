#include "utils.h"

#include <sys/socket.h>

#include <filesystem>
#include <stdexcept>

#include "config.h"

namespace fs = std::filesystem;

namespace utils {

std::string extractJson(const std::string& json, const std::string& key) {
  size_t pos = json.find("\"" + key + "\"");
  if (pos == std::string::npos) {
    return "";
  }
  pos = json.find(":", pos);
  pos = json.find("\"", pos);
  size_t end = json.find("\"", pos + 1);
  if (pos != std::string::npos && end != std::string::npos) {
    return json.substr(pos + 1, end - pos - 1);
  }
  return "";
}

std::string jsonMsg(bool ok, const std::string& msg) {
  if (ok) {
    return R"({"success":true,"message":")" + msg + R"("})";
  }
  return R"({"success":false,"message":")" + msg + R"("})";
}

std::string validateUser(const std::string& body) {
  std::string user = extractJson(body, "user");

  if (user.empty()) {
    throw std::invalid_argument("Missing user field");
  }

  // Prevent path traversal attacks
  if (user.find("..") != std::string::npos ||
      user.find("/") != std::string::npos) {
    throw std::invalid_argument("Invalid user");
  }

  // Check if user directory exists
  if (!fs::exists(Config::PATH_HOME_BASE + user)) {
    throw std::invalid_argument("User not found");
  }

  return user;
}

void sendHttpResponse(int socket, int status, const std::string& body) {
  const char* text;

  if (status == 200) {
    text = "OK";
  } else if (status == 400) {
    text = "Bad Request";
  } else if (status == 401) {
    text = "Unauthorized";
  } else if (status == 404) {
    text = "Not Found";
  } else {
    text = "Internal Server Error";
  }

  std::string response = "HTTP/1.1 " + std::to_string(status) + " " + text +
                         "\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: " +
                         std::to_string(body.length()) + "\r\n\r\n" + body;

  send(socket, response.c_str(), response.length(), 0);
}

}  // namespace utils
