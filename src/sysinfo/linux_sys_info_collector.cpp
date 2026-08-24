#include "sysinfo/linux_sys_info_collector.h"

#include <QFile>
#include <QSysInfo>

#include <algorithm>
#include <limits>
#include <ranges>
#include <unistd.h>
#include <utility>

namespace dashboard::sysinfo {
namespace {

constexpr qsizetype kSmallFileLimit = 64 * 1024;
constexpr qsizetype kCpuInfoLimit = 1024 * 1024;

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
  if (values.logical_cpu_count.value_or(0) > 0) {
    info.cpu.logical_cpu_count = values.logical_cpu_count;
  }

  QStringList diagnostics;
  if (const auto meminfo = access_->readFile(QStringLiteral("/proc/meminfo"), kSmallFileLimit); meminfo.has_value()) {
    info.memory.total_bytes = memoryBytes(*meminfo);
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
    if (const auto cpuinfo = access_->readFile(QStringLiteral("/proc/cpuinfo"), kCpuInfoLimit)) {
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
