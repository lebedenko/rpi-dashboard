#pragma once

#include "weather/weather_config.h"

#include <QByteArray>
#include <QString>

#include <optional>

namespace dashboard {

struct DashboardConfig {
  bool windowed{};
  int window_width{1480};
  int window_height{320};
  QString github_owner{QStringLiteral("lebedenko")};
  QString telemetry_bind_address{QStringLiteral("0.0.0.0")};
  int telemetry_port{51337};
  QByteArray github_token_file;
  QByteArray open_weather_api_key_file;
  QByteArray ip_geolocation_api_key_file;
  std::optional<weather::WeatherConfig> weather;
};

struct DashboardConfigResult {
  DashboardConfig config;
  QString path;
  QString diagnostic;
  bool fatal{};
};

[[nodiscard]] DashboardConfig parseDashboardConfig(const QByteArray& contents);
[[nodiscard]] DashboardConfigResult loadDashboardConfig(const QString& explicitPath = {});

}  // namespace dashboard
