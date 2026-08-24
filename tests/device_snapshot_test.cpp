#include "protocol/device_snapshot.h"

#include <QJsonObject>
#include <QtTest>

using dashboard::protocol::DeviceSnapshot;
using dashboard::protocol::fromJson;
using dashboard::protocol::toJson;

class DeviceSnapshotTest : public QObject {
  Q_OBJECT

private slots:
  void roundTripPreservesSnapshot();
  void rejectsUnsupportedProtocol();
  void rejectsNegativeSequence();
  void rejectsOutOfRangeRatio();
  void acceptsMissingOptionalMetrics();
};

void DeviceSnapshotTest::roundTripPreservesSnapshot() {  // NOLINT(readability-convert-member-functions-to-static)
  const DeviceSnapshot source{
      .device_id = QStringLiteral("rpi-5"),
      .display_name = QStringLiteral("Raspberry Pi 5"),
      .boot_id = QStringLiteral("boot-42"),
      .sequence = 17,
      .uptime_seconds = 3600,
      .cpu_usage_ratio = 0.34,
      .memory_usage_ratio = 0.62,
      .temperature_celsius = 58.0,
  };

  const auto result = fromJson(toJson(source));

  QVERIFY(result.has_value());
  QCOMPARE(result->device_id, source.device_id);
  QCOMPARE(result->sequence, source.sequence);
  QCOMPARE(result->cpu_usage_ratio, source.cpu_usage_ratio);
}

void DeviceSnapshotTest::rejectsUnsupportedProtocol() {  // NOLINT(readability-convert-member-functions-to-static)
  QJsonObject object = toJson(DeviceSnapshot{
      .device_id = QStringLiteral("rpi-5"),
      .display_name = QStringLiteral("Raspberry Pi 5"),
      .boot_id = QStringLiteral("boot-42"),
  });
  object.insert(QStringLiteral("protocol"), 99);

  QVERIFY(!fromJson(object).has_value());
}

void DeviceSnapshotTest::rejectsNegativeSequence() {  // NOLINT(readability-convert-member-functions-to-static)
  QJsonObject object = toJson(DeviceSnapshot{
      .device_id = QStringLiteral("rpi-5"),
      .display_name = QStringLiteral("Raspberry Pi 5"),
      .boot_id = QStringLiteral("boot-42"),
  });
  object.insert(QStringLiteral("sequence"), -1);

  QVERIFY(!fromJson(object).has_value());
}

void DeviceSnapshotTest::rejectsOutOfRangeRatio() {  // NOLINT(readability-convert-member-functions-to-static)
  DeviceSnapshot source{
      .device_id = QStringLiteral("rpi-5"),
      .display_name = QStringLiteral("Raspberry Pi 5"),
      .boot_id = QStringLiteral("boot-42"),
      .cpu_usage_ratio = 1.01,
  };

  QVERIFY(!source.isValid());
  QVERIFY(!fromJson(toJson(source)).has_value());
}

void DeviceSnapshotTest::acceptsMissingOptionalMetrics() {  // NOLINT(readability-convert-member-functions-to-static)
  const DeviceSnapshot source{
      .device_id = QStringLiteral("server-1"),
      .display_name = QStringLiteral("Headless server"),
      .boot_id = QStringLiteral("boot-7"),
  };

  const auto result = fromJson(toJson(source));

  QVERIFY(result.has_value());
  QVERIFY(!result->temperature_celsius.has_value());
}

QTEST_APPLESS_MAIN(DeviceSnapshotTest)

#include "device_snapshot_test.moc"
