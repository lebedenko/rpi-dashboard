#include "dashboard_daemon/daemon.h"

#include <arpa/inet.h>
#include <bit>
#include <charconv>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <netdb.h>
#include <random>
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
  std::map<std::string, Value> root, host, os, kernel, cpu, memory;
  char name[256]{};
  if (root_ == "/" && gethostname(name, sizeof name) == 0)
    host["host_name"] = Value::s(name);
  else {
    auto n = trim(read(rooted(root_, "/etc/hostname")));
    if (!n.empty()) host["host_name"] = Value::s(n);
  }
  struct utsname u{};
  if (root_ == "/" && uname(&u) == 0) {
    kernel["kernel_type"] = Value::s(u.sysname);
    kernel["kernel_version"] = Value::s(u.release);
    cpu["architecture"] = Value::s(u.machine);
    os["os_family"] = Value::s(u.sysname);
  }
  std::string rel = read(rooted(root_, "/etc/os-release"));
  for (std::string line; !rel.empty();) {
    auto e = rel.find('\n');
    line = rel.substr(0, e);
    rel = e == std::string::npos ? "" : rel.substr(e + 1);
    auto q = line.find('=');
    if (q == std::string::npos) continue;
    auto v = line.substr(q + 1);
    if (v.size() > 1 && v.front() == '"' && v.back() == '"') v = v.substr(1, v.size() - 2);
    if (line.substr(0, q) == "ID") os["os_id"] = Value::s(v);
    if (line.substr(0, q) == "VERSION_ID") os["os_version"] = Value::s(v);
    if (line.substr(0, q) == "PRETTY_NAME") os["os_pretty_name"] = Value::s(v);
  }
  std::string mem = read(rooted(root_, "/proc/meminfo"));
  auto p = mem.find("MemTotal:");
  if (p != std::string::npos) {
    std::uint64_t kb{};
    std::from_chars(mem.data() + p + 9, mem.data() + mem.size(), kb);
    memory["total_bytes"] = Value::u(kb * 1024);
  }
  if (!host.empty()) root["host"] = Value::object(host);
  if (!os.empty()) root["os"] = Value::object(os);
  if (!kernel.empty()) root["kernel"] = Value::object(kernel);
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
