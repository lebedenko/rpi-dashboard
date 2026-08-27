#include "telemetry/remote_device_registry.h"

#include <QCborArray>
#include <QCborMap>
#include <QCborParserError>
#include <QCborValue>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>

namespace dashboard::telemetry {
namespace {
constexpr qsizetype maximumDevices = 64;
}
// NOLINTBEGIN(readability-braces-around-statements, readability-identifier-length,
// readability-avoid-return-with-void-value)

RemoteDeviceRegistry::RemoteDeviceRegistry(QString storage_path, QObject* parent)
    : QObject(parent), storage_path_(std::move(storage_path)) {
  clock_.start();
  freshness_timer_.setInterval(1000);
  connect(&freshness_timer_, &QTimer::timeout, this, [this] { updateFreshness(); });
  freshness_timer_.start();
  if (storage_path_.isEmpty()) {
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    storage_path_ = QDir(directory).filePath(QStringLiteral("remote-devices.cbor"));
  }
  load();
}

QVector<RemoteDeviceRegistry::Device> RemoteDeviceRegistry::devices() const { return devices_; }
std::optional<RemoteDeviceRegistry::Device> RemoteDeviceRegistry::device(const QUuid& device_id) const {
  for (const auto& item : devices_)
    if (item.device_id == device_id) return item;
  return std::nullopt;
}
QString RemoteDeviceRegistry::diagnostic() const { return diagnostic_; }
qint64 RemoteDeviceRegistry::now(qint64 supplied) const { return supplied >= 0 ? supplied : clock_.elapsed(); }

QString RemoteDeviceRegistry::registerHello(const protocol::Hello& hello, const QHostAddress& address, quint16 port,
                                            qint64 now_ms) {
  updateFreshness(now_ms);
  for (auto& item : devices_) {
    if (item.device_id != hello.device_id) continue;
    const bool active = item.state != State::Offline;
    const bool same_instance = item.instance_id == hello.instance_id;
    if (active && !same_instance && !item.address.isNull() && item.address != address)
      return QStringLiteral("duplicate_device_id");
    item.display_name = hello.display_name;
    item.interval_seconds = hello.interval_seconds;
    item.system_info = hello.system_info;
    item.address = address;
    item.port = port;
    if (!same_instance) {
      item.instance_id = hello.instance_id;
      item.last_sequence.reset();
      item.metrics.reset();
      item.state = State::Registered;
      item.last_snapshot_ms = -1;
    }
    persist();
    emit deviceChanged(item.device_id);
    return {};
  }
  if (devices_.size() >= maximumDevices) return QStringLiteral("registry_full");
  Device item;
  item.device_id = hello.device_id;
  item.instance_id = hello.instance_id;
  item.display_name = hello.display_name;
  item.interval_seconds = hello.interval_seconds;
  item.system_info = hello.system_info;
  item.state = State::Registered;
  item.address = address;
  item.port = port;
  devices_.append(item);
  persist();
  emit devicesChanged();
  return {};
}

bool RemoteDeviceRegistry::acceptSnapshot(const protocol::DeviceSnapshot& snapshot, const QHostAddress& address,
                                          quint16 port, qint64 now_ms) {
  for (auto& item : devices_) {
    if (item.device_id != snapshot.device_id) continue;
    if (item.instance_id != snapshot.instance_id || item.address != address || item.port != port ||
        item.interval_seconds != snapshot.interval_seconds ||
        (item.last_sequence && snapshot.sequence <= *item.last_sequence))
      return false;
    item.last_sequence = snapshot.sequence;
    item.system_info = snapshot.system_info;
    item.metrics = snapshot.metrics;
    item.last_snapshot_ms = now(now_ms);
    item.state = State::Online;
    persist();
    emit deviceChanged(item.device_id);
    return true;
  }
  return false;
}

void RemoteDeviceRegistry::updateFreshness(qint64 now_ms) {
  const qint64 current = now(now_ms);
  for (auto& item : devices_) {
    if (item.last_snapshot_ms < 0) continue;
    const qint64 age = current - item.last_snapshot_ms;
    State next = State::Online;
    if (age >= static_cast<qint64>(item.interval_seconds) * 10000)
      next = State::Offline;
    else if (age >= static_cast<qint64>(item.interval_seconds) * 3000)
      next = State::Stale;
    if (item.state != next) {
      item.state = next;
      emit deviceChanged(item.device_id);
    }
  }
}

bool RemoteDeviceRegistry::forgetDevice(const QUuid& device_id) {
  for (qsizetype index = 0; index < devices_.size(); ++index)
    if (devices_[index].device_id == device_id) {
      devices_.removeAt(index);
      persist();
      emit devicesChanged();
      return true;
    }
  return false;
}

void RemoteDeviceRegistry::persist() {
  QDir().mkpath(QFileInfo(storage_path_).absolutePath());
  QCborArray entries;
  for (const auto& item : devices_)
    entries.append(QCborMap{{QStringLiteral("device_id"), item.device_id.toRfc4122()},
                            {QStringLiteral("display_name"), item.display_name},
                            {QStringLiteral("interval_s"), item.interval_seconds},
                            {QStringLiteral("system_info"), protocol::encodeSystemInfo(item.system_info)}});
  const QByteArray bytes = QCborValue(QCborMap{{QStringLiteral("devices"), entries}, {QStringLiteral("version"), 1}})
                               .toCbor(QCborValue::SortKeysInMaps);
  QSaveFile file(storage_path_);
  if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
    diagnostic_ = QStringLiteral("Unable to persist remote device registry");
    emit diagnosticChanged();
  }
}

void RemoteDeviceRegistry::load() {
  QFile file(storage_path_);
  if (!file.exists()) return;
  if (!file.open(QIODevice::ReadOnly)) {
    diagnostic_ = QStringLiteral("Unable to read remote device registry");
    emit diagnosticChanged();
    return;
  }
  QCborParserError error;
  auto root = QCborValue::fromCbor(file.readAll(), &error);
  auto fail = [&] {
    devices_.clear();
    diagnostic_ = QStringLiteral("Remote device registry is corrupt");
    emit diagnosticChanged();
  };
  if (error.error != QCborError::NoError || !root.isMap()) return fail();
  auto map = root.toMap();
  auto version = map.value(QStringLiteral("version"));
  auto raw = map.value(QStringLiteral("devices"));
  if (!version.isInteger() || version.toInteger() != 1 || !raw.isArray() || raw.toArray().size() > maximumDevices)
    return fail();
  QSet<QUuid> seen;
  for (const auto& value : raw.toArray()) {
    if (!value.isMap()) return fail();
    auto entry = value.toMap();
    auto idBytes = entry.value(QStringLiteral("device_id"));
    auto name = entry.value(QStringLiteral("display_name"));
    auto interval = entry.value(QStringLiteral("interval_s"));
    auto infoBytes = entry.value(QStringLiteral("system_info"));
    if (!idBytes.isByteArray() || idBytes.toByteArray().size() != 16 || !name.isString() || name.toString().isEmpty() ||
        name.toString().toUtf8().size() > 128 || !interval.isInteger() || interval.toInteger() < 1 ||
        interval.toInteger() > 5 || !infoBytes.isByteArray())
      return fail();
    auto id = QUuid::fromRfc4122(idBytes.toByteArray());
    auto info = protocol::decodeSystemInfo(infoBytes.toByteArray());
    if (id.isNull() || seen.contains(id) || !info) return fail();
    seen.insert(id);
    Device item;
    item.device_id = id;
    item.display_name = name.toString();
    item.interval_seconds = static_cast<int>(interval.toInteger());
    item.system_info = *info;
    devices_.append(item);
  }
}
// NOLINTEND(readability-braces-around-statements, readability-identifier-length,
// readability-avoid-return-with-void-value)
}  // namespace dashboard::telemetry
