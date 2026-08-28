#include "dashboard_config.h"

#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QStandardPaths>

#include <stdexcept>
#include <string_view>
#include <toml++/toml.hpp>

namespace dashboard {
namespace {

[[nodiscard]] QString textValue(const toml::node_view<toml::node>& node, const QString& fallback) {
  return QString::fromStdString(node.value_or(fallback.toStdString())).trimmed();
}

[[nodiscard]] QByteArray pathValue(const toml::node_view<toml::node>& node) {
  return QByteArray::fromStdString(node.value_or(std::string{})).trimmed();
}

[[noreturn]] void invalid(const QString& message) {
  throw std::runtime_error(QStringLiteral("Invalid dashboard configuration: %1").arg(message).toStdString());
}

}  // namespace

DashboardConfig parseDashboardConfig(const QByteArray& contents) {
  DashboardConfig result;
  toml::table document;
  try {
    document = toml::parse(std::string_view(contents.constData(), static_cast<std::size_t>(contents.size())));
  } catch (const toml::parse_error& error) {
    invalid(QString::fromUtf8(error.description().data(), static_cast<qsizetype>(error.description().size())));
  }

  if (auto* const display = document["display"].as_table()) {
    result.windowed = (*display)["windowed"].value_or(false);
    result.window_width = static_cast<int>((*display)["width"].value_or<int64_t>(1480));
    result.window_height = static_cast<int>((*display)["height"].value_or<int64_t>(320));
    if (result.window_width <= 0 || result.window_height <= 0) {
      invalid(QStringLiteral("display dimensions must be positive"));
    }
  }
  if (auto* const projects = document["projects"].as_table()) {
    result.github_owner = textValue((*projects)["github_owner"], result.github_owner);
    if (result.github_owner.isEmpty()) {
      invalid(QStringLiteral("GitHub owner must not be empty"));
    }
  }
  if (auto* const telemetry = document["telemetry"].as_table()) {
    result.telemetry_bind_address = textValue((*telemetry)["bind_address"], result.telemetry_bind_address);
    const auto port = (*telemetry)["port"].value_or<int64_t>(51337);
    const QHostAddress address(result.telemetry_bind_address);
    if (port < 1 || port > 65535 || address.protocol() != QAbstractSocket::IPv4Protocol) {
      invalid(QStringLiteral("telemetry listener must use a valid IPv4 address and port"));
    }
    result.telemetry_port = static_cast<int>(port);
  }
  if (auto* const credentials = document["credentials"].as_table()) {
    result.github_token_file = pathValue((*credentials)["github_token_file"]);
    result.open_weather_api_key_file = pathValue((*credentials)["openweather_api_key_file"]);
    result.ip_geolocation_api_key_file = pathValue((*credentials)["ipgeolocation_api_key_file"]);
  }
  if (document.contains("weather")) {
    result.weather = weather::parseWeatherConfig(contents);
  }
  return result;
}

DashboardConfigResult loadDashboardConfig(const QString& explicitPath) {
  QString path = explicitPath;
  if (path.isEmpty()) {
    for (const auto& directory : QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation)) {
      const QString candidate = QDir(directory).filePath(QStringLiteral("rpi-dashboard/config.toml"));
      if (QFile::exists(candidate)) {
        path = candidate;
        break;
      }
    }
    if (path.isEmpty()) {
      return {};
    }
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {.path = path,
            .diagnostic = QStringLiteral("Dashboard configuration could not be read"),
            .fatal = !explicitPath.isEmpty()};
  }
  try {
    return {.config = parseDashboardConfig(file.readAll()), .path = path};
  } catch (const std::exception& error) {
    return {.path = path, .diagnostic = QString::fromUtf8(error.what()), .fatal = !explicitPath.isEmpty()};
  }
}

}  // namespace dashboard
