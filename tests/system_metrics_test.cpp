#include "sysmetrics/linux_sys_metrics_collector.h"
#include "sysmetrics/sys_metrics_service.h"
#include "sysmetrics/system_metric_history_model.h"

#include <QHash>
#include <QtTest>

#include <memory>

using dashboard::protocol::SystemMetrics;
using dashboard::sysmetrics::LinuxStorageVolume;
using dashboard::sysmetrics::LinuxSysMetricsAccess;
using dashboard::sysmetrics::LinuxSysMetricsCollector;
using dashboard::sysmetrics::SysMetricsCollectionResult;
using dashboard::sysmetrics::SysMetricsCollector;
using dashboard::sysmetrics::SysMetricsService;
using dashboard::sysmetrics::SystemMetricHistoryModel;

namespace {
class FakeAccess final : public LinuxSysMetricsAccess {
 public:
  QHash<QString, QByteArray> files;
  QHash<QString, QStringList> directories;
  QList<LinuxStorageVolume> volumes;
  qint64 milliseconds{1000};
  [[nodiscard]] std::optional<QByteArray> readFile(const QString& path, qsizetype limit) const override {
    const auto found = files.constFind(path);
    return found != files.cend() && found->size() <= limit ? std::optional(*found) : std::nullopt;
  }
  [[nodiscard]] QStringList directoryEntries(const QString& path) const override { return directories.value(path); }
  [[nodiscard]] QList<LinuxStorageVolume> storageVolumes() const override { return volumes; }
  [[nodiscard]] qint64 monotonicMilliseconds() const override { return milliseconds; }
};

std::shared_ptr<FakeAccess> completeAccess() {
  auto access = std::make_shared<FakeAccess>();
  access->files[QStringLiteral("/proc/stat")] =
      QByteArrayLiteral("cpu 100 0 50 850 0 0 0 0\ncpu0 100 0 50 850 0 0 0 0\n");
  access->files[QStringLiteral("/proc/meminfo")] =
      QByteArrayLiteral("MemTotal: 1000 kB\nMemAvailable: 400 kB\nSwapTotal: 200 kB\nSwapFree: 150 kB\n");
  access->files[QStringLiteral("/proc/uptime")] = QByteArrayLiteral("65.5 10.0\n");
  access->files[QStringLiteral("/proc/loadavg")] = QByteArrayLiteral("0.10 0.20 0.30 1/10 1\n");
  access->files[QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq")] =
      QByteArrayLiteral("2400000\n");
  access->volumes = {{.mount_point = QStringLiteral("/"),
                      .device_name = QStringLiteral("/dev/root"),
                      .ready = true,
                      .read_only = false,
                      .total_bytes = 10'000,
                      .available_bytes = 4'000}};
  access->directories[QStringLiteral("/sys/class/net")] = {QStringLiteral("lo"), QStringLiteral("eth0")};
  access->files[QStringLiteral("/sys/class/net/eth0/statistics/rx_bytes")] = QByteArrayLiteral("1000\n");
  access->files[QStringLiteral("/sys/class/net/eth0/statistics/tx_bytes")] = QByteArrayLiteral("2000\n");
  return access;
}

class SequenceCollector final : public SysMetricsCollector {
 public:
  QList<SysMetricsCollectionResult> values;
  qsizetype index{0};
  SysMetricsCollectionResult collect() override { return values.at(qMin(index++, values.size() - 1)); }
};

SystemMetrics readyMetrics() {
  SystemMetrics value;
  value.cpu.usage_ratio = 0.5;
  value.memory.total_bytes = 100;
  value.memory.available_bytes = 25;
  value.system.uptime_seconds = 60;
  value.storage_volumes.append(
      {.mount_point = QStringLiteral("/"), .primary = true, .total_bytes = 1000, .available_bytes = 500});
  return value;
}
}  // namespace

class SystemMetricsTest : public QObject {
  Q_OBJECT
 private slots:
  void classifiesSnapshots();
  void calculatesSecondSampleDeltas();
  void serviceReplacesAndPreservesSnapshots();
  void historyRecordsOptionalValuesAndPrunesOldSamples();
};

void SystemMetricsTest::classifiesSnapshots() {  // NOLINT(readability-convert-member-functions-to-static)
  SystemMetrics metrics;
  QVERIFY(!metrics.hasAnyValue());
  metrics.cpu.usage_ratio = 2.0;
  QVERIFY(!metrics.hasAnyValue());
  metrics = readyMetrics();
  QVERIFY(metrics.hasAllBaselineFields());
  metrics.memory.available_bytes = 101;
  QVERIFY(!metrics.hasAllBaselineFields());
}

void SystemMetricsTest::calculatesSecondSampleDeltas() {  // NOLINT(readability-convert-member-functions-to-static)
  const auto access = completeAccess();
  LinuxSysMetricsCollector collector(access);
  const auto first = collector.collect();
  QVERIFY(!first.metrics.cpu.usage_ratio);
  QCOMPARE(first.metrics.cpu.logical_cpus[0].frequency_hz, 2'400'000'000ULL);
  QVERIFY(!first.metrics.network_interfaces[0].rx_bytes_per_second);
  access->milliseconds = 3000;
  access->files[QStringLiteral("/proc/stat")] =
      QByteArrayLiteral("cpu 180 0 70 950 0 0 0 0\ncpu0 180 0 70 950 0 0 0 0\n");
  access->files[QStringLiteral("/sys/class/net/eth0/statistics/rx_bytes")] = QByteArrayLiteral("1400\n");
  access->files[QStringLiteral("/sys/class/net/eth0/statistics/tx_bytes")] = QByteArrayLiteral("2600\n");
  const auto second = collector.collect();
  QCOMPARE(*second.metrics.cpu.usage_ratio, 0.5);
  QCOMPARE(*second.metrics.network_interfaces[0].rx_bytes_per_second, 200.0);
  QCOMPARE(*second.metrics.network_interfaces[0].tx_bytes_per_second, 300.0);
  QVERIFY(second.metrics.hasAllBaselineFields());
}

void SystemMetricsTest::
    serviceReplacesAndPreservesSnapshots() {  // NOLINT(readability-convert-member-functions-to-static)
  SystemMetrics partial;
  partial.system.uptime_seconds = 1;
  auto collector = std::make_shared<SequenceCollector>();
  collector->values = {
      {.metrics = partial}, {.metrics = readyMetrics()}, {.metrics = {}, .diagnostics = {QStringLiteral("failed")}}};
  SysMetricsService service(collector, 60'000);
  QTRY_COMPARE(service.state(), SysMetricsService::State::Partial);
  service.refresh();
  QTRY_COMPARE(service.state(), SysMetricsService::State::Ready);
  QCOMPARE(service.memoryUsageRatio(), QVariant(0.75));
  const QDateTime success = service.lastSuccessUtc();
  service.refresh();
  QTRY_COMPARE(service.state(), SysMetricsService::State::Error);
  QCOMPARE(service.lastSuccessUtc(), success);
  QCOMPARE(service.cpuUsageRatio(), QVariant(0.5));
  auto* history = service.usageHistoryModel();
  QCOMPARE(history->rowCount(), 3);
  const auto roles = history->roleNames();
  const QModelIndex gap = history->index(2, 0);
  QVERIFY(!history->data(gap, roles.key(QByteArrayLiteral("cpuUsageRatio"))).isValid());
  QVERIFY(!history->data(gap, roles.key(QByteArrayLiteral("memoryUsageRatio"))).isValid());
}

void SystemMetricsTest::
    historyRecordsOptionalValuesAndPrunesOldSamples() {  // NOLINT(readability-convert-member-functions-to-static)
  SystemMetricHistoryModel model;
  model.appendSample(1'000, 0.25, std::nullopt);
  model.appendSample(61'000, std::nullopt, 0.75);
  QCOMPARE(model.rowCount(), 2);
  const auto roles = model.roleNames();
  const int elapsed = roles.key(QByteArrayLiteral("elapsedMilliseconds"));
  const int cpu = roles.key(QByteArrayLiteral("cpuUsageRatio"));
  const int memory = roles.key(QByteArrayLiteral("memoryUsageRatio"));
  QCOMPARE(model.data(model.index(0, 0), elapsed).toLongLong(), 1'000);
  QCOMPARE(model.data(model.index(0, 0), cpu), QVariant(0.25));
  QVERIFY(!model.data(model.index(0, 0), memory).isValid());
  QCOMPARE(model.data(model.index(1, 0), elapsed).toLongLong(), 61'000);
  QVERIFY(!model.data(model.index(1, 0), cpu).isValid());
  QCOMPARE(model.data(model.index(1, 0), memory), QVariant(0.75));

  model.appendSample(61'001, 0.5, 0.6);
  QCOMPARE(model.rowCount(), 2);
  QCOMPARE(model.data(model.index(0, 0), elapsed).toLongLong(), 61'000);
  QCOMPARE(model.data(model.index(1, 0), elapsed).toLongLong(), 61'001);
}

QTEST_MAIN(SystemMetricsTest)
#include "system_metrics_test.moc"
