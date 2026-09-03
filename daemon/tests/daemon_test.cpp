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
  write_fixture(temp, "/proc/meminfo", "MemTotal: 1024 kB\nMemAvailable: 512 kB\nSwapTotal: 256 kB\nSwapFree: 64 kB\n");
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
  write_fixture(temp, "/sys/class/devfreq/1002000000.v3d/cur_freq", "800000000\n");
  write_fixture(temp, "/sys/class/thermal/thermal_zone0/type", "x86_pkg_temp\n");
  write_fixture(temp, "/sys/class/thermal/thermal_zone0/temp", "52000\n");
  write_fixture(temp, "/sys/class/thermal/thermal_zone1/type", "gpu-thermal\n");
  write_fixture(temp, "/sys/class/thermal/thermal_zone1/temp", "51000\n");
  write_fixture(temp, "/sys/class/net/eth0/statistics/rx_bytes", "1000\n");
  write_fixture(temp, "/sys/class/net/eth0/statistics/tx_bytes", "2000\n");
  write_fixture(temp, "/sys/class/net/lo/statistics/rx_bytes", "50\n");
  write_fixture(temp, "/sys/class/net/lo/statistics/tx_bytes", "50\n");
  write_fixture(temp, "/proc/self/mountinfo",
                "20 1 179:2 / / rw,relatime - ext4 /dev/mmcblk0p2 rw\n"
                "21 20 0:20 / /run rw,nosuid - tmpfs tmpfs rw\n"
                "22 20 259:1 / /data ro,relatime - ext4 /dev/nvme0n1p1 ro\n"
                "23 20 259:1 / /data rw,relatime - ext4 /dev/duplicate rw\n");
  std::filesystem::create_directories(temp / "data");
  std::filesystem::create_directories(temp / "run");
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
  const auto metrics = collector.metrics();
  assert(metrics.has_value());
  assert(field(field(*metrics, "cpu"), "temperature_celsius").number == 52.0);
  assert(field(field(*metrics, "memory"), "swap_total_bytes").unsigned_value == 256 * 1024);
  assert(field(field(*metrics, "memory"), "swap_available_bytes").unsigned_value == 64 * 1024);
  const auto& storage = field(*metrics, "storage_volumes");
  assert(storage.array.size() == 2);
  assert(field(storage.array[0], "mount_point").text == "/");
  assert(field(storage.array[0], "primary").boolean);
  assert(field(storage.array[1], "mount_point").text == "/data");
  assert(field(storage.array[1], "read_only").boolean);
  const auto& gpu = field(*metrics, "gpus").array[0];
  assert(field(gpu, "name").text == "V3D");
  assert(field(gpu, "core_clock_hz").unsigned_value == 800000000);
  assert(field(gpu, "temperature_celsius").number == 51.0);
  assert(gpu.map.find("usage_ratio") == gpu.map.end());
  const auto& network = field(*metrics, "network_interfaces").array;
  assert(network.size() == 1);
  assert(field(network[0], "name").text == "eth0");
  assert(field(network[0], "rx_bytes").unsigned_value == 1000);
  assert(field(network[0], "tx_bytes").unsigned_value == 2000);
  const auto next_metrics = collector.metrics();
  const auto& next_network = field(*next_metrics, "network_interfaces").array[0];
  assert(field(next_network, "rx_bytes_per_second").number == 0.0);
  assert(field(next_network, "tx_bytes_per_second").number == 0.0);

  const auto platform_v3d = temp / "platform-v3d";
  write_fixture(platform_v3d, "/sys/bus/platform/devices/1002000000.v3d/uevent", "DRIVER=v3d\n");
  write_fixture(platform_v3d, "/sys/bus/platform/devices/1002000000.v3d/gpu_stats",
                "queue timestamp jobs runtime\nrender 100 1 10\n");
  Collector platform_collector(platform_v3d);
  const auto platform_metrics = platform_collector.metrics();
  assert(platform_metrics.has_value());
  const auto& platform_gpu = field(*platform_metrics, "gpus").array[0];
  assert(field(platform_gpu, "name").text == "V3D");
  assert(platform_gpu.map.find("core_clock_hz") == platform_gpu.map.end());
  assert(platform_gpu.map.find("usage_ratio") == platform_gpu.map.end());
  write_fixture(platform_v3d, "/sys/bus/platform/devices/1002000000.v3d/gpu_stats",
                "queue timestamp jobs runtime\nrender 200 2 60\n");
  const auto next_platform_metrics = platform_collector.metrics();
  assert(field(field(*next_platform_metrics, "gpus").array[0], "usage_ratio").number == 0.5);

  const auto drm = temp / "drm";
  write_fixture(drm, "/sys/class/drm/card0/device/vendor", "0x10de\n");
  write_fixture(drm, "/sys/class/drm/card1/device/vendor", "0x8086\n");
  write_fixture(drm, "/sys/class/drm/card1/gt_cur_freq_mhz", "1650\n");
  const auto drm_metrics = Collector(drm).metrics();
  assert(drm_metrics.has_value());
  const auto& drm_gpus = field(*drm_metrics, "gpus").array;
  assert(drm_gpus.size() == 2);
  assert(field(drm_gpus[0], "name").text == "Intel");
  assert(field(drm_gpus[0], "core_clock_hz").unsigned_value == 1650000000);
  assert(field(drm_gpus[1], "name").text == "NVIDIA");

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
  write_fixture(invalid, "/proc/meminfo", "MemTotal: 18446744073709551615 kB\nSwapTotal: 1 kB\nSwapFree: 2 kB\n");
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
  const auto invalid_metrics = Collector(invalid).metrics();
  assert(invalid_metrics.has_value());
  const auto& invalid_memory = field(*invalid_metrics, "memory");
  assert(field(invalid_memory, "swap_total_bytes").unsigned_value == 1024);
  assert(invalid_memory.map.find("swap_available_bytes") == invalid_memory.map.end());
  std::filesystem::remove_all(temp);
  std::cout << "dashboard daemon tests passed\n";
}
