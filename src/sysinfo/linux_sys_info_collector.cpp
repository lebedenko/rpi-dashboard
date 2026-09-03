#include "sysinfo/linux_sys_info_collector.h"

#include <QFile>
#include <QSysInfo>

#include <algorithm>
#include <limits>
#include <ranges>
#include <set>
#include <unistd.h>
#include <utility>

namespace dashboard::sysinfo {
namespace {

constexpr qsizetype kSmallFileLimit = 64 * 1024;
constexpr qsizetype kCpuInfoLimit = 1024 * 1024;
constexpr quint32 kMaximumCpuCount = 256;
constexpr quint32 kMaximumCpuId = 4095;

std::optional<QString> cleanString(QString value) {
  value = value.trimmed();
  if (value.isEmpty() || value.compare(QStringLiteral("unknown"), Qt::CaseInsensitive) == 0) {
    return std::nullopt;
  }
  return value;
}

std::optional<QString> normalizedArchitecture(QString value) {
  value = value.trimmed().toLower();
  if (value == QStringLiteral("amd64")) {
    value = QStringLiteral("x86_64");
  } else if (value == QStringLiteral("arm64")) {
    value = QStringLiteral("aarch64");
  }
  return cleanString(value);
}

QStringList compatibleIds(const QByteArray& contents) {
  QStringList result;
  for (const QByteArray& entry : contents.split('\0')) {
    if (const auto value = cleanString(QString::fromUtf8(entry)); value.has_value()) {
      result.append(*value);
    }
  }
  return result;
}

std::optional<quint64> memoryBytes(const QByteArray& contents) {
  for (const QByteArray& line : contents.split('\n')) {
    if (!line.startsWith("MemTotal:")) {
      continue;
    }
    const QList<QByteArray> parts = line.simplified().split(' ');
    if (parts.size() != 3 || parts.at(2) != "kB") {
      return std::nullopt;
    }
    bool valid = false;
    const quint64 kibibytes = parts.at(1).toULongLong(&valid);
    constexpr quint64 bytes_per_kibibyte = 1024;
    if (!valid || kibibytes == 0 || kibibytes > std::numeric_limits<quint64>::max() / bytes_per_kibibyte) {
      return std::nullopt;
    }
    return kibibytes * bytes_per_kibibyte;
  }
  return std::nullopt;
}

std::optional<QString> boardRevision(const QByteArray& contents) {
  for (const QByteArray& line : contents.split('\n')) {
    const qsizetype separator = line.indexOf(':');
    if (separator < 0 || line.left(separator).trimmed() != "Revision") {
      continue;
    }
    return cleanString(QString::fromLatin1(line.mid(separator + 1)));
  }
  return std::nullopt;
}

std::optional<QString> cpuInfoValue(const QByteArray& contents, const QByteArray& key) {
  for (const QByteArray& line : contents.split('\n')) {
    const qsizetype separator = line.indexOf(':');
    if (separator >= 0 && line.first(separator).trimmed() == key) {
      return cleanString(QString::fromUtf8(line.sliced(separator + 1)));
    }
  }
  return std::nullopt;
}

std::optional<quint32> unsignedValue(const QByteArray& contents, quint32 maximum) {
  const QByteArray value = contents.trimmed();
  if (value.isEmpty() ||
      !std::ranges::all_of(value, [](char character) { return character >= '0' && character <= '9'; })) {
    return std::nullopt;
  }
  bool valid = false;
  const quint64 parsed = value.toULongLong(&valid);
  if (!valid || parsed > maximum) {
    return std::nullopt;
  }
  return static_cast<quint32>(parsed);
}

std::optional<QList<quint32>> onlineCpuIds(const QByteArray& contents) {
  const QList<QByteArray> entries = contents.trimmed().split(',');
  if (entries.isEmpty()) {
    return std::nullopt;
  }
  QList<quint32> result;
  std::set<quint32> unique;
  for (const QByteArray& entry : entries) {
    if (entry.isEmpty() || entry != entry.trimmed() || entry.count('-') > 1) {
      return std::nullopt;
    }
    const qsizetype separator = entry.indexOf('-');
    const auto first = unsignedValue(separator < 0 ? entry : entry.first(separator), kMaximumCpuId);
    const auto last = unsignedValue(separator < 0 ? entry : entry.sliced(separator + 1), kMaximumCpuId);
    if (!first || !last || *first > *last || *last - *first + 1 > kMaximumCpuCount) {
      return std::nullopt;
    }
    for (quint32 id = *first;; ++id) {
      if (!unique.insert(id).second || result.size() >= kMaximumCpuCount) {
        return std::nullopt;
      }
      result.append(id);
      if (id == *last) {
        break;
      }
    }
  }
  return result.isEmpty() ? std::nullopt : std::optional<QList<quint32>>(std::move(result));
}

std::optional<quint32> physicalCoreCount(const LinuxPlatformAccess& access) {
  const auto online = access.readFile(QStringLiteral("/sys/devices/system/cpu/online"), kSmallFileLimit);
  const auto ids = online ? onlineCpuIds(*online) : std::nullopt;
  if (!ids) {
    return std::nullopt;
  }
  std::set<std::pair<quint32, quint32>> cores;
  for (const quint32 cpu_id : *ids) {
    const QString base = QStringLiteral("/sys/devices/system/cpu/cpu%1/topology/").arg(cpu_id);
    const auto package_file = access.readFile(base + QStringLiteral("physical_package_id"), kSmallFileLimit);
    const auto core_file = access.readFile(base + QStringLiteral("core_id"), kSmallFileLimit);
    const auto package =
        package_file ? unsignedValue(*package_file, std::numeric_limits<quint32>::max()) : std::nullopt;
    const auto core = core_file ? unsignedValue(*core_file, std::numeric_limits<quint32>::max()) : std::nullopt;
    if (!package || !core) {
      return std::nullopt;
    }
    cores.emplace(*package, *core);
  }
  return cores.empty() || cores.size() > kMaximumCpuCount ? std::nullopt
                                                          : std::optional<quint32>(static_cast<quint32>(cores.size()));
}

class NativeLinuxPlatformAccess final : public LinuxPlatformAccess {
 public:
  [[nodiscard]] LinuxPlatformValues values() const override {
    const long processors = ::sysconf(_SC_NPROCESSORS_ONLN);
    return {
        .host_name = QSysInfo::machineHostName(),
        .os_id = QSysInfo::productType(),
        .os_version = QSysInfo::productVersion(),
        .os_pretty_name = QSysInfo::prettyProductName(),
        .kernel_type = QSysInfo::kernelType(),
        .kernel_version = QSysInfo::kernelVersion(),
        .architecture = QSysInfo::currentCpuArchitecture(),
        .logical_cpu_count = processors > 0 && std::in_range<quint32>(processors)
                                 ? std::optional<quint32>(static_cast<quint32>(processors))
                                 : std::nullopt,
    };
  }

  [[nodiscard]] std::optional<QByteArray> readFile(const QString& path, qsizetype maximum_bytes) const override {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
      return std::nullopt;
    }
    QByteArray contents = file.read(maximum_bytes + 1);
    if (contents.size() > maximum_bytes) {
      return std::nullopt;
    }
    return contents;
  }
};

void addMissingBaselineDiagnostics(const protocol::SystemInfo& info, QStringList& diagnostics) {
  if (!info.host.host_name) {
    diagnostics.append(QStringLiteral("Hostname is unavailable"));
  }
  if (!info.os.os_family) {
    diagnostics.append(QStringLiteral("OS family is unavailable"));
  }
  if (!info.kernel.kernel_type) {
    diagnostics.append(QStringLiteral("Kernel type is unavailable"));
  }
  if (!info.kernel.kernel_version) {
    diagnostics.append(QStringLiteral("Kernel version is unavailable"));
  }
  if (!info.cpu.architecture) {
    diagnostics.append(QStringLiteral("CPU architecture is unavailable"));
  }
  if (!info.cpu.logical_cpu_count) {
    diagnostics.append(QStringLiteral("Logical CPU count is unavailable"));
  }
  if (!info.memory.total_bytes) {
    diagnostics.append(QStringLiteral("Total memory is unavailable"));
  }
}

}  // namespace

LinuxSysInfoCollector::LinuxSysInfoCollector(std::shared_ptr<const LinuxPlatformAccess> access)
    : access_(access ? std::move(access) : std::make_shared<NativeLinuxPlatformAccess>()) {}

SysInfoCollectionResult LinuxSysInfoCollector::collect() const {
  const LinuxPlatformValues values = access_->values();
  protocol::SystemInfo info;
  info.host.host_name = cleanString(values.host_name);
  if (info.host.host_name) {
    info.host.host_name = info.host.host_name->section('.', 0, 0);
  }
  info.os.os_family = QStringLiteral("linux");
  info.os.os_id = cleanString(values.os_id);
  info.os.os_version = cleanString(values.os_version);
  info.os.os_pretty_name = cleanString(values.os_pretty_name);
  info.kernel.kernel_type = cleanString(values.kernel_type);
  info.kernel.kernel_version = cleanString(values.kernel_version);
  info.cpu.architecture = normalizedArchitecture(values.architecture);
  if (values.logical_cpu_count.value_or(0) > 0 && values.logical_cpu_count.value() <= kMaximumCpuCount) {
    info.cpu.logical_cpu_count = values.logical_cpu_count;
  }
  info.cpu.physical_core_count = physicalCoreCount(*access_);

  QStringList diagnostics;
  if (const auto meminfo = access_->readFile(QStringLiteral("/proc/meminfo"), kSmallFileLimit); meminfo.has_value()) {
    info.memory.total_bytes = memoryBytes(*meminfo);
  }

  if (const auto manufacturer = access_->readFile(QStringLiteral("/sys/class/dmi/id/sys_vendor"), kSmallFileLimit)) {
    info.hardware.manufacturer = cleanString(QString::fromUtf8(*manufacturer));
  }
  if (const auto model = access_->readFile(QStringLiteral("/sys/class/dmi/id/product_name"), kSmallFileLimit)) {
    info.hardware.model = cleanString(QString::fromUtf8(*model));
  }
  const auto cpuinfo = access_->readFile(QStringLiteral("/proc/cpuinfo"), kCpuInfoLimit);
  if (cpuinfo) {
    info.cpu.vendor = cpuInfoValue(*cpuinfo, QByteArrayLiteral("vendor_id"));
    info.cpu.model = cpuInfoValue(*cpuinfo, QByteArrayLiteral("model name"));
  }

  const auto compatible_file =
      access_->readFile(QStringLiteral("/sys/firmware/devicetree/base/compatible"), kSmallFileLimit);
  const QStringList compatible = compatible_file ? compatibleIds(*compatible_file) : QStringList{};
  const bool is_pi = std::ranges::any_of(
      compatible, [](const QString& identifier) { return identifier.startsWith(QStringLiteral("raspberrypi,")); });
  if (is_pi) {
    info.hardware.manufacturer = QStringLiteral("Raspberry Pi");
    info.hardware.compatible_ids = compatible;
    if (const auto model = access_->readFile(QStringLiteral("/sys/firmware/devicetree/base/model"), kSmallFileLimit)) {
      info.hardware.model = cleanString(QString::fromUtf8(*model).remove(QChar::Null));
    }
    if (cpuinfo) {
      info.hardware.board_revision = boardRevision(*cpuinfo);
    }
    for (const QString& identifier : compatible) {
      if (identifier.startsWith(QStringLiteral("brcm,bcm"))) {
        info.cpu.vendor = QStringLiteral("Broadcom");
        info.cpu.model = identifier.sliced(5).toUpper();
        break;
      }
    }
    if (!info.hardware.model) {
      diagnostics.append(QStringLiteral("Raspberry Pi model is unavailable"));
    }
    if (!info.hardware.board_revision) {
      diagnostics.append(QStringLiteral("Raspberry Pi board revision is unavailable"));
    }
  } else {
    diagnostics.append(QStringLiteral("Raspberry Pi Device Tree enrichment is unavailable"));
  }

  addMissingBaselineDiagnostics(info, diagnostics);
  return {.info = std::move(info), .diagnostics = std::move(diagnostics)};
}

}  // namespace dashboard::sysinfo
