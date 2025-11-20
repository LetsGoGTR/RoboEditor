#include <arpa/inet.h>
#include <netinet/in.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "controllers/httpController.h"
#include "utils/config.h"

// Graceful shutdown
static std::atomic<bool> running{true};
static int server_socket = -1;

static void signalHandler(int signum) {
  PLOGI << "Received signal " << signum << ", shutting down...";
  running = false;
  if (server_socket >= 0) {
    close(server_socket);
    server_socket = -1;
  }
}

int main(int argc, char* argv[]) {
  // Logger 초기화
  const std::string log_dir = "/var/log/workspace-controller";
  const std::string log_path = log_dir + "/server.log";
  std::filesystem::create_directories(log_dir);

  static plog::RollingFileAppender<plog::TxtFormatter> fileAppender(
      log_path.c_str(), 1024 * 1024, 3);
  plog::init(plog::info, &fileAppender);

  try {
    // Port 설정
    int port = Config::PORT;
    if (argc > 1) {
      try {
        int parsed_port = std::stoi(argv[1]);
        if (parsed_port >= 1 && parsed_port <= 65535) {
          port = parsed_port;
        } else {
          PLOGW << "Invalid port range. Using default port " << Config::PORT;
        }
      } catch (...) {
        PLOGW << "Invalid port format. Using default port " << Config::PORT;
      }
    }

    // Signal handler 등록
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Socket 생성 및 설정
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
      throw std::runtime_error("Socket failed");
    }
    server_socket = server;

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0) {
      close(server);
      server_socket = -1;
      throw std::runtime_error("Bind failed");
    }

    if (listen(server, Config::LISTEN_BACKLOG) < 0) {
      close(server);
      server_socket = -1;
      throw std::runtime_error("Listen failed");
    }

    PLOGI << "Server started on port " << port;

    controllers::HttpController httpController;

    // Main loop
    while (running) {
      sockaddr_in client_addr;
      socklen_t len = sizeof(client_addr);
      int client = accept(server, (sockaddr*)&client_addr, &len);
      if (client < 0) {
        if (!running) break;
        continue;
      }

      try {
        // Timeout 설정
        struct timeval timeout;
        timeout.tv_sec = Config::HTTP_TIMEOUT_SEC;
        timeout.tv_usec = 0;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // Client IP 추출
        char ip_str[INET_ADDRSTRLEN];
        std::string client_ip = "unknown";
        if (inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str))) {
          client_ip = ip_str;
        }

        httpController.handleRequest(client, client_ip);
      } catch (const std::exception& e) {
        PLOGE << "Error: " << e.what();
      }
      close(client);
    }

    close(server);
  } catch (const std::exception& e) {
    PLOGF << "Fatal: " << e.what();
    return 1;
  }
  return 0;
}
