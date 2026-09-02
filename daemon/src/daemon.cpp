#include "dashboard_daemon/daemon.h"

#include <algorithm>
#include <arpa/inet.h>
#include <bit>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <netdb.h>
#include <random>
#include <ranges>
#include <set>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <thread>
#include <unistd.h>

namespace dashboard_daemon {
namespace {
std::string trim(std::string s) {
  auto a = s.find_first_not_of(" \t\r\n");
  auto b = s.find_last_not_of(" \t\r\n");
  return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
bool utf8(std::string_view s) {
  unsigned left = 0;
  for (unsigned char c : s) {
    if (!left) {
      if (c < 0x80) continue;
      if ((c & 0xe0) == 0xc0)
        left = 1;
      else if ((c & 0xf0) == 0xe0)
        left = 2;
      else if ((c & 0xf8) == 0xf0)
        left = 3;
      else
        return false;
    } else {
      if ((c & 0xc0) != 0x80) return false;
      --left;
    }
  }
  return left == 0;
}
std::string read(const std::filesystem::path& p) {
  std::ifstream f(p);
  if (!f) return {};
  return {std::istreambuf_iterator<char>(f), {}};
}
constexpr std::size_t kSmallFileLimit = 64 * 1024;
constexpr std::size_t kCpuInfoLimit = 1024 * 1024;
constexpr std::uint32_t kMaximumCpuCount = 256;
constexpr std::uint32_t kMaximumCpuId = 4095;

std::optional<std::string> bounded_read(const std::filesystem::path& path, std::size_t maximum_bytes) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return std::nullopt;
  std::string contents(maximum_bytes + 1, '\0');
  file.read(contents.data(), static_cast<std::streamsize>(contents.size()));
  contents.resize(static_cast<std::size_t>(file.gcount()));
  if (contents.size() > maximum_bytes) return std::nullopt;
  return contents;
}

std::optional<std::string> clean(std::string value) {
  value = trim(std::move(value));
  std::string lower = value;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
  if (value.empty() || lower == "unknown" || !utf8(value)) return std::nullopt;
  return value;
}

std::optional<std::uint32_t> unsigned_value(std::string_view input, std::uint32_t maximum) {
  if (input.empty() || !std::ranges::all_of(input, [](unsigned char c) { return std::isdigit(c); }))
    return std::nullopt;
  std::uint32_t value{};
  const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), value);
  if (error != std::errc{} || end != input.data() + input.size() || value > maximum) return std::nullopt;
  return value;
}

std::optional<std::vector<std::uint32_t>> online_cpu_ids(std::string contents) {
  contents = trim(std::move(contents));
  if (contents.empty()) return std::nullopt;
  std::vector<std::uint32_t> result;
  std::set<std::uint32_t> unique;
  std::size_t position = 0;
  while (position <= contents.size()) {
    const auto comma = contents.find(',', position);
    const std::string_view entry(contents.data() + position,
                                 (comma == std::string::npos ? contents.size() : comma) - position);
    const auto dash = entry.find('-');
    if (entry.empty() || (dash != std::string_view::npos && entry.find('-', dash + 1) != std::string_view::npos))
      return std::nullopt;
    const auto first = unsigned_value(entry.substr(0, dash), kMaximumCpuId);
    const auto last = dash == std::string_view::npos ? first : unsigned_value(entry.substr(dash + 1), kMaximumCpuId);
    if (!first || !last || *first > *last || *last - *first + 1 > kMaximumCpuCount) return std::nullopt;
    for (auto id = *first;; ++id) {
      if (!unique.insert(id).second || result.size() >= kMaximumCpuCount) return std::nullopt;
      result.push_back(id);
      if (id == *last) break;
    }
    if (comma == std::string::npos) break;
    position = comma + 1;
  }
  return result.empty() ? std::nullopt : std::optional(std::move(result));
}

std::optional<std::uint64_t> memory_bytes(std::string_view contents) {
  std::size_t position = 0;
  while (position <= contents.size()) {
    const auto newline = contents.find('\n', position);
    const auto line =
        contents.substr(position, (newline == std::string_view::npos ? contents.size() : newline) - position);
    if (line.starts_with("MemTotal:")) {
      const auto value = trim(std::string(line.substr(9)));
      const auto separator = value.find_first_of(" \t");
      if (separator == std::string::npos || trim(value.substr(separator)) != "kB") return std::nullopt;
      std::uint64_t kibibytes{};
      const auto digits = std::string_view(value).substr(0, separator);
      const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), kibibytes);
      constexpr std::uint64_t bytes_per_kibibyte = 1024;
      if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size() || kibibytes == 0 ||
          kibibytes > std::numeric_limits<std::uint64_t>::max() / bytes_per_kibibyte)
        return std::nullopt;
      return kibibytes * bytes_per_kibibyte;
    }
    if (newline == std::string_view::npos) break;
    position = newline + 1;
  }
  return std::nullopt;
}

std::optional<std::string> cpu_info_value(std::string_view contents, std::string_view expected_key) {
  std::size_t position = 0;
  while (position <= contents.size()) {
    const auto newline = contents.find('\n', position);
    const auto line =
        contents.substr(position, (newline == std::string_view::npos ? contents.size() : newline) - position);
    const auto separator = line.find(':');
    if (separator != std::string_view::npos && trim(std::string(line.substr(0, separator))) == expected_key)
      return clean(std::string(line.substr(separator + 1)));
    if (newline == std::string_view::npos) break;
    position = newline + 1;
  }
  return std::nullopt;
}
void head(std::vector<std::uint8_t>& o, unsigned m, std::uint64_t n) {
  if (n < 24)
    o.push_back((m << 5) | n);
  else if (n < 256) {
    o.push_back((m << 5) | 24);
    o.push_back(n);
  } else if (n < 65536) {
    o.push_back((m << 5) | 25);
    o.push_back(n >> 8);
    o.push_back(n);
  } else if (n < 4294967296ULL) {
    o.push_back((m << 5) | 26);
    for (int i = 3; i >= 0; --i) o.push_back(n >> (i * 8));
  } else {
    o.push_back((m << 5) | 27);
    for (int i = 7; i >= 0; --i) o.push_back(n >> (i * 8));
  }
}
void encode(std::vector<std::uint8_t>& o, const Value& v) {
  switch (v.kind) {
    case Value::Kind::Unsigned:
      head(o, 0, v.unsigned_value);
      break;
    case Value::Kind::Number: {
      o.push_back(0xfb);
      auto bits = std::bit_cast<std::uint64_t>(v.number);
      for (int i = 7; i >= 0; --i) o.push_back(bits >> (i * 8));
      break;
    }
    case Value::Kind::Text:
      head(o, 3, v.text.size());
      o.insert(o.end(), v.text.begin(), v.text.end());
      break;
    case Value::Kind::Bytes:
      head(o, 2, v.bytes.size());
      o.insert(o.end(), v.bytes.begin(), v.bytes.end());
      break;
    case Value::Kind::Boolean:
      o.push_back(v.boolean ? 0xf5 : 0xf4);
      break;
    case Value::Kind::Array:
      head(o, 4, v.array.size());
      for (auto& x : v.array) encode(o, x);
      break;
    case Value::Kind::Map: {
      std::vector<std::pair<std::vector<std::uint8_t>, const Value*>> p;
      for (auto& [k, x] : v.map) p.push_back({cbor(Value::s(k)), &x});
      std::sort(p.begin(), p.end(), [](auto& a, auto& b) {
        return a.first.size() != b.first.size() ? a.first.size() < b.first.size() : a.first < b.first;
      });
      head(o, 5, p.size());
      for (auto& [k, x] : p) {
        o.insert(o.end(), k.begin(), k.end());
        encode(o, *x);
      }
      break;
    }
  }
}
Value envelope(std::string type, const Uuid& d, const Uuid& i) {
  return Value::object({{"device_id", Value::b({d.begin(), d.end()})},
                        {"instance_id", Value::b({i.begin(), i.end()})},
                        {"type", Value::s(std::move(type))},
                        {"version", Value::u(1)}});
}
std::filesystem::path rooted(const std::filesystem::path& r, std::string_view p) {
  return r / (p.front() == '/' ? p.substr(1) : p);
}
}  // namespace
Value Value::u(std::uint64_t v) {
  Value x{Kind::Unsigned};
  x.unsigned_value = v;
  return x;
}
Value Value::n(double v) {
  Value x{Kind::Number};
  x.number = v;
  return x;
}
Value Value::s(std::string v) {
  Value x{Kind::Text};
  x.text = std::move(v);
  return x;
}
Value Value::b(std::vector<std::uint8_t> v) {
  Value x{Kind::Bytes};
  x.bytes = std::move(v);
  return x;
}
Value Value::flag(bool v) {
  Value x{Kind::Boolean};
  x.boolean = v;
  return x;
}
Value Value::list(std::vector<Value> v) {
  Value x{Kind::Array};
  x.array = std::move(v);
  return x;
}
Value Value::object(std::map<std::string, Value> v) {
  Value x{Kind::Map};
  x.map = std::move(v);
  return x;
}
std::vector<std::uint8_t> cbor(const Value& v) {
  std::vector<std::uint8_t> o;
  encode(o, v);
  return o;
}
Config parse_config(std::string_view input) {
  if (!utf8(input)) throw std::runtime_error("configuration is not valid UTF-8");
  Config c;
  std::string section;
  std::map<std::string, bool> seen;
  std::string all(input);
  size_t pos = 0;
  while (pos <= all.size()) {
    auto e = all.find('\n', pos);
    auto line = trim(all.substr(pos, e - pos));
    pos = e == std::string::npos ? all.size() + 1 : e + 1;
    if (line.empty() || line[0] == '#') continue;
    if (line.front() == '[' && line.back() == ']') {
      section = line.substr(1, line.size() - 2);
      if (section != "dashboard" && section != "telemetry") throw std::runtime_error("unknown section");
      continue;
    }
    auto eq = line.find('=');
    if (eq == std::string::npos || section.empty()) throw std::runtime_error("malformed configuration");
    auto key = section + "." + trim(line.substr(0, eq));
    auto raw = trim(line.substr(eq + 1));
    if (seen[key]) throw std::runtime_error("duplicate key: " + key);
    seen[key] = true;
    if (key == "dashboard.host" || key == "telemetry.display_name") {
      if (raw.size() < 2 || raw.front() != '"' || raw.back() != '"') throw std::runtime_error("expected quoted string");
      auto v = raw.substr(1, raw.size() - 2);
      if (v.find_first_of("\\\n\r") != std::string::npos) throw std::runtime_error("unsupported string escape");
      if (key == "dashboard.host")
        c.host = v;
      else
        c.display_name = v;
    } else if (key == "dashboard.port" || key == "telemetry.interval_seconds") {
      int v{};
      auto [p, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), v);
      if (ec != std::errc{} || p != raw.data() + raw.size()) throw std::runtime_error("expected integer");
      if (key == "dashboard.port") {
        if (v < 1 || v > 65535) throw std::runtime_error("port must be 1..65535");
        c.port = v;
      } else {
        if (v < 1 || v > 5) throw std::runtime_error("interval must be 1..5");
        c.interval_seconds = v;
      }
    } else
      throw std::runtime_error("unknown key: " + key);
  }
  if (c.host.empty()) throw std::runtime_error("dashboard.host is required");
  if (c.display_name.size() > 128) throw std::runtime_error("display_name exceeds 128 bytes");
  return c;
}
std::filesystem::path discover_config(const std::optional<std::filesystem::path>& explicit_path) {
  if (explicit_path) return *explicit_path;
  std::vector<std::filesystem::path> p;
  if (auto* x = getenv("XDG_CONFIG_HOME"))
    p.emplace_back(std::filesystem::path(x) / "dashboard-daemon/config.toml");
  else if (auto* h = getenv("HOME"))
    p.emplace_back(std::filesystem::path(h) / ".config/dashboard-daemon/config.toml");
  if (auto* x = getenv("XDG_CONFIG_DIRS")) {
    std::string s = x;
    size_t a = 0;
    while (a <= s.size()) {
      auto b = s.find(':', a);
      p.emplace_back(std::filesystem::path(s.substr(a, b - a)) / "dashboard-daemon/config.toml");
      if (b == std::string::npos) break;
      a = b + 1;
    }
  } else
    p.emplace_back("/etc/xdg/dashboard-daemon/config.toml");
  for (auto& x : p)
    if (std::filesystem::is_regular_file(x)) return x;
  throw std::runtime_error("no configuration file found");
}
Uuid random_uuid() {
  Uuid id{};
  int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
  if (fd < 0 || ::read(fd, id.data(), id.size()) != (ssize_t)id.size()) {
    if (fd >= 0) close(fd);
    throw std::runtime_error("cannot obtain random identity");
  }
  close(fd);
  id[6] = (id[6] & 0x0f) | 0x40;
  id[8] = (id[8] & 0x3f) | 0x80;
  return id;
}
std::string uuid_string(const Uuid& id) {
  static constexpr char h[] = "0123456789abcdef";
  std::string s;
  for (size_t i = 0; i < id.size(); ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10) s += '-';
    s += h[id[i] >> 4];
    s += h[id[i] & 15];
  }
  return s;
}
Uuid load_or_create_identity(const std::filesystem::path& p) {
  auto s = trim(read(p));
  if (!s.empty()) {
    Uuid id{};
    std::string hex;
    for (char c : s)
      if (c != '-') hex += c;
    if (hex.size() != 32) throw std::runtime_error("invalid persisted device identity");
    for (size_t i = 0; i < 16; ++i) {
      unsigned v{};
      auto [q, e] = std::from_chars(hex.data() + i * 2, hex.data() + i * 2 + 2, v, 16);
      if (e != std::errc{}) throw std::runtime_error("invalid persisted device identity");
      id[i] = v;
    }
    return id;
  }
  std::filesystem::create_directories(p.parent_path());
  auto id = random_uuid();
  auto tmp = p;
  tmp += ".tmp." + std::to_string(getpid());
  int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  auto out = uuid_string(id) + "\n";
  if (fd < 0 || write(fd, out.data(), out.size()) != (ssize_t)out.size() || fsync(fd) || close(fd) ||
      rename(tmp.c_str(), p.c_str())) {
    if (fd >= 0) close(fd);
    unlink(tmp.c_str());
    throw std::runtime_error("cannot persist device identity");
  }
  return id;
}
Collector::Collector(std::filesystem::path r) : root_(std::move(r)) {}
Value Collector::system_info() const {
  std::map<std::string, Value> root, host, os, kernel, hardware, cpu, memory;
  char name[256]{};
  std::optional<std::string> hostname;
  if (root_ == "/" && gethostname(name, sizeof name) == 0) {
    name[sizeof name - 1] = '\0';
    hostname = clean(name);
  } else if (const auto contents = bounded_read(rooted(root_, "/etc/hostname"), kSmallFileLimit)) {
    hostname = clean(*contents);
  }
  if (hostname) {
    hostname = hostname->substr(0, hostname->find('.'));
    if (hostname->empty()) hostname.reset();
  }
  if (hostname) host["host_name"] = Value::s(*hostname);

  struct utsname u{};
  if (uname(&u) == 0) {
    if (auto type = clean(u.sysname)) {
      std::transform(type->begin(), type->end(), type->begin(), [](unsigned char c) { return std::tolower(c); });
      kernel["kernel_type"] = Value::s(*type);
      os["os_family"] = Value::s(*type);
    }
    if (auto version = clean(u.release)) kernel["kernel_version"] = Value::s(*version);
    if (auto architecture = clean(u.machine)) {
      std::transform(architecture->begin(), architecture->end(), architecture->begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (*architecture == "amd64") *architecture = "x86_64";
      if (*architecture == "arm64") *architecture = "aarch64";
      cpu["architecture"] = Value::s(*architecture);
    }
  }

  if (auto release = bounded_read(rooted(root_, "/etc/os-release"), kSmallFileLimit)) {
    for (std::string line; !release->empty();) {
      const auto newline = release->find('\n');
      line = release->substr(0, newline);
      *release = newline == std::string::npos ? "" : release->substr(newline + 1);
      const auto separator = line.find('=');
      if (separator == std::string::npos) continue;
      auto value = line.substr(separator + 1);
      if (value.size() > 1 && value.front() == '"' && value.back() == '"') value = value.substr(1, value.size() - 2);
      const auto normalized = clean(std::move(value));
      if (!normalized) continue;
      const auto key = line.substr(0, separator);
      if (key == "ID") os["os_id"] = Value::s(*normalized);
      if (key == "VERSION_ID") os["os_version"] = Value::s(*normalized);
      if (key == "PRETTY_NAME") os["os_pretty_name"] = Value::s(*normalized);
    }
  }

  if (const auto meminfo = bounded_read(rooted(root_, "/proc/meminfo"), kSmallFileLimit)) {
    if (const auto bytes = memory_bytes(*meminfo)) memory["total_bytes"] = Value::u(*bytes);
  }
  if (const auto manufacturer = bounded_read(rooted(root_, "/sys/class/dmi/id/sys_vendor"), kSmallFileLimit)) {
    if (auto value = clean(*manufacturer)) hardware["manufacturer"] = Value::s(*value);
  }
  if (const auto model = bounded_read(rooted(root_, "/sys/class/dmi/id/product_name"), kSmallFileLimit)) {
    if (auto value = clean(*model)) hardware["model"] = Value::s(*value);
  }
  const auto cpuinfo = bounded_read(rooted(root_, "/proc/cpuinfo"), kCpuInfoLimit);
  if (cpuinfo) {
    if (auto vendor = cpu_info_value(*cpuinfo, "vendor_id")) cpu["vendor"] = Value::s(*vendor);
    if (auto model = cpu_info_value(*cpuinfo, "model name")) cpu["model"] = Value::s(*model);
  }

  std::optional<std::vector<std::uint32_t>> online;
  if (const auto contents = bounded_read(rooted(root_, "/sys/devices/system/cpu/online"), kSmallFileLimit)) {
    online = online_cpu_ids(*contents);
  }
  if (online) {
    cpu["logical_cpu_count"] = Value::u(online->size());
    std::set<std::pair<std::uint32_t, std::uint32_t>> cores;
    bool complete = true;
    for (const auto id : *online) {
      const auto topology = std::string("/sys/devices/system/cpu/cpu") + std::to_string(id) + "/topology/";
      const auto package_file = bounded_read(rooted(root_, topology + "physical_package_id"), kSmallFileLimit);
      const auto core_file = bounded_read(rooted(root_, topology + "core_id"), kSmallFileLimit);
      const auto package =
          package_file ? unsigned_value(trim(*package_file), std::numeric_limits<std::uint32_t>::max()) : std::nullopt;
      const auto core_id =
          core_file ? unsigned_value(trim(*core_file), std::numeric_limits<std::uint32_t>::max()) : std::nullopt;
      if (!package || !core_id) {
        complete = false;
        break;
      }
      cores.emplace(*package, *core_id);
    }
    if (complete && !cores.empty() && cores.size() <= kMaximumCpuCount)
      cpu["physical_core_count"] = Value::u(cores.size());
  } else if (root_ == "/") {
    const long processors = sysconf(_SC_NPROCESSORS_ONLN);
    if (processors > 0 && processors <= kMaximumCpuCount)
      cpu["logical_cpu_count"] = Value::u(static_cast<std::uint64_t>(processors));
  }

  std::vector<std::string> compatible_ids;
  if (const auto compatible =
          bounded_read(rooted(root_, "/sys/firmware/devicetree/base/compatible"), kSmallFileLimit)) {
    std::size_t position = 0;
    while (position < compatible->size()) {
      const auto separator = compatible->find('\0', position);
      const auto length = (separator == std::string::npos ? compatible->size() : separator) - position;
      if (auto identifier = clean(compatible->substr(position, length))) compatible_ids.push_back(*identifier);
      if (separator == std::string::npos) break;
      position = separator + 1;
    }
  }
  const bool is_pi = std::ranges::any_of(compatible_ids, [](const auto& id) { return id.starts_with("raspberrypi,"); });
  if (is_pi) {
    hardware["manufacturer"] = Value::s("Raspberry Pi");
    std::vector<Value> ids;
    for (const auto& id : compatible_ids) ids.push_back(Value::s(id));
    hardware["compatible_ids"] = Value::list(std::move(ids));
    if (const auto model_file = bounded_read(rooted(root_, "/sys/firmware/devicetree/base/model"), kSmallFileLimit)) {
      auto model_text = *model_file;
      std::erase(model_text, '\0');
      if (auto model = clean(std::move(model_text))) hardware["model"] = Value::s(*model);
    }
    if (cpuinfo) {
      std::size_t position = 0;
      while (position <= cpuinfo->size()) {
        const auto newline = cpuinfo->find('\n', position);
        const auto line = std::string_view(*cpuinfo).substr(
            position, (newline == std::string::npos ? cpuinfo->size() : newline) - position);
        const auto separator = line.find(':');
        if (separator != std::string_view::npos && trim(std::string(line.substr(0, separator))) == "Revision") {
          if (auto revision = clean(std::string(line.substr(separator + 1))))
            hardware["board_revision"] = Value::s(*revision);
          break;
        }
        if (newline == std::string::npos) break;
        position = newline + 1;
      }
    }
    for (const auto& id : compatible_ids) {
      if (id.starts_with("brcm,bcm")) {
        cpu["vendor"] = Value::s("Broadcom");
        auto model = id.substr(5);
        std::transform(model.begin(), model.end(), model.begin(), [](unsigned char c) { return std::toupper(c); });
        cpu["model"] = Value::s(std::move(model));
        break;
      }
    }
  }
  if (!host.empty()) root["host"] = Value::object(host);
  if (!os.empty()) root["os"] = Value::object(os);
  if (!kernel.empty()) root["kernel"] = Value::object(kernel);
  if (!hardware.empty()) root["hardware"] = Value::object(hardware);
  if (!cpu.empty()) root["cpu"] = Value::object(cpu);
  if (!memory.empty()) root["memory"] = Value::object(memory);
  return Value::object(root);
}
std::optional<Value> Collector::metrics() {
  std::map<std::string, Value> root, cpu, memory, system;
  auto stat = read(rooted(root_, "/proc/stat"));
  if (stat.starts_with("cpu ")) {
    std::istringstream in(stat.substr(4));
    std::uint64_t x{}, total = 0, idle = 0;
    int i = 0;
    while (in >> x) {
      total += x;
      if (i == 3 || i == 4) idle += x;
      ++i;
    }
    if (previous_cpu_ && total > previous_cpu_->first) {
      auto dt = total - previous_cpu_->first;
      auto di = idle - previous_cpu_->second;
      cpu["usage_ratio"] = Value::n(std::clamp(1.0 - double(di) / double(dt), 0.0, 1.0));
    }
    previous_cpu_ = {{total, idle}};
  }
  auto mem = read(rooted(root_, "/proc/meminfo"));
  for (auto [source, target] : {std::pair{"MemTotal:", "total_bytes"},
                                {"MemAvailable:", "available_bytes"},
                                {"SwapTotal:", "swap_total_bytes"},
                                {"SwapFree:", "swap_available_bytes"}}) {
    auto p = mem.find(source);
    if (p != std::string::npos) {
      std::uint64_t kb{};
      std::from_chars(mem.data() + p + strlen(source), mem.data() + mem.size(), kb);
      memory[target] = Value::u(kb * 1024);
    }
  }
  auto up = trim(read(rooted(root_, "/proc/uptime")));
  if (!up.empty()) {
    char* e{};
    double v = strtod(up.c_str(), &e);
    if (e != up.c_str()) system["uptime_seconds"] = Value::n(v);
  }
  auto load = read(rooted(root_, "/proc/loadavg"));
  if (!load.empty()) {
    std::istringstream in(load);
    double a, b, c;
    if (in >> a >> b >> c) {
      system["load_average_1m"] = Value::n(a);
      system["load_average_5m"] = Value::n(b);
      system["load_average_15m"] = Value::n(c);
    }
  }
  if (root_ == "/") {
    struct statvfs s{};
    if (statvfs("/", &s) == 0)
      root["storage_volumes"] = Value::list({Value::object({{"mount_point", Value::s("/")},
                                                            {"device_name", Value::s("root")},
                                                            {"primary", Value::flag(true)},
                                                            {"read_only", Value::flag(false)},
                                                            {"total_bytes", Value::u(s.f_blocks * s.f_frsize)},
                                                            {"available_bytes", Value::u(s.f_bavail * s.f_frsize)}})});
  }
  if (!cpu.empty()) root["cpu"] = Value::object(cpu);
  if (!memory.empty()) root["memory"] = Value::object(memory);
  if (!system.empty()) root["system"] = Value::object(system);
  if (root.empty()) return std::nullopt;
  return Value::object(root);
}
std::vector<std::uint8_t> hello(const Config& c, const Uuid& d, const Uuid& i, const Value& info) {
  auto v = envelope("hello", d, i);
  v.map["display_name"] = Value::s(c.display_name);
  v.map["interval_s"] = Value::u(c.interval_seconds);
  v.map["system_info"] = info;
  return cbor(v);
}
std::vector<std::uint8_t> snapshot(const Config& c, const Uuid& d, const Uuid& i, std::uint64_t seq, const Value& info,
                                   const std::optional<Value>& m) {
  auto v = envelope("snapshot", d, i);
  v.map["interval_s"] = Value::u(c.interval_seconds);
  v.map["sequence"] = Value::u(seq);
  v.map["system_info"] = info;
  if (m) v.map["metrics"] = *m;
  return cbor(v);
}
int run(const Config& config, const std::filesystem::path& identity, bool once) {
  auto device = load_or_create_identity(identity), instance = random_uuid();
  Collector collector;
  auto info = collector.system_info();
  Config c = config;
  if (c.display_name.empty()) {
    char h[256]{};
    if (gethostname(h, sizeof h)) strcpy(h, "remote-device");
    c.display_name = h;
  }
  std::uint64_t seq = 0;
  bool registered = false;
  for (;;) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    addrinfo* res{};
    auto service = std::to_string(c.port);
    if (getaddrinfo(c.host.c_str(), service.c_str(), &hints, &res)) {
      if (once) return 1;
      std::this_thread::sleep_for(std::chrono::seconds(c.interval_seconds));
      continue;
    }
    int fd = socket(res->ai_family, res->ai_socktype | SOCK_CLOEXEC, res->ai_protocol);
    if (fd < 0) {
      freeaddrinfo(res);
      continue;
    }
    timeval tv{2, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    auto h = hello(c, device, instance, info);
    sendto(fd, h.data(), h.size(), 0, res->ai_addr, res->ai_addrlen);
    std::uint8_t response[2048];
    auto n = recv(fd, response, sizeof response, 0);
    registered = n > 0 && std::find(response, response + n, 0xf5) != response + n;
    if (registered) {
      auto s = snapshot(c, device, instance, seq++, info, collector.metrics());
      sendto(fd, s.data(), s.size(), 0, res->ai_addr, res->ai_addrlen);
    }
    close(fd);
    freeaddrinfo(res);
    if (once) return registered ? 0 : 1;
    std::this_thread::sleep_for(std::chrono::seconds(registered ? c.interval_seconds : 10));
  }
}
}  // namespace dashboard_daemon
