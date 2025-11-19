#pragma once

#include <string>

namespace controllers {

class WorkspaceController {
 public:
  // POST /api/workspace/compress
  void handleCompress(int client, const std::string& body);

  // POST /api/workspace/extract
  void handleExtract(int client, const std::string& body);
};

}  // namespace controllers
