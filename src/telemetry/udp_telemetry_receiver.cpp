#include "telemetry/udp_telemetry_receiver.h"

#include <QNetworkDatagram>
#include <QTimer>

namespace dashboard::telemetry {
// NOLINTBEGIN(readability-braces-around-statements, readability-qualified-auto,
// readability-implicit-bool-conversion, readability-inconsistent-ifelse-braces,
// modernize-use-designated-initializers)
UdpTelemetryReceiver::UdpTelemetryReceiver(RemoteDeviceRegistry* registry, QObject* parent)
    : QObject(parent), registry_(registry) {
  Q_ASSERT(registry_);
  connect(&socket_, &QUdpSocket::readyRead, this, &UdpTelemetryReceiver::processPendingDatagrams);
}
bool UdpTelemetryReceiver::bind(const QHostAddress& address, quint16 port) {
  if (socket_.bind(address, port)) return true;
  diagnostic_ = QStringLiteral("Telemetry receiver bind failed: %1").arg(socket_.errorString());
  emit diagnosticChanged();
  return false;
}
QString UdpTelemetryReceiver::diagnostic() const { return diagnostic_; }
quint16 UdpTelemetryReceiver::localPort() const { return socket_.localPort(); }
quint64 UdpTelemetryReceiver::acceptedPackets() const { return accepted_; }
quint64 UdpTelemetryReceiver::droppedPackets() const { return dropped_; }
void UdpTelemetryReceiver::processPendingDatagrams() {
  constexpr int batchSize = 32;
  int processed = 0;
  while (socket_.hasPendingDatagrams() && processed++ < batchSize) {
    auto datagram = socket_.receiveDatagram(protocol::maximum_datagram_size + 1);
    if (!datagram.isValid() || datagram.data().size() > protocol::maximum_datagram_size) {
      ++dropped_;
      continue;
    }
    auto decoded = protocol::decodeMessage(datagram.data());
    if (!decoded.message) {
      ++dropped_;
      continue;
    }
    if (auto hello = std::get_if<protocol::Hello>(&*decoded.message)) {
      QString reason = registry_->registerHello(*hello, datagram.senderAddress(), datagram.senderPort());
      protocol::RegistrationResult response{hello->device_id, hello->instance_id, reason.isEmpty(), reason};
      socket_.writeDatagram(protocol::encodeMessage(response), datagram.senderAddress(), datagram.senderPort());
      ++accepted_;
    } else if (auto snapshot = std::get_if<protocol::DeviceSnapshot>(&*decoded.message);
               snapshot && registry_->acceptSnapshot(*snapshot, datagram.senderAddress(), datagram.senderPort()))
      ++accepted_;
    else
      ++dropped_;
  }
  if (socket_.hasPendingDatagrams()) QTimer::singleShot(0, this, &UdpTelemetryReceiver::processPendingDatagrams);
}
// NOLINTEND(readability-braces-around-statements, readability-qualified-auto,
// readability-implicit-bool-conversion, readability-inconsistent-ifelse-braces,
// modernize-use-designated-initializers)
}  // namespace dashboard::telemetry
