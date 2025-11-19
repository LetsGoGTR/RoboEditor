#include "utils.h"

#include <sys/socket.h>

#include <filesystem>
#include <stdexcept>

#include "config.h"

namespace fs = std::filesystem;

namespace utils {

// JSON value 추출
std::string extractJson(const std::string& json, const std::string& key) {
  size_t pos = json.find("\"" + key + "\"");
  if (pos == std::string::npos) {
    return "";
  }
  pos = json.find(":", pos);
  if (pos == std::string::npos) {
    return "";
  }
  pos = json.find("\"", pos);
  if (pos == std::string::npos) {
    return "";
  }
  size_t end = json.find("\"", pos + 1);
  if (end == std::string::npos) {
    return "";
  }
  return json.substr(pos + 1, end - pos - 1);
}

// JSON response 생성
std::string jsonMsg(bool ok, const std::string& msg) {
  if (ok) {
    return R"({"success":true,"message":")" + msg + R"("})";
  }
  return R"({"success":false,"message":")" + msg + R"("})";
}

// User validation
std::string validateUser(const std::string& body) {
  std::string user = extractJson(body, "user");

  if (user.empty()) {
    throw std::invalid_argument("Missing user field");
  }

  // Path traversal 방지
  if (user.find("..") != std::string::npos ||
      user.find("/") != std::string::npos) {
    throw std::invalid_argument("Invalid user");
  }

  if (!fs::exists(Config::PATH_HOME_BASE + user)) {
    throw std::invalid_argument("User not found");
  }

  return user;
}

// HTTP response 전송
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

  // MSG_NOSIGNAL: SIGPIPE 방지
  send(socket, response.c_str(), response.length(), MSG_NOSIGNAL);
}

}  // namespace utils
