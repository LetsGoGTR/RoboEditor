#include <drogon/drogon.h>
int main()
{
    // Set HTTP listener address and port
    drogon::app().addListener("0.0.0.0", 5555);
    // Load config file
    drogon::app().loadConfigFile("../config.json");
    // Run HTTP framework,the method will block in the internal event loop
    drogon::app().run();
}

/*
#include <filesystem>

int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;

    fs::path exePath = fs::absolute(argv[0]);
    fs::path exeDir  = exePath.parent_path();

    std::error_code ec;
    fs::current_path(exeDir, ec);

    drogon::app().loadConfigFile("config.json");
    drogon::app().run();
}
*/