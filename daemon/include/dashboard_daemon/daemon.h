#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace dashboard_daemon {
struct Config {
  std::string host;
  std::uint16_t port{51337};
  int interval_seconds{1};
  std::string display_name;
};
Config parse_config(std::string_view text);
std::filesystem::path discover_config(const std::optional<std::filesystem::path>& explicit_path = std::nullopt);
using Uuid = std::array<std::uint8_t, 16>;
Uuid load_or_create_identity(const std::filesystem::path& path);
Uuid random_uuid();
std::string uuid_string(const Uuid& id);

struct Value {
  enum class Kind { Unsigned, Number, Text, Bytes, Boolean, Array, Map } kind;
  std::uint64_t unsigned_value{};
  double number{};
  bool boolean{};
  std::string text;
  std::vector<std::uint8_t> bytes;
  std::vector<Value> array;
  std::map<std::string, Value> map;
  static Value u(std::uint64_t v);
  static Value n(double v);
  static Value s(std::string v);
  static Value b(std::vector<std::uint8_t> v);
  static Value flag(bool v);
  static Value list(std::vector<Value> v);
  static Value object(std::map<std::string, Value> v);
};
std::vector<std::uint8_t> cbor(const Value& value);

class Collector {
 public:
  explicit Collector(std::filesystem::path root = "/");
  Value system_info() const;
  std::optional<Value> metrics();

 private:
  std::filesystem::path root_;
  std::optional<std::pair<std::uint64_t, std::uint64_t>> previous_cpu_;
  std::map<std::string, std::pair<std::uint64_t, std::uint64_t>> previous_network_;
  std::optional<std::chrono::steady_clock::time_point> previous_time_;
};
std::vector<std::uint8_t> hello(const Config&, const Uuid&, const Uuid&, const Value&);
std::vector<std::uint8_t> snapshot(const Config&, const Uuid&, const Uuid&, std::uint64_t, const Value&,
                                   const std::optional<Value>&);
int run(const Config&, const std::filesystem::path& identity_path, bool once = false);
}  // namespace dashboard_daemon
