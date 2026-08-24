#pragma once

#include "sysinfo/sys_info_collector.h"

#include <QByteArray>
#include <QString>

#include <memory>
#include <optional>

namespace dashboard::sysinfo {

struct LinuxPlatformValues {
  QString host_name;
  QString os_id;
  QString os_version;
  QString os_pretty_name;
  QString kernel_type;
  QString kernel_version;
  QString architecture;
  std::optional<quint32> logical_cpu_count;
};

class LinuxPlatformAccess {
 public:
  LinuxPlatformAccess() = default;
  virtual ~LinuxPlatformAccess() = default;
  LinuxPlatformAccess(const LinuxPlatformAccess&) = default;
  LinuxPlatformAccess& operator=(const LinuxPlatformAccess&) = default;
  LinuxPlatformAccess(LinuxPlatformAccess&&) = default;
  LinuxPlatformAccess& operator=(LinuxPlatformAccess&&) = default;
  [[nodiscard]] virtual LinuxPlatformValues values() const = 0;
  [[nodiscard]] virtual std::optional<QByteArray> readFile(const QString& path, qsizetype maximum_bytes) const = 0;
};

class LinuxSysInfoCollector final : public SysInfoCollector {
 public:
  explicit LinuxSysInfoCollector(std::shared_ptr<const LinuxPlatformAccess> access = {});
  [[nodiscard]] SysInfoCollectionResult collect() const override;

 private:
  std::shared_ptr<const LinuxPlatformAccess> access_;
};

}  // namespace dashboard::sysinfo
