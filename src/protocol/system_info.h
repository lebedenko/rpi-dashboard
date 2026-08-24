#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QtTypes>

#include <optional>

namespace dashboard::protocol {

struct SystemInfo {
  struct Host {
    std::optional<QString> host_name;
  } host;

  struct OperatingSystem {
    std::optional<QString> os_family;
    std::optional<QString> os_id;
    std::optional<QString> os_version;
    std::optional<QString> os_pretty_name;
  } os;

  struct Kernel {
    std::optional<QString> kernel_type;
    std::optional<QString> kernel_version;
  } kernel;

  struct Hardware {
    std::optional<QString> manufacturer;
    std::optional<QString> model;
    std::optional<QString> board_revision;
    std::optional<QStringList> compatible_ids;
  } hardware;

  struct Cpu {
    std::optional<QString> architecture;
    std::optional<QString> vendor;
    std::optional<QString> model;
    std::optional<quint32> logical_cpu_count;
    std::optional<quint32> physical_core_count;
  } cpu;

  struct Memory {
    std::optional<quint64> total_bytes;
  } memory;

  [[nodiscard]] bool hasAnyValue() const;
  [[nodiscard]] bool hasAllBaselineFields() const;
};

}  // namespace dashboard::protocol

Q_DECLARE_METATYPE(dashboard::protocol::SystemInfo)
