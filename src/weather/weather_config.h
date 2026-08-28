#pragma once

#include <QByteArray>
#include <QString>

#include <cstdint>
#include <optional>

namespace dashboard::weather {
// NOLINTBEGIN(readability-identifier-naming)

enum class LocationMode : std::uint8_t { Automatic, City, Coordinates };

struct WeatherConfig {
  QString provider{QStringLiteral("openweather")};
  int refreshIntervalSeconds{600};
  LocationMode locationMode{LocationMode::Automatic};
  QString automaticProvider{QStringLiteral("ipgeolocation")};
  QString city;
  double latitude{};
  double longitude{};
};

struct ConfigResult {
  std::optional<WeatherConfig> config;
  QString path;
  QString diagnostic;
  bool fatal{};
};

struct CredentialResult {
  QByteArray value;
  QString diagnostic;
};

[[nodiscard]] ConfigResult loadWeatherConfig(const QString& explicitPath = {});
[[nodiscard]] WeatherConfig parseWeatherConfig(const QByteArray& contents);
[[nodiscard]] CredentialResult loadCredential(const QByteArray& filePath, const QByteArray& fallback,
                                              const QString& label);

// NOLINTEND(readability-identifier-naming)
}  // namespace dashboard::weather
