#pragma once

#include "sysinfo/sys_info_collector.h"

#include <QDateTime>
#include <QFutureWatcher>
#include <QObject>

#include <cstdint>
#include <memory>

namespace dashboard::sysinfo {

class SysInfoService final : public QObject {
  Q_OBJECT
  Q_PROPERTY(State state READ state NOTIFY stateChanged)
  Q_PROPERTY(dashboard::protocol::SystemInfo currentInfo READ currentInfo NOTIFY currentInfoChanged)
  Q_PROPERTY(QDateTime lastSuccessUtc READ lastSuccessUtc NOTIFY lastSuccessUtcChanged)
  Q_PROPERTY(QStringList diagnostics READ diagnostics NOTIFY diagnosticsChanged)

 public:
  enum class State : std::uint8_t { Idle, Collecting, Ready, Partial, Error };
  Q_ENUM(State)

  explicit SysInfoService(std::shared_ptr<const SysInfoCollector> collector, QObject* parent = nullptr);

  [[nodiscard]] State state() const;
  [[nodiscard]] protocol::SystemInfo currentInfo() const;
  [[nodiscard]] QDateTime lastSuccessUtc() const;
  [[nodiscard]] QStringList diagnostics() const;

 public slots:
  void refresh();

 signals:
  void stateChanged();
  void currentInfoChanged();
  void lastSuccessUtcChanged();
  void diagnosticsChanged();

 private slots:
  void collectionFinished();

 private:
  void setState(State state);

  std::shared_ptr<const SysInfoCollector> collector_;
  QFutureWatcher<SysInfoCollectionResult> watcher_;
  State state_{State::Idle};
  protocol::SystemInfo current_info_;
  QDateTime last_success_utc_;
  QStringList diagnostics_;
};

}  // namespace dashboard::sysinfo
