#include "protocol/device_snapshot.h"

#include <QCborMap>
#include <QCborValue>
#include <QtTest>

using namespace dashboard::protocol;
// NOLINTBEGIN(modernize-use-designated-initializers, readability-convert-member-functions-to-static)

namespace {
SystemInfo sampleInfo() {
  SystemInfo info;
  info.host.host_name = QStringLiteral("pi-five");
  info.os.os_pretty_name = QStringLiteral("Raspberry Pi OS");
  info.hardware.compatible_ids = QStringList{QStringLiteral("raspberrypi,5-model-b")};
  info.cpu.logical_cpu_count = 4;
  info.memory.total_bytes = 8'000'000'000ULL;
  return info;
}
}  // namespace

class DeviceSnapshotTest : public QObject {
  Q_OBJECT
 private slots:
  void helloRoundTrip();
  void snapshotRoundTripCoversMetricGroups();
  void encodingIsDeterministic();
  void rejectsInvalidRatio();
  void rejectsOversizedDatagram();
};

void DeviceSnapshotTest::helloRoundTrip() {
  Hello source{QUuid::createUuid(), QUuid::createUuid(), QStringLiteral("Workshop Pi"), 2, sampleInfo()};
  const auto decoded = decodeMessage(encodeMessage(source));
  QVERIFY(decoded.message);
  const auto* hello = std::get_if<Hello>(&*decoded.message);
  QVERIFY(hello);
  QCOMPARE(hello->device_id, source.device_id);
  QCOMPARE(hello->system_info.cpu.logical_cpu_count, std::optional<quint32>(4));
}

void DeviceSnapshotTest::snapshotRoundTripCoversMetricGroups() {
  SystemMetrics metrics;
  metrics.cpu.usage_ratio = 0.25;
  metrics.cpu.logical_cpus.append({QStringLiteral("cpu0"), 0.5, 2'400'000'000ULL});
  metrics.memory = {.total_bytes = 1000, .available_bytes = 400};
  metrics.system.uptime_seconds = 42.5;
  metrics.storage_volumes.append({QStringLiteral("/"), QStringLiteral("/dev/root"), true, false, 100, 20});
  metrics.network_interfaces.append({QStringLiteral("eth0"), 10, 20, 1.5, 2.5});
  metrics.gpus.append({QStringLiteral("v3d"), 0.3, 100, 30, 1'000, 2'000, 55.0});
  DeviceSnapshot source{QUuid::createUuid(), QUuid::createUuid(), 1, 9, sampleInfo(), metrics};
  const auto decoded = decodeMessage(encodeMessage(source));
  QVERIFY(decoded.message);
  const auto* snapshot = std::get_if<DeviceSnapshot>(&*decoded.message);
  QVERIFY(snapshot && snapshot->metrics);
  QCOMPARE(snapshot->sequence, 9ULL);
  QCOMPARE(snapshot->metrics->storage_volumes.first().mount_point, QStringLiteral("/"));
  QCOMPARE(snapshot->metrics->network_interfaces.first().rx_bytes, std::optional<quint64>(10));
  QCOMPARE(snapshot->metrics->gpus.first().usage_ratio, std::optional<double>(0.3));
}

void DeviceSnapshotTest::encodingIsDeterministic() {
  const Hello source{QUuid::createUuid(), QUuid::createUuid(), QStringLiteral("Pi"), 1, sampleInfo()};
  QCOMPARE(encodeMessage(source), encodeMessage(source));
}

void DeviceSnapshotTest::rejectsInvalidRatio() {
  SystemMetrics metrics;
  metrics.cpu.usage_ratio = 1.1;
  const DeviceSnapshot source{QUuid::createUuid(), QUuid::createUuid(), 1, 0, sampleInfo(), metrics};
  QVERIFY(!decodeMessage(encodeMessage(source)).message);
}

void DeviceSnapshotTest::rejectsOversizedDatagram() {
  QVERIFY(!decodeMessage(QByteArray(maximum_datagram_size + 1, 'x')).message);
}

QTEST_APPLESS_MAIN(DeviceSnapshotTest)
#include "device_snapshot_test.moc"
// NOLINTEND(modernize-use-designated-initializers, readability-convert-member-functions-to-static)
