#include "sysinfo/linux_sys_info_collector.h"
#include "sysinfo/sys_info_service.h"

#include <QHash>
#include <QMutex>
#include <QSemaphore>
#include <QtTest>

#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>

using dashboard::protocol::SystemInfo;
using dashboard::sysinfo::LinuxPlatformAccess;
using dashboard::sysinfo::LinuxPlatformValues;
using dashboard::sysinfo::LinuxSysInfoCollector;
using dashboard::sysinfo::SysInfoCollectionResult;
using dashboard::sysinfo::SysInfoCollector;
using dashboard::sysinfo::SysInfoService;

namespace {

class FakePlatformAccess final : public LinuxPlatformAccess {
 public:
  LinuxPlatformValues platform_values;
  QHash<QString, QByteArray> files;

  [[nodiscard]] LinuxPlatformValues values() const override { return platform_values; }

  [[nodiscard]] std::optional<QByteArray> readFile(const QString& path, qsizetype maximum_bytes) const override {
    const auto iterator = files.constFind(path);
    if (iterator == files.cend() || iterator->size() > maximum_bytes) {
      return std::nullopt;
    }
    return *iterator;
  }
};

LinuxPlatformValues completeValues(QString architecture = QStringLiteral("amd64")) {
  return {
      .host_name = QStringLiteral("dashboard"),
      .os_id = QStringLiteral("debian"),
      .os_version = QStringLiteral("13"),
      .os_pretty_name = QStringLiteral("Debian GNU/Linux 13"),
      .kernel_type = QStringLiteral("linux"),
      .kernel_version = QStringLiteral("6.12.34+rpt-rpi-2712"),
      .architecture = std::move(architecture),
      .logical_cpu_count = 4,
  };
}

std::shared_ptr<FakePlatformAccess> completeGenericAccess() {
  auto access = std::make_shared<FakePlatformAccess>();
  access->platform_values = completeValues();
  access->files.insert(QStringLiteral("/proc/meminfo"), QByteArrayLiteral("MemTotal:       16384000 kB\n"));
  access->files.insert(QStringLiteral("/proc/cpuinfo"),
                       QByteArrayLiteral("vendor_id\t: GenuineIntel\nmodel name\t: Example Processor 9000\n"));
  access->files.insert(QStringLiteral("/sys/class/dmi/id/sys_vendor"), QByteArrayLiteral("Example Systems\n"));
  access->files.insert(QStringLiteral("/sys/class/dmi/id/product_name"), QByteArrayLiteral("Example Workstation\n"));
  return access;
}

void addTopology(const std::shared_ptr<FakePlatformAccess>& access, const QByteArray& online,
                 const QList<std::tuple<quint32, quint32, quint32>>& cpus) {
  access->files.insert(QStringLiteral("/sys/devices/system/cpu/online"), online);
  for (const auto& [cpu_id, package, core] : cpus) {
    const QString base = QStringLiteral("/sys/devices/system/cpu/cpu%1/topology/").arg(cpu_id);
    access->files.insert(base + QStringLiteral("physical_package_id"), QByteArray::number(package));
    access->files.insert(base + QStringLiteral("core_id"), QByteArray::number(core));
  }
}

class SequenceCollector final : public SysInfoCollector {
 public:
  explicit SequenceCollector(QList<SysInfoCollectionResult> results, QSemaphore* entered = nullptr,
                             QSemaphore* proceed = nullptr)
      : results_(std::move(results)), entered_(entered), proceed_(proceed) {}

  [[nodiscard]] SysInfoCollectionResult collect() const override {
    if (entered_ != nullptr) {
      entered_->release();
    }
    if (proceed_ != nullptr) {
      proceed_->acquire();
    }
    QMutexLocker lock(&mutex_);
    const qsizetype index = qMin(calls_, results_.size() - 1);
    ++calls_;
    return results_.at(index);
  }

  [[nodiscard]] qsizetype calls() const {
    QMutexLocker lock(&mutex_);
    return calls_;
  }

 private:
  QList<SysInfoCollectionResult> results_;
  QSemaphore* entered_;
  QSemaphore* proceed_;
  mutable QMutex mutex_;
  mutable qsizetype calls_{0};
};

class ThrowingCollector final : public SysInfoCollector {
 public:
  [[nodiscard]] SysInfoCollectionResult collect() const override { throw std::runtime_error("fixture failure"); }
};

SystemInfo completeInfo() { return LinuxSysInfoCollector(completeGenericAccess()).collect().info; }

}  // namespace

class SystemInfoTest : public QObject {
  Q_OBJECT

 private slots:
  void collectsNormalizedGenericLinuxRecord();
  void enrichesRaspberryPiAndPreservesCompatibleOrder();
  void treatsInvalidAndMissingValuesAsAbsent();
  void countsDistinctPhysicalCores();
  void countsSparseOneThreadPerCoreTopology();
  void rejectsInvalidPhysicalCoreTopology_data();  // NOLINT(readability-identifier-naming)
  void rejectsInvalidPhysicalCoreTopology();
  void doesNotLeakSensitiveCpuInfo();
  void serviceCollectsAtStartupAndCoalescesRefresh();
  void serviceReportsPartialResult();
  void serviceProjectsCompleteSnapshot();
  void serviceProjectsMissingFieldsAsInvalidVariants();
  void servicePreservesLastSuccessAfterFailure();
  void serviceRejectsNullCollectorAndReportsExceptions();
};

void SystemInfoTest::countsDistinctPhysicalCores() {  // NOLINT(readability-convert-member-functions-to-static)
  auto access = completeGenericAccess();
  addTopology(access, "0-3\n", {{0, 0, 0}, {1, 0, 0}, {2, 0, 1}, {3, 0, 1}});

  QCOMPARE(LinuxSysInfoCollector(access).collect().info.cpu.physical_core_count, std::optional<quint32>(2));
}

void SystemInfoTest::countsSparseOneThreadPerCoreTopology() {  // NOLINT(readability-convert-member-functions-to-static)
  auto access = completeGenericAccess();
  addTopology(access, "0,2-3", {{0, 0, 0}, {2, 0, 2}, {3, 1, 0}});

  QCOMPARE(LinuxSysInfoCollector(access).collect().info.cpu.physical_core_count, std::optional<quint32>(3));
}

void SystemInfoTest::rejectsInvalidPhysicalCoreTopology_data() {  // NOLINT(readability-identifier-naming,
                                                                  // readability-convert-member-functions-to-static)
  QTest::addColumn<QByteArray>("online");
  QTest::newRow("missing") << QByteArray{};
  QTest::newRow("malformed") << QByteArray("0-a");
  QTest::newRow("duplicate") << QByteArray("0-2,2");
  QTest::newRow("reversed") << QByteArray("3-1");
  QTest::newRow("over-limit") << QByteArray("0-256");
  QTest::newRow("oversized-id") << QByteArray("4096");
  QTest::newRow("oversized-file") << QByteArray((64 * 1024) + 1, '0');
}

void SystemInfoTest::rejectsInvalidPhysicalCoreTopology() {  // NOLINT(readability-convert-member-functions-to-static)
  QFETCH(QByteArray, online);
  auto access = completeGenericAccess();
  if (!online.isEmpty()) {
    access->files.insert(QStringLiteral("/sys/devices/system/cpu/online"), online);
  }

  QVERIFY(!LinuxSysInfoCollector(access).collect().info.cpu.physical_core_count.has_value());

  addTopology(access, "0-1", {{0, 0, 0}});
  QVERIFY(!LinuxSysInfoCollector(access).collect().info.cpu.physical_core_count.has_value());
}

void SystemInfoTest::
    serviceRejectsNullCollectorAndReportsExceptions() {  // NOLINT(readability-convert-member-functions-to-static)
  QVERIFY_EXCEPTION_THROWN(SysInfoService(nullptr), std::invalid_argument);
  SysInfoService service(std::make_shared<ThrowingCollector>());
  QTRY_COMPARE(service.state(), SysInfoService::State::Error);
  QVERIFY(!service.diagnostics().isEmpty());
}

void SystemInfoTest::collectsNormalizedGenericLinuxRecord() {  // NOLINT(readability-convert-member-functions-to-static)
  const SysInfoCollectionResult result = LinuxSysInfoCollector(completeGenericAccess()).collect();

  QVERIFY(result.info.hasAllBaselineFields());
  QCOMPARE(result.info.os.os_family, QStringLiteral("linux"));
  QCOMPARE(result.info.cpu.architecture, QStringLiteral("x86_64"));
  QCOMPARE(result.info.hardware.manufacturer, QStringLiteral("Example Systems"));
  QCOMPARE(result.info.hardware.model, QStringLiteral("Example Workstation"));
  QCOMPARE(result.info.cpu.vendor, QStringLiteral("GenuineIntel"));
  QCOMPARE(result.info.cpu.model, QStringLiteral("Example Processor 9000"));
  QCOMPARE(result.info.memory.total_bytes, 16'777'216'000ULL);
  QVERIFY(result.diagnostics.contains(QStringLiteral("Raspberry Pi Device Tree enrichment is unavailable")));
}

void SystemInfoTest::
    enrichesRaspberryPiAndPreservesCompatibleOrder() {  // NOLINT(readability-convert-member-functions-to-static)
  auto access = completeGenericAccess();
  access->platform_values.architecture = QStringLiteral("arm64");
  QByteArray compatible = QByteArrayLiteral("raspberrypi,5-model-b");
  compatible.append('\0');
  compatible.append(QByteArrayLiteral("brcm,bcm2712"));
  compatible.append('\0');
  access->files.insert(QStringLiteral("/sys/firmware/devicetree/base/compatible"), compatible);
  access->files.insert(QStringLiteral("/sys/firmware/devicetree/base/model"),
                       QByteArray("Raspberry Pi 5 Model B Rev 1.0\0", 31));
  access->files.insert(QStringLiteral("/proc/cpuinfo"),
                       QByteArrayLiteral("Hardware\t: misleading\nRevision\t: c04170\nSerial\t\t: secret-value\n"));

  const SystemInfo info = LinuxSysInfoCollector(access).collect().info;

  QCOMPARE(info.cpu.architecture, QStringLiteral("aarch64"));
  QCOMPARE(info.hardware.manufacturer, QStringLiteral("Raspberry Pi"));
  QCOMPARE(info.hardware.model, QStringLiteral("Raspberry Pi 5 Model B Rev 1.0"));
  QCOMPARE(*info.hardware.compatible_ids,
           QStringList({QStringLiteral("raspberrypi,5-model-b"), QStringLiteral("brcm,bcm2712")}));
  QCOMPARE(info.cpu.vendor, QStringLiteral("Broadcom"));
  QCOMPARE(info.cpu.model, QStringLiteral("BCM2712"));
  QCOMPARE(info.hardware.board_revision, QStringLiteral("c04170"));
}

void SystemInfoTest::
    treatsInvalidAndMissingValuesAsAbsent() {  // NOLINT(readability-convert-member-functions-to-static)
  auto access = std::make_shared<FakePlatformAccess>();
  access->platform_values = completeValues(QStringLiteral("unknown"));
  access->platform_values.host_name.clear();
  access->platform_values.logical_cpu_count = 0;
  access->files.insert(QStringLiteral("/proc/meminfo"), QByteArrayLiteral("MemTotal: 18014398509481984 kB\n"));
  access->files.insert(QStringLiteral("/proc/cpuinfo"), QByteArrayLiteral("vendor_id: unknown\nmodel name:\n"));
  access->files.insert(QStringLiteral("/sys/class/dmi/id/sys_vendor"), QByteArrayLiteral("unknown\n"));
  access->files.insert(QStringLiteral("/sys/class/dmi/id/product_name"), QByteArray((64 * 1024) + 1, 'x'));

  const SysInfoCollectionResult result = LinuxSysInfoCollector(access).collect();

  QVERIFY(result.info.hasAnyValue());
  QVERIFY(!result.info.hasAllBaselineFields());
  QVERIFY(!result.info.host.host_name.has_value());
  QVERIFY(!result.info.cpu.architecture.has_value());
  QVERIFY(!result.info.cpu.logical_cpu_count.has_value());
  QVERIFY(!result.info.hardware.manufacturer.has_value());
  QVERIFY(!result.info.hardware.model.has_value());
  QVERIFY(!result.info.cpu.vendor.has_value());
  QVERIFY(!result.info.cpu.model.has_value());
  QVERIFY(!result.info.memory.total_bytes.has_value());
  QVERIFY(!result.diagnostics.isEmpty());
}

void SystemInfoTest::doesNotLeakSensitiveCpuInfo() {  // NOLINT(readability-convert-member-functions-to-static)
  auto access = completeGenericAccess();
  QByteArray compatible = QByteArrayLiteral("raspberrypi,5-model-b");
  compatible.append('\0');
  access->files.insert(QStringLiteral("/sys/firmware/devicetree/base/compatible"), compatible);
  access->files.insert(QStringLiteral("/proc/cpuinfo"),
                       QByteArrayLiteral("Serial: do-not-leak\nMachine ID: private\nRevision: d04170\n"));

  const SystemInfo info = LinuxSysInfoCollector(access).collect().info;
  const QStringList published = {info.hardware.board_revision.value_or(QString{}),
                                 info.hardware.model.value_or(QString{}), info.cpu.vendor.value_or(QString{}),
                                 info.cpu.model.value_or(QString{})};
  QVERIFY(!published.join(' ').contains(QStringLiteral("do-not-leak")));
  QVERIFY(!published.join(' ').contains(QStringLiteral("private")));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void SystemInfoTest::serviceCollectsAtStartupAndCoalescesRefresh() {
  QSemaphore entered;
  QSemaphore proceed;
  auto collector = std::make_shared<SequenceCollector>(
      QList<SysInfoCollectionResult>{{.info = completeInfo(), .diagnostics = {}}}, &entered, &proceed);
  SysInfoService service(collector);
  QSignalSpy state_spy(&service, &SysInfoService::stateChanged);

  service.refresh();
  QVERIFY(entered.tryAcquire(1, 2000));
  QCOMPARE(service.state(), SysInfoService::State::Collecting);
  service.refresh();
  proceed.release();
  QTRY_COMPARE_WITH_TIMEOUT(service.state(), SysInfoService::State::Ready, 2000);
  QCOMPARE(collector->calls(), 1);
  QVERIFY(state_spy.count() >= 2);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void SystemInfoTest::serviceReportsPartialResult() {
  SystemInfo partial;
  partial.os.os_family = QStringLiteral("linux");
  auto collector = std::make_shared<SequenceCollector>(QList<SysInfoCollectionResult>{
      {.info = partial, .diagnostics = {QStringLiteral("Hostname is unavailable")}},
  });
  SysInfoService service(collector);

  QTRY_COMPARE_WITH_TIMEOUT(service.state(), SysInfoService::State::Partial, 2000);
  QVERIFY(service.lastSuccessUtc().isValid());
  QCOMPARE(service.currentInfo().os.os_family, QStringLiteral("linux"));
}

void SystemInfoTest::serviceProjectsCompleteSnapshot() {  // NOLINT(readability-convert-member-functions-to-static)
  SystemInfo info = completeInfo();
  info.hardware.manufacturer = QStringLiteral("Acme");
  info.hardware.model = QStringLiteral("Board");
  info.cpu.vendor = QStringLiteral("Vendor");
  info.cpu.model = QStringLiteral("Processor");
  info.cpu.physical_core_count = 2;
  auto collector = std::make_shared<SequenceCollector>(QList<SysInfoCollectionResult>{{.info = info}});
  SysInfoService service(collector);

  QTRY_COMPARE_WITH_TIMEOUT(service.state(), SysInfoService::State::Ready, 2000);
  QCOMPARE(service.hostname(), QVariant(QStringLiteral("dashboard")));
  QCOMPARE(service.osFamily(), QVariant(QStringLiteral("linux")));
  QCOMPARE(service.osId(), QVariant(QStringLiteral("debian")));
  QCOMPARE(service.osVersion(), QVariant(QStringLiteral("13")));
  QCOMPARE(service.osPrettyName(), QVariant(QStringLiteral("Debian GNU/Linux 13")));
  QCOMPARE(service.kernelType(), QVariant(QStringLiteral("linux")));
  QCOMPARE(service.kernelVersion(), QVariant(QStringLiteral("6.12.34+rpt-rpi-2712")));
  QCOMPARE(service.architecture(), QVariant(QStringLiteral("x86_64")));
  QCOMPARE(service.hardwareManufacturer(), QVariant(QStringLiteral("Acme")));
  QCOMPARE(service.hardwareModel(), QVariant(QStringLiteral("Board")));
  QCOMPARE(service.cpuVendor(), QVariant(QStringLiteral("Vendor")));
  QCOMPARE(service.cpuModel(), QVariant(QStringLiteral("Processor")));
  QCOMPARE(service.physicalCoreCount(), QVariant::fromValue<quint32>(2));
  QCOMPARE(service.logicalCpuCount(), QVariant::fromValue<quint32>(4));
  QCOMPARE(service.totalMemoryBytes(), QVariant::fromValue<quint64>(16'777'216'000ULL));
}

void SystemInfoTest::
    serviceProjectsMissingFieldsAsInvalidVariants() {  // NOLINT(readability-convert-member-functions-to-static)
  SystemInfo partial;
  partial.os.os_family = QStringLiteral("linux");
  auto collector = std::make_shared<SequenceCollector>(QList<SysInfoCollectionResult>{{.info = partial}});
  SysInfoService service(collector);

  QTRY_COMPARE_WITH_TIMEOUT(service.state(), SysInfoService::State::Partial, 2000);
  QCOMPARE(service.osFamily(), QVariant(QStringLiteral("linux")));
  QVERIFY(!service.hostname().isValid());
  QVERIFY(!service.hardwareModel().isValid());
  QVERIFY(!service.physicalCoreCount().isValid());
  QVERIFY(!service.totalMemoryBytes().isValid());
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void SystemInfoTest::servicePreservesLastSuccessAfterFailure() {
  const SystemInfo first = completeInfo();
  auto collector = std::make_shared<SequenceCollector>(QList<SysInfoCollectionResult>{
      {.info = first, .diagnostics = {}},
      {.info = {}, .diagnostics = {QStringLiteral("fixture failure")}},
  });
  SysInfoService service(collector);

  QTRY_COMPARE_WITH_TIMEOUT(service.state(), SysInfoService::State::Ready, 2000);
  const QDateTime first_success = service.lastSuccessUtc();
  service.refresh();
  QTRY_COMPARE_WITH_TIMEOUT(service.state(), SysInfoService::State::Error, 2000);
  QCOMPARE(service.currentInfo().host.host_name, first.host.host_name);
  QCOMPARE(service.lastSuccessUtc(), first_success);
  QCOMPARE(service.diagnostics(), QStringList{QStringLiteral("fixture failure")});
}

QTEST_MAIN(SystemInfoTest)

#include "system_info_test.moc"
