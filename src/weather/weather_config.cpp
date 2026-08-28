#include "weather/weather_config.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <stdexcept>
#include <toml++/toml.hpp>

namespace dashboard::weather {
namespace {

constexpr int kMinimumRefreshSeconds = 60;

QString validationError(const QString& text) { return QStringLiteral("Invalid weather configuration: %1").arg(text); }

}  // namespace

WeatherConfig parseWeatherConfig(const QByteArray& contents) {
  WeatherConfig result;
  toml::table document;
  try {
    document = toml::parse(std::string_view(contents.constData(), static_cast<std::size_t>(contents.size())));
  } catch (const toml::parse_error& error) {
    throw std::runtime_error(validationError(QString::fromUtf8(error.description().data(),
                                                               static_cast<qsizetype>(error.description().size())))
                                 .toStdString());
  }

  const auto weather = document["weather"];
  if (!weather || !weather.is_table()) {
    throw std::runtime_error(validationError(QStringLiteral("missing [weather] table")).toStdString());
  }
  result.provider = QString::fromStdString(weather["provider"].value_or(std::string{})).trimmed();
  if (result.provider != QStringLiteral("openweather")) {
    throw std::runtime_error(validationError(QStringLiteral("unknown weather provider")).toStdString());
  }
  const auto interval = weather["refresh_interval_seconds"].value<int64_t>();
  if (!interval || *interval < kMinimumRefreshSeconds || *interval > 86400) {
    throw std::runtime_error(
        validationError(QStringLiteral("refresh interval must be between 60 and 86400 seconds")).toStdString());
  }
  result.refreshIntervalSeconds = static_cast<int>(*interval);

  const auto location = weather["location"];
  if (!location || !location.is_table()) {
    throw std::runtime_error(validationError(QStringLiteral("missing [weather.location] table")).toStdString());
  }
  const auto latitude = location["latitude"].value<double>();
  const auto longitude = location["longitude"].value<double>();
  const auto city = QString::fromStdString(location["city"].value_or(std::string{})).trimmed();
  if (latitude.has_value() != longitude.has_value()) {
    throw std::runtime_error(
        validationError(QStringLiteral("latitude and longitude must be specified together")).toStdString());
  }
  if (latitude && !city.isEmpty()) {
    throw std::runtime_error(validationError(QStringLiteral("city conflicts with coordinates")).toStdString());
  }
  if (latitude) {
    if (*latitude < -90.0 || *latitude > 90.0 || *longitude < -180.0 || *longitude > 180.0) {
      throw std::runtime_error(validationError(QStringLiteral("coordinates are out of range")).toStdString());
    }
    result.locationMode = LocationMode::Coordinates;
    result.latitude = *latitude;
    result.longitude = *longitude;
  } else if (!city.isEmpty()) {
    result.locationMode = LocationMode::City;
    result.city = city;
  } else {
    result.locationMode = LocationMode::Automatic;
    result.automaticProvider =
        QString::fromStdString(location["automatic_provider"].value_or(std::string{"ipgeolocation"})).trimmed();
    if (result.automaticProvider != QStringLiteral("ipgeolocation")) {
      throw std::runtime_error(validationError(QStringLiteral("unknown automatic location provider")).toStdString());
    }
  }
  return result;
}

ConfigResult loadWeatherConfig(const QString& explicitPath) {
  QString path = explicitPath;
  if (path.isEmpty()) {
    const auto directories = QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation);
    for (const auto& directory : directories) {
      const QString candidate = QDir(directory).filePath(QStringLiteral("rpi-dashboard/config.toml"));
      if (QFile::exists(candidate)) {
        path = candidate;
        break;
      }
    }
    if (path.isEmpty()) {
      return {.diagnostic = QStringLiteral("Weather is not configured")};
    }
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {.path = path,
            .diagnostic = QStringLiteral("Weather configuration could not be read"),
            .fatal = !explicitPath.isEmpty()};
  }
  try {
    return {.config = parseWeatherConfig(file.readAll()), .path = path};
  } catch (const std::exception& error) {
    return {.path = path, .diagnostic = QString::fromUtf8(error.what()), .fatal = !explicitPath.isEmpty()};
  }
}

CredentialResult loadCredential(const QByteArray& filePath, const QByteArray& fallback, const QString& label) {
  if (filePath.isEmpty()) {
    return {.value = fallback.trimmed()};
  }
  QFile file(QFile::decodeName(filePath));
  if (!file.open(QIODevice::ReadOnly)) {
    return {.diagnostic = QStringLiteral("%1 credential file could not be read").arg(label)};
  }
  const QByteArray value = file.readAll().trimmed();
  if (value.isEmpty()) {
    return {.diagnostic = QStringLiteral("%1 credential file is empty").arg(label)};
  }
  return {.value = value};
}

}  // namespace dashboard::weather
