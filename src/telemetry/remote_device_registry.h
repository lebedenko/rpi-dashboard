#pragma once

#include "protocol/device_snapshot.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QObject>
#include <QTimer>
#include <QVector>

#include <optional>

namespace dashboard::telemetry {

class RemoteDeviceRegistry final : public QObject {
  Q_OBJECT
 public:
  enum class State : quint8 { Registered, Online, Stale, Offline };
  Q_ENUM(State)
  struct Device {
    QUuid device_id;
    QString display_name;
    int interval_seconds{1};
    protocol::SystemInfo system_info;
    std::optional<protocol::SystemMetrics> metrics;
    State state{State::Offline};
    QUuid instance_id;
    QHostAddress address;
    quint16 port{0};
    std::optional<quint64> last_sequence;
    qint64 last_snapshot_ms{-1};
    QDateTime last_snapshot_utc;
  };

  explicit RemoteDeviceRegistry(QString storage_path = {}, QObject* parent = nullptr);
  [[nodiscard]] QVector<Device> devices() const;
  [[nodiscard]] std::optional<Device> device(const QUuid& device_id) const;
  [[nodiscard]] QString diagnostic() const;
  [[nodiscard]] QString registerHello(const protocol::Hello& hello, const QHostAddress& address, quint16 port,
                                      qint64 now_ms = -1);
  [[nodiscard]] bool acceptSnapshot(const protocol::DeviceSnapshot& snapshot, const QHostAddress& address, quint16 port,
                                    qint64 now_ms = -1);
  void updateFreshness(qint64 now_ms = -1);
  Q_INVOKABLE bool forgetDevice(const QUuid& device_id);

 signals:
  void deviceChanged(const QUuid& device_id);
  void deviceAboutToBeAdded(int index);
  void deviceAdded();
  void deviceAboutToBeRemoved(int index);
  void deviceRemoved();
  void devicesChanged();
  void diagnosticChanged();

 private:
  [[nodiscard]] qint64 now(qint64 supplied) const;
  void persist();
  void load();
  QString storage_path_;
  QVector<Device> devices_;
  QString diagnostic_;
  QElapsedTimer clock_;
  QTimer freshness_timer_;
};

}  // namespace dashboard::telemetry
