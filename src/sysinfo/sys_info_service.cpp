#include "sysinfo/sys_info_service.h"

#include <QtConcurrentRun>

#include <utility>

namespace dashboard::sysinfo {

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
