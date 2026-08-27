#include "dashboard_daemon/daemon.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc, char** argv) {
  try {
    std::optional<std::filesystem::path> requested;
    bool check = false;
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--help") {
        std::cout << "Usage: dashboard-daemon [--config PATH] [--check-config]\n";
        return 0;
      }
      if (arg == "--version") {
        std::cout << "dashboard-daemon " DASHBOARD_DAEMON_VERSION "\n";
        return 0;
      }
      if (arg == "--check-config") {
        check = true;
        continue;
      }
      if (arg == "--config" && i + 1 < argc) {
        requested = argv[++i];
        continue;
      }
      throw std::runtime_error("unknown or incomplete option: " + arg);
    }
    auto path = dashboard_daemon::discover_config(requested);
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read configuration: " + path.string());
    std::string text{std::istreambuf_iterator<char>(input), {}};
    auto config = dashboard_daemon::parse_config(text);
    if (check) {
      std::cout << path << ": configuration is valid\n";
      return 0;
    }
    const char* state = std::getenv("STATE_DIRECTORY");
    auto identity = std::filesystem::path(state ? state : "/var/lib/dashboard-daemon") / "device-id";
    return dashboard_daemon::run(config, identity);
  } catch (const std::exception& error) {
    std::cerr << "dashboard-daemon: " << error.what() << '\n';
    return 1;
  }
}
