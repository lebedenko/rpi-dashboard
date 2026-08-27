#include "dashboard_daemon/daemon.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
int main() {
  using namespace dashboard_daemon;
  auto config = parse_config(
      "[dashboard]\nhost=\"127.0.0.1\"\nport=51337\n[telemetry]\ninterval_seconds=5\ndisplay_name=\"Pi\"\n");
  assert(config.interval_seconds == 5);
  for (auto bad : {"[dashboard]\nhost=\"\"\n", "[dashboard]\nhost=\"x\"\nport=0\n",
                   "[dashboard]\nhost=\"x\"\nhost=\"y\"\n", "[other]\nx=1\n"}) {
    try {
      (void)parse_config(bad);
      assert(false);
    } catch (const std::runtime_error&) {
    }
  }
  assert(cbor(Value::object({{"bbb", Value::u(2)}, {"a", Value::u(1)}})) ==
         std::vector<std::uint8_t>({0xa2, 0x61, 0x61, 1, 0x63, 0x62, 0x62, 0x62, 2}));
  auto temp = std::filesystem::temp_directory_path() / ("dashboard-daemon-test-" + std::to_string(getpid()));
  std::filesystem::create_directories(temp / "etc");
  auto path = temp / "device-id";
  auto first = load_or_create_identity(path);
  assert(first == load_or_create_identity(path));
  std::ofstream(temp / "etc/hostname") << "fixture-host\n";
  std::filesystem::create_directories(temp / "proc");
  std::ofstream(temp / "proc/meminfo") << "MemTotal: 1024 kB\nMemAvailable: 512 kB\n";
  Collector collector(temp);
  assert(!cbor(collector.system_info()).empty());
  assert(collector.metrics().has_value());
  std::filesystem::remove_all(temp);
  std::cout << "dashboard daemon tests passed\n";
}
