#include "telemetry/udp_telemetry_receiver.h"

#include <QNetworkDatagram>
#include <QTemporaryDir>
#include <QUdpSocket>
#include <QtTest>

using namespace dashboard;
// NOLINTBEGIN(modernize-use-designated-initializers, readability-convert-member-functions-to-static)

class UdpTelemetryReceiverTest : public QObject {
  Q_OBJECT
 private slots:
  void registrationAndSnapshotOverLoopback();
  void bindFailureIsNonfatal();
};

void UdpTelemetryReceiverTest::registrationAndSnapshotOverLoopback() {
  QTemporaryDir directory;
  telemetry::RemoteDeviceRegistry registry(directory.filePath(QStringLiteral("registry.cbor")));
  telemetry::UdpTelemetryReceiver receiver(&registry);
  QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
  QUdpSocket sender;
  QVERIFY(sender.bind(QHostAddress::LocalHost, 0));
  protocol::SystemInfo info;
  info.host.host_name = QStringLiteral("remote-pi");
  const QUuid device = QUuid::createUuid();
  const QUuid instance = QUuid::createUuid();
  const protocol::Hello hello{device, instance, QStringLiteral("Remote Pi"), 1, info};
  QCOMPARE(sender.writeDatagram(protocol::encodeMessage(hello), QHostAddress::LocalHost, receiver.localPort()),
           protocol::encodeMessage(hello).size());
  QTRY_VERIFY_WITH_TIMEOUT(sender.hasPendingDatagrams(), 1000);
  const auto response = protocol::decodeMessage(sender.receiveDatagram().data());
  QVERIFY(response.message);
  const auto* registration = std::get_if<protocol::RegistrationResult>(&*response.message);
  QVERIFY(registration && registration->accepted);
  const protocol::DeviceSnapshot snapshot{device, instance, 1, 0, info, std::nullopt};
  sender.writeDatagram(protocol::encodeMessage(snapshot), QHostAddress::LocalHost, receiver.localPort());
  QTRY_COMPARE_WITH_TIMEOUT(registry.device(device)->state, telemetry::RemoteDeviceRegistry::State::Online, 1000);
}

void UdpTelemetryReceiverTest::bindFailureIsNonfatal() {
  QTemporaryDir directory;
  QUdpSocket occupied;
  QVERIFY(occupied.bind(QHostAddress::LocalHost, 0));
  telemetry::RemoteDeviceRegistry registry(directory.filePath(QStringLiteral("registry.cbor")));
  telemetry::UdpTelemetryReceiver receiver(&registry);
  QVERIFY(!receiver.bind(QHostAddress::LocalHost, occupied.localPort()));
  QVERIFY(!receiver.diagnostic().isEmpty());
}

QTEST_GUILESS_MAIN(UdpTelemetryReceiverTest)
#include "udp_telemetry_receiver_test.moc"
// NOLINTEND(modernize-use-designated-initializers, readability-convert-member-functions-to-static)
