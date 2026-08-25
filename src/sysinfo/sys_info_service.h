#pragma once

#include "sysinfo/sys_info_collector.h"

#include <QDateTime>
#include <QFutureWatcher>
#include <QObject>
#include <QVariant>

#include <cstdint>
#include <memory>

namespace dashboard::sysinfo {

class SysInfoService final : public QObject {
  Q_OBJECT
  Q_PROPERTY(State state READ state NOTIFY stateChanged)
  Q_PROPERTY(dashboard::protocol::SystemInfo currentInfo READ currentInfo NOTIFY currentInfoChanged)
  Q_PROPERTY(QDateTime lastSuccessUtc READ lastSuccessUtc NOTIFY lastSuccessUtcChanged)
  Q_PROPERTY(QStringList diagnostics READ diagnostics NOTIFY diagnosticsChanged)
  Q_PROPERTY(QVariant hostname READ hostname NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant osFamily READ osFamily NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant osId READ osId NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant osVersion READ osVersion NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant osPrettyName READ osPrettyName NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant kernelType READ kernelType NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant kernelVersion READ kernelVersion NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant architecture READ architecture NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant hardwareManufacturer READ hardwareManufacturer NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant hardwareModel READ hardwareModel NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant cpuVendor READ cpuVendor NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant cpuModel READ cpuModel NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant physicalCoreCount READ physicalCoreCount NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant logicalCpuCount READ logicalCpuCount NOTIFY currentInfoChanged)
  Q_PROPERTY(QVariant totalMemoryBytes READ totalMemoryBytes NOTIFY currentInfoChanged)

 public:
  enum class State : std::uint8_t { Idle, Collecting, Ready, Partial, Error };
  Q_ENUM(State)

  explicit SysInfoService(std::shared_ptr<const SysInfoCollector> collector, QObject* parent = nullptr);

  [[nodiscard]] State state() const;
  [[nodiscard]] protocol::SystemInfo currentInfo() const;
  [[nodiscard]] QDateTime lastSuccessUtc() const;
  [[nodiscard]] QStringList diagnostics() const;
  [[nodiscard]] QVariant hostname() const;
  [[nodiscard]] QVariant osFamily() const;
  [[nodiscard]] QVariant osId() const;
  [[nodiscard]] QVariant osVersion() const;
  [[nodiscard]] QVariant osPrettyName() const;
  [[nodiscard]] QVariant kernelType() const;
  [[nodiscard]] QVariant kernelVersion() const;
  [[nodiscard]] QVariant architecture() const;
  [[nodiscard]] QVariant hardwareManufacturer() const;
  [[nodiscard]] QVariant hardwareModel() const;
  [[nodiscard]] QVariant cpuVendor() const;
  [[nodiscard]] QVariant cpuModel() const;
  [[nodiscard]] QVariant physicalCoreCount() const;
  [[nodiscard]] QVariant logicalCpuCount() const;
  [[nodiscard]] QVariant totalMemoryBytes() const;

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
