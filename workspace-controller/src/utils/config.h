#pragma once

#include <cstddef>

namespace Config {
// Server
constexpr int PORT = 8888;
constexpr int LISTEN_BACKLOG = 10;
constexpr int HTTP_TIMEOUT_SEC = 30;

// Buffer size
constexpr size_t REQUEST_BUFFER_SIZE = 65536;   // 64KB
constexpr size_t FILE_BUFFER_SIZE = 8192;       // 8KB
constexpr size_t ARCHIVE_BLOCK_SIZE = 10240;    // 10KB

// Safety limits
constexpr int MAX_RECURSION_DEPTH = 100;
constexpr size_t MAX_EXTRACT_SIZE = 1024 * 1024 * 1024;  // 1GB
constexpr size_t MAX_ARCHIVE_SIZE = 100 * 1024 * 1024;   // 100MB

// Paths
constexpr const char* PATH_HOME_BASE = "/home/";
constexpr const char* PATH_WORKSPACE = "/workspace";
constexpr const char* PATH_INPUT = "/input.tgz";
constexpr const char* PATH_OUTPUT = "/output.tgz";
constexpr const char* PATH_BACKUP_BASE = "/tmp/workspace_backup";
}  // namespace Config
