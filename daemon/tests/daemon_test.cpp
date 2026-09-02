#include "dashboard_daemon/daemon.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <unistd.h>

namespace {
void write_fixture(const std::filesystem::path& root, std::string_view path, std::string_view contents) {
  const auto destination = root / path.substr(1);
  std::filesystem::create_directories(destination.parent_path());
  std::ofstream file(destination, std::ios::binary);
  file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

const dashboard_daemon::Value& field(const dashboard_daemon::Value& value, std::string_view key) {
  assert(value.kind == dashboard_daemon::Value::Kind::Map);
  const auto iterator = value.map.find(std::string(key));
  assert(iterator != value.map.end());
  return iterator->second;
}

bool contains_text(const dashboard_daemon::Value& value, std::string_view needle) {
  if (value.kind == dashboard_daemon::Value::Kind::Text && value.text.find(needle) != std::string::npos) return true;
  for (const auto& item : value.array)
    if (contains_text(item, needle)) return true;
  for (const auto& [key, item] : value.map)
    if (key.find(needle) != std::string::npos || contains_text(item, needle)) return true;
  return false;
}
}  // namespace

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
  write_fixture(temp, "/etc/hostname", "fixture-host.example.test\n");
  write_fixture(temp, "/etc/os-release", "ID=debian\nVERSION_ID=13\nPRETTY_NAME=\"Debian GNU/Linux 13\"\n");
  write_fixture(temp, "/proc/meminfo", "MemTotal: 1024 kB\nMemAvailable: 512 kB\n");
  write_fixture(temp, "/proc/cpuinfo", "Serial: do-not-leak\nMachine ID: private\nRevision: c04170\n");
  std::string compatible_ids = "raspberrypi,5-model-b";
  compatible_ids.push_back('\0');
  compatible_ids += "brcm,bcm2712";
  compatible_ids.push_back('\0');
  write_fixture(temp, "/sys/firmware/devicetree/base/compatible", compatible_ids);
  std::string model = "Raspberry Pi 5 Model B Rev 1.0";
  model.push_back('\0');
  write_fixture(temp, "/sys/firmware/devicetree/base/model", model);
  write_fixture(temp, "/sys/devices/system/cpu/online", "0-3\n");
  for (unsigned cpu = 0; cpu < 4; ++cpu) {
    const auto base = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/topology/";
    write_fixture(temp, base + "physical_package_id", "0\n");
    write_fixture(temp, base + "core_id", std::to_string(cpu / 2) + "\n");
  }
  Collector collector(temp);
  const auto info = collector.system_info();
  assert(field(field(info, "host"), "host_name").text == "fixture-host");
  assert(field(field(info, "os"), "os_family").text == "linux");
  assert(field(field(info, "os"), "os_id").text == "debian");
  assert(field(field(info, "os"), "os_version").text == "13");
  assert(field(field(info, "os"), "os_pretty_name").text == "Debian GNU/Linux 13");
  assert(!field(field(info, "kernel"), "kernel_type").text.empty());
  assert(!field(field(info, "kernel"), "kernel_version").text.empty());
  assert(field(field(info, "hardware"), "manufacturer").text == "Raspberry Pi");
  assert(field(field(info, "hardware"), "model").text == "Raspberry Pi 5 Model B Rev 1.0");
  assert(field(field(info, "hardware"), "board_revision").text == "c04170");
  const auto& compatible = field(field(info, "hardware"), "compatible_ids");
  assert(compatible.array.size() == 2);
  assert(compatible.array[0].text == "raspberrypi,5-model-b");
  assert(compatible.array[1].text == "brcm,bcm2712");
  assert(!field(field(info, "cpu"), "architecture").text.empty());
  assert(field(field(info, "cpu"), "vendor").text == "Broadcom");
  assert(field(field(info, "cpu"), "model").text == "BCM2712");
  assert(field(field(info, "cpu"), "logical_cpu_count").unsigned_value == 4);
  assert(field(field(info, "cpu"), "physical_core_count").unsigned_value == 2);
  assert(field(field(info, "memory"), "total_bytes").unsigned_value == 1024 * 1024);
  assert(!contains_text(info, "do-not-leak"));
  assert(!contains_text(info, "private"));
  assert(collector.metrics().has_value());

  const auto generic = temp / "generic";
  write_fixture(generic, "/sys/class/dmi/id/sys_vendor", "Example Systems\n");
  write_fixture(generic, "/sys/class/dmi/id/product_name", "Example Workstation\n");
  write_fixture(generic, "/proc/cpuinfo",
                "vendor_id: GenuineIntel\nmodel name: Example Processor 9000\nSerial: do-not-publish\n");
  const auto generic_info = Collector(generic).system_info();
  assert(field(field(generic_info, "hardware"), "manufacturer").text == "Example Systems");
  assert(field(field(generic_info, "hardware"), "model").text == "Example Workstation");
  assert(field(field(generic_info, "cpu"), "vendor").text == "GenuineIntel");
  assert(field(field(generic_info, "cpu"), "model").text == "Example Processor 9000");
  assert(!contains_text(generic_info, "do-not-publish"));

  const auto invalid = temp / "invalid";
  write_fixture(invalid, "/etc/hostname", "unknown\n");
  write_fixture(invalid, "/etc/os-release", "ID=unknown\nVERSION_ID=\nPRETTY_NAME=unknown\n");
  write_fixture(invalid, "/proc/meminfo", "MemTotal: 18446744073709551615 kB\n");
  write_fixture(invalid, "/sys/devices/system/cpu/online", "0-2,2\n");
  write_fixture(invalid, "/sys/firmware/devicetree/base/compatible", std::string(64 * 1024 + 1, 'x'));
  write_fixture(invalid, "/sys/class/dmi/id/sys_vendor", "unknown\n");
  write_fixture(invalid, "/sys/class/dmi/id/product_name", std::string(64 * 1024 + 1, 'x'));
  write_fixture(invalid, "/proc/cpuinfo", "vendor_id: unknown\nmodel name:\nSerial: private\n");
  const auto partial = Collector(invalid).system_info();
  assert(partial.map.find("host") == partial.map.end());
  assert(field(partial, "os").map.size() == 1);
  assert(field(partial, "cpu").map.find("logical_cpu_count") == field(partial, "cpu").map.end());
  assert(field(partial, "cpu").map.find("physical_core_count") == field(partial, "cpu").map.end());
  assert(partial.map.find("hardware") == partial.map.end());
  assert(field(partial, "cpu").map.find("vendor") == field(partial, "cpu").map.end());
  assert(field(partial, "cpu").map.find("model") == field(partial, "cpu").map.end());
  assert(partial.map.find("memory") == partial.map.end());
  std::filesystem::remove_all(temp);
  std::cout << "dashboard daemon tests passed\n";
}
