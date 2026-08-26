#pragma once

#include "projects/project_health.h"

#include <QAbstractListModel>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QTimer>

#include <functional>
#include <memory>

namespace dashboard::projects {

[[nodiscard]] bool shouldReadReplyBody(QNetworkReply::NetworkError error, int http_status, bool has_cached_body);

class ProjectsListModel final : public QAbstractListModel {
  Q_OBJECT

 public:
  // QAbstractItemModel roles are unscoped integers by Qt API convention.
  // NOLINTNEXTLINE(cppcoreguidelines-use-enum-class,performance-enum-size)
  enum Role { KeyRole = Qt::UserRole + 1, NameRole, BranchRole, HealthRole, StatusRole, DetailRole };

  struct Row {
    QString key;
    QString name;
    QString branch;
    QString health;
    QString status;
    QVariantMap detail;
  };

  explicit ProjectsListModel(QObject* parent = nullptr);
  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
  void replace(QList<Row> rows);
  [[nodiscard]] const QList<Row>& rows() const;

 private:
  QList<Row> rows_;
};

class ProjectsService final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QAbstractItemModel* projectModel READ projectModel CONSTANT)
  Q_PROPERTY(QAbstractItemModel* stageModel READ stageModel CONSTANT)
  Q_PROPERTY(QAbstractItemModel* runHistoryModel READ runHistoryModel CONSTANT)
  Q_PROPERTY(int selectedProjectIndex READ selectedProjectIndex NOTIFY selectedProjectChanged)
  Q_PROPERTY(QString selectedRepository READ selectedRepository NOTIFY selectedProjectChanged)
  Q_PROPERTY(QString selectedBranch READ selectedBranch NOTIFY selectedProjectChanged)
  Q_PROPERTY(QString selectedRevision READ selectedRevision NOTIFY selectedProjectChanged)
  Q_PROPERTY(QString selectedRun READ selectedRun NOTIFY selectedProjectChanged)
  Q_PROPERTY(QString selectedRunAge READ selectedRunAge NOTIFY selectedProjectChanged)
  Q_PROPERTY(QString duration READ duration NOTIFY selectedProjectChanged)
  Q_PROPERTY(QString jobsSummary READ jobsSummary NOTIFY selectedProjectChanged)
  Q_PROPERTY(QString artifactSize READ artifactSize NOTIFY selectedProjectChanged)
  Q_PROPERTY(QString deployStatus READ deployStatus NOTIFY selectedProjectChanged)
  Q_PROPERTY(int trackedCount READ trackedCount NOTIFY snapshotChanged)
  Q_PROPERTY(int runningCount READ runningCount NOTIFY snapshotChanged)
  Q_PROPERTY(int failedCount READ failedCount NOTIFY snapshotChanged)
  Q_PROPERTY(QString aggregateHealth READ aggregateHealth NOTIFY snapshotChanged)
  Q_PROPERTY(int onlineRunnerCount READ onlineRunnerCount NOTIFY snapshotChanged)
  Q_PROPERTY(int totalRunnerCount READ totalRunnerCount NOTIFY snapshotChanged)
  Q_PROPERTY(QString state READ state NOTIFY stateChanged)
  Q_PROPERTY(bool stale READ stale NOTIFY stateChanged)
  Q_PROPERTY(QDateTime lastSuccessUtc READ lastSuccessUtc NOTIFY stateChanged)
  Q_PROPERTY(QString diagnostics READ diagnostics NOTIFY stateChanged)

 public:
  explicit ProjectsService(QString owner, QByteArray token = {}, QObject* parent = nullptr);
  [[nodiscard]] QAbstractItemModel* projectModel();
  [[nodiscard]] QAbstractItemModel* stageModel();
  [[nodiscard]] QAbstractItemModel* runHistoryModel();
  [[nodiscard]] int selectedProjectIndex() const;
  [[nodiscard]] QString selectedRepository() const;
  [[nodiscard]] QString selectedBranch() const;
  [[nodiscard]] QString selectedRevision() const;
  [[nodiscard]] QString selectedRun() const;
  [[nodiscard]] QString selectedRunAge() const;
  [[nodiscard]] QString duration() const;
  [[nodiscard]] QString jobsSummary() const;
  [[nodiscard]] QString artifactSize() const;
  [[nodiscard]] QString deployStatus() const;
  [[nodiscard]] int trackedCount() const;
  [[nodiscard]] int runningCount() const;
  [[nodiscard]] int failedCount() const;
  [[nodiscard]] QString aggregateHealth() const;
  [[nodiscard]] int onlineRunnerCount() const;
  [[nodiscard]] int totalRunnerCount() const;
  [[nodiscard]] QString state() const;
  [[nodiscard]] bool stale() const;
  [[nodiscard]] QDateTime lastSuccessUtc() const;
  [[nodiscard]] QString diagnostics() const;

  Q_INVOKABLE void refresh();
  Q_INVOKABLE void selectProject(int index);

 signals:
  void selectedProjectChanged();
  void snapshotChanged();
  void stateChanged();

 private:
  struct Repository {
    QString key;
    QString name;
    QString branch;
  };
  struct Request {
    QUrl url;
    std::function<void(const QJsonDocument&, const QNetworkReply*)> success;
    std::function<void(QString)> failure;
  };

  void discoverRepositories(int page, QList<Repository> repositories);
  void discoverPrivateRepositories(int page, QList<Repository> repositories);
  void loadRuns(QList<Repository> repositories);
  void loadRunners();
  void loadSelectedDetails();
  void request(Request request);
  void startRequests();
  void finishRefresh();
  void failRefresh(const QString& diagnostic);
  void setState(QString state, QString diagnostic = {});
  [[nodiscard]] const ProjectsListModel::Row* selectedRow() const;
  static QString ageText(const QDateTime& timestamp);
  static QString bytesText(qint64 bytes);

  QString owner_;
  QByteArray token_;
  QNetworkAccessManager network_;
  ProjectsListModel projects_;
  ProjectsListModel stages_;
  ProjectsListModel history_;
  QTimer poll_timer_;
  QQueue<Request> requests_;
  int active_requests_{0};
  int pending_run_requests_{0};
  int pending_runner_requests_{0};
  int selected_index_{-1};
  QString selected_key_;
  QString state_{QStringLiteral("loading")};
  QString diagnostics_;
  QDateTime last_success_;
  bool stale_{false};
  int online_runners_{-1};
  int total_runners_{-1};
  QSet<qint64> runner_ids_;
  QSet<qint64> online_runner_ids_;
  QHash<QString, QByteArray> etags_;
  QHash<QString, QByteArray> cached_bodies_;
};

}  // namespace dashboard::projects
