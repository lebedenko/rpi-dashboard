#pragma once

#include "telemetry/remote_device_registry.h"

#include <QHostAddress>
#include <QObject>
#include <QPointer>
#include <QUdpSocket>

namespace dashboard::telemetry {

class UdpTelemetryReceiver final : public QObject {
  Q_OBJECT
 public:
  explicit UdpTelemetryReceiver(RemoteDeviceRegistry& registry, QObject* parent = nullptr);
  ~UdpTelemetryReceiver() override;
  [[nodiscard]] bool bind(const QHostAddress& address = QHostAddress::AnyIPv4, quint16 port = 51337);
  [[nodiscard]] QString diagnostic() const;
  [[nodiscard]] quint16 localPort() const;
  [[nodiscard]] quint64 acceptedPackets() const;
  [[nodiscard]] quint64 droppedPackets() const;

 signals:
  void diagnosticChanged();

 private slots:
  void processPendingDatagrams();

 private:
  Q_DISABLE_COPY_MOVE(UdpTelemetryReceiver)
  QPointer<RemoteDeviceRegistry> registry_;
  QUdpSocket socket_;
  QString diagnostic_;
  quint64 accepted_{0};
  quint64 dropped_{0};
};
}  // namespace dashboard::telemetry
