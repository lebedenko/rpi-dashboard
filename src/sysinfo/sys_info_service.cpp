#include "sysinfo/sys_info_service.h"

#include <QtConcurrentRun>

#include <utility>

namespace dashboard::sysinfo {

namespace {
template <typename T>
QVariant optionalVariant(const std::optional<T>& value) {
  return value ? QVariant::fromValue(*value) : QVariant{};
}
}  // namespace

SysInfoService::SysInfoService(std::shared_ptr<const SysInfoCollector> collector, QObject* parent)
    : QObject(parent), collector_(std::move(collector)) {
  Q_ASSERT(collector_);
  connect(&watcher_, &QFutureWatcherBase::finished, this, &SysInfoService::collectionFinished);
  QMetaObject::invokeMethod(this, &SysInfoService::refresh, Qt::QueuedConnection);
}

SysInfoService::State SysInfoService::state() const { return state_; }

protocol::SystemInfo SysInfoService::currentInfo() const { return current_info_; }

QDateTime SysInfoService::lastSuccessUtc() const { return last_success_utc_; }

QStringList SysInfoService::diagnostics() const { return diagnostics_; }

QVariant SysInfoService::hostname() const { return optionalVariant(current_info_.host.host_name); }
QVariant SysInfoService::osFamily() const { return optionalVariant(current_info_.os.os_family); }
QVariant SysInfoService::osId() const { return optionalVariant(current_info_.os.os_id); }
QVariant SysInfoService::osVersion() const { return optionalVariant(current_info_.os.os_version); }
QVariant SysInfoService::osPrettyName() const { return optionalVariant(current_info_.os.os_pretty_name); }
QVariant SysInfoService::kernelType() const { return optionalVariant(current_info_.kernel.kernel_type); }
QVariant SysInfoService::kernelVersion() const { return optionalVariant(current_info_.kernel.kernel_version); }
QVariant SysInfoService::architecture() const { return optionalVariant(current_info_.cpu.architecture); }
QVariant SysInfoService::hardwareManufacturer() const { return optionalVariant(current_info_.hardware.manufacturer); }
QVariant SysInfoService::hardwareModel() const { return optionalVariant(current_info_.hardware.model); }
QVariant SysInfoService::cpuVendor() const { return optionalVariant(current_info_.cpu.vendor); }
QVariant SysInfoService::cpuModel() const { return optionalVariant(current_info_.cpu.model); }
QVariant SysInfoService::physicalCoreCount() const { return optionalVariant(current_info_.cpu.physical_core_count); }
QVariant SysInfoService::logicalCpuCount() const { return optionalVariant(current_info_.cpu.logical_cpu_count); }
QVariant SysInfoService::totalMemoryBytes() const { return optionalVariant(current_info_.memory.total_bytes); }

void SysInfoService::refresh() {
  if (state_ == State::Collecting) {
    return;
  }
  setState(State::Collecting);
  const std::shared_ptr<const SysInfoCollector> collector = collector_;
  watcher_.setFuture(QtConcurrent::run([collector] { return collector->collect(); }));
}

void SysInfoService::collectionFinished() {
  SysInfoCollectionResult result = watcher_.result();
  if (diagnostics_ != result.diagnostics) {
    diagnostics_ = std::move(result.diagnostics);
    emit diagnosticsChanged();
  }

  if (!result.info.hasAnyValue()) {
    if (diagnostics_.isEmpty()) {
      diagnostics_.append(QStringLiteral("System information collection produced no usable values"));
      emit diagnosticsChanged();
    }
    setState(State::Error);
    return;
  }

  current_info_ = std::move(result.info);
  emit currentInfoChanged();
  last_success_utc_ = QDateTime::currentDateTimeUtc();
  emit lastSuccessUtcChanged();
  setState(current_info_.hasAllBaselineFields() ? State::Ready : State::Partial);
}

void SysInfoService::setState(State state) {
  if (state_ == state) {
    return;
  }
  state_ = state;
  emit stateChanged();
}

}  // namespace dashboard::sysinfo
