#include "dashboard/device_model.h"

#include "sysinfo/sys_info_service.h"
#include "sysmetrics/sys_metrics_service.h"
#include "telemetry/remote_device_registry.h"

#include <QTemporaryDir>
#include <QtTest>

#include <memory>

namespace {
class InfoCollector final : public dashboard::sysinfo::SysInfoCollector {
 public:
  [[nodiscard]] dashboard::sysinfo::SysInfoCollectionResult collect() const override { return {}; }
};
class MetricsCollector final : public dashboard::sysmetrics::SysMetricsCollector {
 public:
  dashboard::sysmetrics::SysMetricsCollectionResult collect() override { return {}; }
};
}  // namespace

class DeviceModelTest : public QObject {
  Q_OBJECT
 private slots:
  void appendsAndUpdatesRemoteInPlace();
  void invalidatesWhenDependencyIsDestroyed();
};

void DeviceModelTest::appendsAndUpdatesRemoteInPlace() {  // NOLINT(readability-convert-member-functions-to-static)
  QTemporaryDir directory;
  dashboard::sysinfo::SysInfoService info(std::make_shared<InfoCollector>());
  dashboard::sysmetrics::SysMetricsService metrics(std::make_shared<MetricsCollector>(), 60'000);
  dashboard::telemetry::RemoteDeviceRegistry registry(directory.filePath(QStringLiteral("devices.cbor")));
  dashboard::DeviceModel model(info, metrics, registry);
  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(model.get(0).value(QStringLiteral("deviceNumber")).toString(), QStringLiteral("01"));

  dashboard::protocol::SystemInfo remote_info;
  remote_info.host.host_name = QStringLiteral("fallback-host");
  remote_info.cpu.architecture = QStringLiteral("x86_64");
  const QUuid device = QUuid::createUuid();
  const QUuid instance = QUuid::createUuid();
  dashboard::protocol::Hello hello{.device_id = device,
                                   .instance_id = instance,
                                   .display_name = QStringLiteral("Build server"),
                                   .interval_seconds = 1,
                                   .system_info = remote_info};
  QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
  QVERIFY(registry.registerHello(hello, QHostAddress::LocalHost, 12345, 0).isEmpty());
  QCOMPARE(inserted.count(), 1);
  QCOMPARE(model.rowCount(), 2);
  QCOMPARE(model.get(1).value(QStringLiteral("hostname")).toString(), QStringLiteral("Build server"));
  QCOMPARE(model.get(1).value(QStringLiteral("statusKey")).toString(), QStringLiteral("registered"));
  QCOMPARE(model.get(1).value(QStringLiteral("historyAvailable")).toBool(), false);

  dashboard::protocol::SystemMetrics remote_metrics;
  remote_metrics.cpu.usage_ratio = 0.25;
  remote_metrics.memory.total_bytes = 1000;
  remote_metrics.memory.available_bytes = 400;
  remote_metrics.system.uptime_seconds = 120.0;
  dashboard::protocol::DeviceSnapshot snapshot{.device_id = device,
                                               .instance_id = instance,
                                               .interval_seconds = 1,
                                               .sequence = 0,
                                               .system_info = remote_info,
                                               .metrics = remote_metrics};
  QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
  QVERIFY(registry.acceptSnapshot(snapshot, QHostAddress::LocalHost, 12345, 1));
  QCOMPARE(changed.count(), 1);
  QVERIFY(!changed.first().at(2).value<QList<int>>().isEmpty());
  QCOMPARE(model.get(1).value(QStringLiteral("statusKey")).toString(), QStringLiteral("online"));
  QCOMPARE(model.get(1).value(QStringLiteral("cpuUsageRatio")).toDouble(), 0.25);
  QCOMPARE(model.get(1).value(QStringLiteral("memoryUsageRatio")).toDouble(), 0.6);
}

void DeviceModelTest::
    invalidatesWhenDependencyIsDestroyed() {  // NOLINT(readability-convert-member-functions-to-static)
  QTemporaryDir directory;
  auto info = std::make_unique<dashboard::sysinfo::SysInfoService>(std::make_shared<InfoCollector>());
  dashboard::sysmetrics::SysMetricsService metrics(std::make_shared<MetricsCollector>(), 60'000);
  dashboard::telemetry::RemoteDeviceRegistry registry(directory.filePath(QStringLiteral("devices.cbor")));
  dashboard::DeviceModel model(*info, metrics, registry);
  QCOMPARE(model.rowCount(), 1);
  info.reset();
  QCOMPARE(model.rowCount(), 0);
}

QTEST_GUILESS_MAIN(DeviceModelTest)
#include "device_model_test.moc"
