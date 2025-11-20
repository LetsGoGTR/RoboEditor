#pragma once

#include <string>

namespace services {

class WorkspaceService {
 public:
  static std::string compress(const std::string& user);
  static std::string extract(const std::string& user);
};

}  // namespace services
