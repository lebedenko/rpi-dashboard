#include "protocol/device_snapshot.h"
#include "sysinfo/linux_sys_info_collector.h"
#include "sysinfo/sys_info_service.h"
#include "sysmetrics/linux_sys_metrics_collector.h"
#include "sysmetrics/sys_metrics_service.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHostInfo>
#include <QNetworkDatagram>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUdpSocket>

#include <cstdlib>
#include <memory>

namespace {
// NOLINTBEGIN(readability-braces-around-statements, readability-identifier-length,
// modernize-use-designated-initializers, clang-analyzer-cplusplus.NewDeleteLeaks)
std::optional<QUuid> persistentDeviceId() {
  const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  const QString path = QDir(directory).filePath(QStringLiteral("device-id"));
  QFile input(path);
  if (input.exists()) {
    if (!input.open(QIODevice::ReadOnly)) return std::nullopt;
    const QUuid id(QString::fromUtf8(input.readAll()).trimmed());
    return id.isNull() ? std::nullopt : std::optional(id);
  }
  if (!QDir().mkpath(directory)) return std::nullopt;
  const QUuid id = QUuid::createUuid();
  const QByteArray bytes = id.toString(QUuid::WithoutBraces).toUtf8();
  QSaveFile output(path);
  if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit())
    return std::nullopt;
  return id;
}

class TelemetryAgent final : public QObject {
 public:
  TelemetryAgent(QUuid device_id, QString display_name, int interval, QHostAddress destination, quint16 port,
                 QObject* parent = nullptr)
      : QObject(parent),
        device_id_(device_id),
        instance_id_(QUuid::createUuid()),
        display_name_(std::move(display_name)),
        interval_(interval),
        destination_(std::move(destination)),
        port_(port),
        info_(std::make_shared<dashboard::sysinfo::LinuxSysInfoCollector>()),
        metrics_(std::make_shared<dashboard::sysmetrics::LinuxSysMetricsCollector>(), interval * 1000) {
    socket_.bind(QHostAddress::AnyIPv4, 0);
    connect(&socket_, &QUdpSocket::readyRead, this, [this] {
      while (socket_.hasPendingDatagrams()) {
        const auto decoded = dashboard::protocol::decodeMessage(socket_.receiveDatagram().data());
        if (!decoded.message) continue;
        const auto* response = std::get_if<dashboard::protocol::RegistrationResult>(&*decoded.message);
        if (!response || response->device_id != device_id_ || response->instance_id != instance_id_) continue;
        if (response->accepted)
          registered_ = true;
        else if (response->reason == QStringLiteral("unsupported_version"))
          QCoreApplication::exit(EXIT_FAILURE);
      }
    });
    connect(&hello_timer_, &QTimer::timeout, this, [this] { sendHello(); });
    hello_timer_.start(10000);
    connect(&info_, &dashboard::sysinfo::SysInfoService::currentInfoChanged, this, [this] { sendHello(); });
    connect(&metrics_, &dashboard::sysmetrics::SysMetricsService::stateChanged, this, [this] { sendSnapshot(); });
  }

 private:
  void sendHello() {
    const auto info = info_.currentInfo();
    if (!info.hasAnyValue()) return;
    QString name = display_name_;
    if (name.isEmpty() && info.host.host_name) name = *info.host.host_name;
    if (name.isEmpty()) name = QStringLiteral("remote-device");
    socket_.writeDatagram(
        dashboard::protocol::encodeMessage(dashboard::protocol::Hello{device_id_, instance_id_, name, interval_, info}),
        destination_, port_);
  }
  void sendSnapshot() {
    using State = dashboard::sysmetrics::SysMetricsService::State;
    if (!registered_ || !info_.currentInfo().hasAnyValue() || metrics_.state() == State::Idle ||
        metrics_.state() == State::Collecting)
      return;
    dashboard::protocol::DeviceSnapshot snapshot{device_id_,  instance_id_,        interval_,
                                                 sequence_++, info_.currentInfo(), std::nullopt};
    if (metrics_.state() == State::Ready || metrics_.state() == State::Partial)
      snapshot.metrics = metrics_.currentMetrics();
    socket_.writeDatagram(dashboard::protocol::encodeMessage(snapshot), destination_, port_);
  }

  QUuid device_id_;
  QUuid instance_id_;
  QString display_name_;
  int interval_;
  QHostAddress destination_;
  quint16 port_;
  QUdpSocket socket_;
  QTimer hello_timer_;
  dashboard::sysinfo::SysInfoService info_;
  dashboard::sysmetrics::SysMetricsService metrics_;
  bool registered_{false};
  quint64 sequence_{0};
};
}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("rpi-dashboard-agent"));
  QCoreApplication::setOrganizationName(QStringLiteral("rpi-dashboard"));
  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("Remote telemetry agent"));
  parser.addHelpOption();
  const QCommandLineOption host(QStringLiteral("dashboard-host"), QStringLiteral("Dashboard hostname or address."),
                                QStringLiteral("host"));
  const QCommandLineOption port(QStringLiteral("dashboard-port"), QStringLiteral("Dashboard UDP port."),
                                QStringLiteral("port"), QStringLiteral("51337"));
  const QCommandLineOption interval(QStringLiteral("interval"), QStringLiteral("Collection interval in seconds (1-5)."),
                                    QStringLiteral("seconds"), QStringLiteral("1"));
  const QCommandLineOption device(QStringLiteral("device-id"), QStringLiteral("Override persistent device UUID."),
                                  QStringLiteral("uuid"));
  const QCommandLineOption name(QStringLiteral("display-name"), QStringLiteral("Device display name."),
                                QStringLiteral("name"));
  parser.addOptions({host, port, interval, device, name});
  parser.process(app);
  bool port_ok = false;
  bool interval_ok = false;
  const int port_value = parser.value(port).toInt(&port_ok);
  const int interval_value = parser.value(interval).toInt(&interval_ok);
  if (!parser.isSet(host) || !port_ok || port_value < 1 || port_value > 65535 || !interval_ok || interval_value < 1 ||
      interval_value > 5 || parser.value(name).toUtf8().size() > 128)
    parser.showHelp(EXIT_FAILURE);
  std::optional<QUuid> device_id;
  if (parser.isSet(device)) {
    const QUuid parsed(parser.value(device));
    if (parsed.isNull()) parser.showHelp(EXIT_FAILURE);
    device_id = parsed;
  } else {
    device_id = persistentDeviceId();
  }
  if (!device_id) {
    qCritical("Unable to establish persistent device identity");
    return EXIT_FAILURE;
  }
  QHostInfo::lookupHost(
      parser.value(host), &app,
      [&app, device_id, display_name = parser.value(name), interval_value, port_value](const QHostInfo& result) {
        if (result.error() != QHostInfo::NoError || result.addresses().isEmpty()) {
          qCritical("Dashboard hostname resolution failed");
          QCoreApplication::exit(EXIT_FAILURE);
          return;
        }
        new TelemetryAgent(*device_id, display_name, interval_value, result.addresses().first(),
                           static_cast<quint16>(port_value), &app);
      });
  return QCoreApplication::exec();
}
// NOLINTEND(readability-braces-around-statements, readability-identifier-length,
// modernize-use-designated-initializers, clang-analyzer-cplusplus.NewDeleteLeaks)
