#include "telemetry/remote_device_registry.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace dashboard;
// NOLINTBEGIN(modernize-use-designated-initializers, readability-convert-member-functions-to-static,
// readability-identifier-length)

namespace {
protocol::SystemInfo info() {
  protocol::SystemInfo value;
  value.host.host_name = QStringLiteral("pi");
  return value;
}
}  // namespace

class RemoteDeviceRegistryTest : public QObject {
  Q_OBJECT
 private slots:
  void registrationSequenceAndFreshness();
  void snapshotReplacementClearsMetrics();
  void persistenceLoadsOffline();
};

void RemoteDeviceRegistryTest::registrationSequenceAndFreshness() {
  QTemporaryDir directory;
  telemetry::RemoteDeviceRegistry registry(directory.filePath(QStringLiteral("registry.cbor")));
  const QUuid device = QUuid::createUuid();
  const QUuid instance = QUuid::createUuid();
  const QHostAddress address = QHostAddress::LocalHost;
  QCOMPARE(registry.registerHello({device, instance, QStringLiteral("Pi"), 1, info()}, address, 1234, 0), QString());
  QVERIFY(registry.acceptSnapshot({device, instance, 1, 0, info(), std::nullopt}, address, 1234, 100));
  QVERIFY(!registry.acceptSnapshot({device, instance, 1, 0, info(), std::nullopt}, address, 1234, 200));
  registry.updateFreshness(3099);
  QCOMPARE(registry.device(device)->state, telemetry::RemoteDeviceRegistry::State::Online);
  registry.updateFreshness(3100);
  QCOMPARE(registry.device(device)->state, telemetry::RemoteDeviceRegistry::State::Stale);
  registry.updateFreshness(10100);
  QCOMPARE(registry.device(device)->state, telemetry::RemoteDeviceRegistry::State::Offline);
}

void RemoteDeviceRegistryTest::snapshotReplacementClearsMetrics() {
  QTemporaryDir directory;
  telemetry::RemoteDeviceRegistry registry(directory.filePath(QStringLiteral("registry.cbor")));
  const QUuid device = QUuid::createUuid();
  const QUuid instance = QUuid::createUuid();
  const QHostAddress address = QHostAddress::LocalHost;
  QCOMPARE(registry.registerHello({device, instance, QStringLiteral("Pi"), 1, info()}, address, 9, 0), QString());
  protocol::SystemMetrics metrics;
  metrics.cpu.usage_ratio = 0.5;
  QVERIFY(registry.acceptSnapshot({device, instance, 1, 0, info(), metrics}, address, 9, 1));
  QVERIFY(registry.device(device)->metrics);
  QVERIFY(registry.acceptSnapshot({device, instance, 1, 1, info(), std::nullopt}, address, 9, 2));
  QVERIFY(!registry.device(device)->metrics);
}

void RemoteDeviceRegistryTest::persistenceLoadsOffline() {
  QTemporaryDir directory;
  const QString path = directory.filePath(QStringLiteral("registry.cbor"));
  const QUuid id = QUuid::createUuid();
  {
    telemetry::RemoteDeviceRegistry registry(path);
    QCOMPARE(registry.registerHello({id, QUuid::createUuid(), QStringLiteral("Pi"), 2, info()}, QHostAddress::LocalHost,
                                    1, 0),
             QString());
  }
  telemetry::RemoteDeviceRegistry loaded(path);
  QVERIFY(loaded.device(id));
  QCOMPARE(loaded.device(id)->state, telemetry::RemoteDeviceRegistry::State::Offline);
  QVERIFY(loaded.device(id)->instance_id.isNull());
}

QTEST_APPLESS_MAIN(RemoteDeviceRegistryTest)
#include "remote_device_registry_test.moc"
// NOLINTEND(modernize-use-designated-initializers, readability-convert-member-functions-to-static,
// readability-identifier-length)
