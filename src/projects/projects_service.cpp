#include "projects/projects_service.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <algorithm>
#include <limits>

namespace dashboard::projects {
// Qt network orchestration is callback-heavy and intentionally uses Qt-sized containers and model-role aggregates.
// NOLINTBEGIN
namespace {
constexpr int kRequestTimeoutMs = 10'000;
constexpr int kMaximumConcurrentRequests = 4;
constexpr int kAuthenticatedPollMs = 60 * 1000;
constexpr int kAnonymousMinimumPollMs = 15 * 60 * 1000;
constexpr int kAnonymousRequestBudgetPerHour = 50;

QDateTime date(const QJsonValue& value) { return QDateTime::fromString(value.toString(), Qt::ISODate); }

QString conclusionHealth(const QJsonObject& object) {
  return healthKey(healthForRun(object.value(QStringLiteral("status")).toString(),
                                object.value(QStringLiteral("conclusion")).toString(),
                                date(object.value(QStringLiteral("updated_at")))));
}

bool isDeployName(const QString& name) {
  const auto lower = name.toLower();
  return lower.contains(QStringLiteral("deploy")) || lower.contains(QStringLiteral("release")) ||
         lower.contains(QStringLiteral("publish"));
}
}  // namespace

bool shouldReadReplyBody(QNetworkReply::NetworkError error, int http_status, bool has_cached_body) {
  const auto successful_status = http_status >= 200 && http_status < 300;
  return error == QNetworkReply::NoError && successful_status && !(http_status == 304 && has_cached_body);
}

int pollIntervalMs(bool authenticated, int request_count) {
  if (authenticated) return kAuthenticatedPollMs;
  const auto measured_requests = std::max(1, request_count);
  constexpr qint64 milliseconds_per_hour = 60LL * 60 * 1000;
  const auto budgeted_interval =
      (static_cast<qint64>(measured_requests) * milliseconds_per_hour + kAnonymousRequestBudgetPerHour - 1) /
      kAnonymousRequestBudgetPerHour;
  return static_cast<int>(
      std::min<qint64>(std::max<qint64>(kAnonymousMinimumPollMs, budgeted_interval), std::numeric_limits<int>::max()));
}

int rateLimitBackoffMs(int http_status, const QByteArray& retry_after, const QByteArray& rate_limit_remaining,
                       const QByteArray& rate_limit_reset, qint64 current_epoch_seconds) {
  qint64 delay_ms = 0;
  if (http_status == 403 || http_status == 429) {
    bool retry_ok = false;
    const auto retry_seconds = retry_after.toLongLong(&retry_ok);
    if (retry_ok && retry_seconds > 0) {
      delay_ms = std::min<qint64>(retry_seconds, std::numeric_limits<int>::max() / 1000) * 1000;
    }
  }

  bool remaining_ok = false;
  const auto remaining = rate_limit_remaining.toLongLong(&remaining_ok);
  if (remaining_ok && remaining == 0) {
    bool reset_ok = false;
    const auto reset = rate_limit_reset.toLongLong(&reset_ok);
    if (reset_ok && reset > current_epoch_seconds) {
      const auto reset_seconds =
          std::min<qint64>(reset - current_epoch_seconds, std::numeric_limits<int>::max() / 1000);
      delay_ms = std::max(delay_ms, reset_seconds * 1000);
    }
  }
  return static_cast<int>(std::min<qint64>(delay_ms, std::numeric_limits<int>::max()));
}

int countSuccessfulHistory(const QVariantList& history) {
  return std::ranges::count_if(
      history, [](const auto& item) { return item.toMap().value(QStringLiteral("successful")).toBool(); });
}

ProjectsListModel::ProjectsListModel(QObject* parent) : QAbstractListModel(parent) {}

int ProjectsListModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : rows_.size(); }

QVariant ProjectsListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
    return {};
  }
  const auto& row = rows_.at(index.row());
  switch (role) {
    case KeyRole:
      return row.key;
    case NameRole:
      return row.name;
    case BranchRole:
      return row.branch;
    case AgeRole:
      return row.age;
    case HealthRole:
      return row.health;
    case StatusRole:
      return row.status;
    case DetailRole:
      return row.detail;
    default:
      return {};
  }
}

QHash<int, QByteArray> ProjectsListModel::roleNames() const {
  return {{KeyRole, "key"},       {NameRole, "name"},     {BranchRole, "branch"}, {AgeRole, "age"},
          {HealthRole, "health"}, {StatusRole, "status"}, {DetailRole, "detail"}};
}

void ProjectsListModel::replace(QList<Row> rows) {
  beginResetModel();
  rows_ = std::move(rows);
  endResetModel();
}

const QList<ProjectsListModel::Row>& ProjectsListModel::rows() const { return rows_; }

ProjectsService::ProjectsService(QString owner, QByteArray token, QObject* parent)
    : QObject(parent),
      owner_(std::move(owner)),
      token_(std::move(token)),
      projects_(this),
      stages_(this),
      history_(this) {
  poll_timer_.setSingleShot(false);
  poll_timer_.setInterval(pollIntervalMs(!token_.isEmpty(), 0));
  connect(&poll_timer_, &QTimer::timeout, this, &ProjectsService::refresh);
  poll_timer_.start();
  QTimer::singleShot(0, this, &ProjectsService::refresh);
}

QAbstractItemModel* ProjectsService::projectModel() { return &projects_; }
QAbstractItemModel* ProjectsService::stageModel() { return &stages_; }
QAbstractItemModel* ProjectsService::runHistoryModel() { return &history_; }
int ProjectsService::selectedProjectIndex() const { return selected_index_; }
QString ProjectsService::state() const { return state_; }
bool ProjectsService::stale() const { return stale_; }
QDateTime ProjectsService::lastSuccessUtc() const { return last_success_; }
QString ProjectsService::diagnostics() const { return diagnostics_; }
int ProjectsService::trackedCount() const { return projects_.rowCount(); }
int ProjectsService::onlineRunnerCount() const { return online_runners_; }
int ProjectsService::totalRunnerCount() const { return total_runners_; }

const ProjectsListModel::Row* ProjectsService::selectedRow() const {
  const auto& rows = projects_.rows();
  return selected_index_ >= 0 && selected_index_ < rows.size() ? &rows.at(selected_index_) : nullptr;
}

QString ProjectsService::selectedRepository() const { return selectedRow() ? selectedRow()->name : QString{}; }
QString ProjectsService::selectedBranch() const { return selectedRow() ? selectedRow()->branch : QString{}; }
QString ProjectsService::selectedRevision() const {
  return selectedRow() ? selectedRow()->detail.value(QStringLiteral("revision")).toString() : QString{};
}
QString ProjectsService::selectedRun() const {
  return selectedRow() ? selectedRow()->detail.value(QStringLiteral("runLabel")).toString() : QString{};
}
QString ProjectsService::selectedRunAge() const {
  return selectedRow() ? ageText(selectedRow()->detail.value(QStringLiteral("updatedAt")).toDateTime()) : QString{};
}
QString ProjectsService::selectedHealth() const {
  return selectedRow() ? selectedRow()->health : QStringLiteral("unknown");
}
int ProjectsService::historySuccessfulCount() const {
  return selectedRow() ? countSuccessfulHistory(selectedRow()->detail.value(QStringLiteral("history")).toList()) : 0;
}
int ProjectsService::historyCount() const {
  return selectedRow() ? selectedRow()->detail.value(QStringLiteral("history")).toList().size() : 0;
}
QString ProjectsService::duration() const {
  return selectedRow() ? selectedRow()->detail.value(QStringLiteral("duration")).toString() : QString{};
}
QString ProjectsService::jobsSummary() const {
  return selectedRow() ? selectedRow()->detail.value(QStringLiteral("jobsSummary")).toString() : QString{};
}
QString ProjectsService::artifactSize() const {
  return selectedRow() ? selectedRow()->detail.value(QStringLiteral("artifactSize")).toString() : QString{};
}
QString ProjectsService::deployStatus() const {
  return selectedRow() ? selectedRow()->detail.value(QStringLiteral("deployStatus")).toString() : QString{};
}

int ProjectsService::runningCount() const {
  return std::ranges::count_if(projects_.rows(),
                               [](const auto& row) { return row.health == QStringLiteral("running"); });
}
int ProjectsService::failedCount() const {
  return std::ranges::count_if(projects_.rows(),
                               [](const auto& row) { return row.health == QStringLiteral("failed"); });
}
QString ProjectsService::aggregateHealth() const {
  if (projects_.rows().isEmpty()) {
    return QStringLiteral("unknown");
  }
  const auto worst = std::ranges::min_element(projects_.rows(), {}, [](const auto& row) {
    const auto keys = QStringList{QStringLiteral("failed"), QStringLiteral("attention"), QStringLiteral("running"),
                                  QStringLiteral("stale"),  QStringLiteral("unknown"),   QStringLiteral("healthy")};
    return keys.indexOf(row.health);
  });
  return worst->health;
}

void ProjectsService::refresh() {
  if (state_ == QStringLiteral("loading") && active_requests_ > 0) {
    return;
  }
  refresh_request_count_ = 0;
  counting_refresh_requests_ = true;
  setState(QStringLiteral("loading"));
  discoverRepositories(1, {});
}

void ProjectsService::discoverRepositories(int page, QList<Repository> repositories) {
  QUrl url(QStringLiteral("https://api.github.com/users/%1/repos").arg(owner_));
  QUrlQuery query;
  query.addQueryItem(QStringLiteral("per_page"), QStringLiteral("100"));
  query.addQueryItem(QStringLiteral("page"), QString::number(page));
  query.addQueryItem(QStringLiteral("type"), QStringLiteral("owner"));
  url.setQuery(query);
  request({url,
           [this, page, repositories = std::move(repositories)](const QJsonDocument& document,
                                                                const QNetworkReply*) mutable {
             if (!document.isArray()) {
               failRefresh(QStringLiteral("GitHub returned invalid repository data"));
               return;
             }
             const auto array = document.array();
             for (const auto& value : array) {
               const auto object = value.toObject();
               if (object.value(QStringLiteral("archived")).toBool()) {
                 continue;
               }
               repositories.push_back({object.value(QStringLiteral("full_name")).toString(),
                                       object.value(QStringLiteral("name")).toString(),
                                       object.value(QStringLiteral("default_branch")).toString()});
             }
             if (array.size() == 100) {
               discoverRepositories(page + 1, std::move(repositories));
             } else if (!token_.isEmpty()) {
               discoverPrivateRepositories(1, std::move(repositories));
             } else {
               loadRuns(std::move(repositories));
             }
           },
           [this](QString error) { failRefresh(std::move(error)); }});
}

void ProjectsService::discoverPrivateRepositories(int page, QList<Repository> repositories) {
  QUrl url(QStringLiteral("https://api.github.com/user/repos"));
  QUrlQuery query;
  query.addQueryItem(QStringLiteral("per_page"), QStringLiteral("100"));
  query.addQueryItem(QStringLiteral("page"), QString::number(page));
  query.addQueryItem(QStringLiteral("affiliation"), QStringLiteral("owner"));
  url.setQuery(query);
  auto accumulated = std::make_shared<QList<Repository>>(std::move(repositories));
  request({url,
           [this, page, accumulated](const QJsonDocument& document, const QNetworkReply*) mutable {
             if (!document.isArray()) {
               failRefresh(QStringLiteral("GitHub returned invalid private repository data"));
               return;
             }
             QSet<QString> keys;
             for (const auto& repository : *accumulated) keys.insert(repository.key);
             const auto array = document.array();
             for (const auto& value : array) {
               const auto object = value.toObject();
               if (object.value(QStringLiteral("archived")).toBool() ||
                   object.value(QStringLiteral("owner"))
                           .toObject()
                           .value(QStringLiteral("login"))
                           .toString()
                           .compare(owner_, Qt::CaseInsensitive) != 0) {
                 continue;
               }
               const auto key = object.value(QStringLiteral("full_name")).toString();
               if (!keys.contains(key)) {
                 keys.insert(key);
                 accumulated->push_back({key, object.value(QStringLiteral("name")).toString(),
                                         object.value(QStringLiteral("default_branch")).toString()});
               }
             }
             if (array.size() == 100)
               discoverPrivateRepositories(page + 1, std::move(*accumulated));
             else
               loadRuns(std::move(*accumulated));
           },
           [this, accumulated](QString) mutable { loadRuns(std::move(*accumulated)); }});
}

void ProjectsService::loadRuns(QList<Repository> repositories) {
  if (repositories.isEmpty()) {
    projects_.replace({});
    finishRefresh();
    return;
  }
  pending_run_requests_ = repositories.size();
  auto rows = std::make_shared<QList<ProjectsListModel::Row>>();
  for (const auto& repository : repositories) {
    QUrl url(QStringLiteral("https://api.github.com/repos/%1/actions/runs").arg(repository.key));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("branch"), repository.branch);
    query.addQueryItem(QStringLiteral("per_page"), QStringLiteral("20"));
    url.setQuery(query);
    request({url,
             [this, repository, rows](const QJsonDocument& document, const QNetworkReply*) {
               const auto runs = document.object().value(QStringLiteral("workflow_runs")).toArray();
               if (!runs.isEmpty()) {
                 const auto latest = runs.first().toObject();
                 QVariantList history;
                 for (const auto& value : runs) {
                   const auto run = value.toObject();
                   history.push_back(
                       QVariantMap{{QStringLiteral("health"), conclusionHealth(run)},
                                   {QStringLiteral("successful"),
                                    run.value(QStringLiteral("conclusion")).toString() == QStringLiteral("success")},
                                   {QStringLiteral("number"), run.value(QStringLiteral("run_number")).toInt()}});
                 }
                 auto sha = latest.value(QStringLiteral("head_sha")).toString();
                 sha.truncate(7);
                 const auto created = date(latest.value(QStringLiteral("created_at")));
                 const auto updated = date(latest.value(QStringLiteral("updated_at")));
                 rows->push_back({repository.key,
                                  repository.name,
                                  repository.branch,
                                  ageText(updated),
                                  conclusionHealth(latest),
                                  latest.value(QStringLiteral("status")).toString(),
                                  {{QStringLiteral("runId"), latest.value(QStringLiteral("id")).toVariant()},
                                   {QStringLiteral("revision"), sha},
                                   {QStringLiteral("runLabel"),
                                    QStringLiteral("#%1").arg(latest.value(QStringLiteral("run_number")).toInt())},
                                   {QStringLiteral("updatedAt"), updated},
                                   {QStringLiteral("createdAt"), created},
                                   {QStringLiteral("history"), history}}});
               }
               if (--pending_run_requests_ == 0) {
                 const auto ranks =
                     QStringList{QStringLiteral("failed"), QStringLiteral("attention"), QStringLiteral("running"),
                                 QStringLiteral("stale"),  QStringLiteral("unknown"),   QStringLiteral("healthy")};
                 std::ranges::stable_sort(*rows, [&](const auto& left, const auto& right) {
                   return ranks.indexOf(left.health) < ranks.indexOf(right.health);
                 });
                 projects_.replace(std::move(*rows));
                 selected_index_ = 0;
                 for (int index = 0; index < projects_.rows().size(); ++index) {
                   if (projects_.rows().at(index).key == selected_key_) {
                     selected_index_ = index;
                   }
                 }
                 selected_key_ = selectedRow() ? selectedRow()->key : QString{};
                 finishRefresh();
                 loadSelectedDetails();
               }
             },
             [this](QString error) {
               if (--pending_run_requests_ == 0) {
                 failRefresh(std::move(error));
               }
             }});
  }
}

void ProjectsService::loadRunners() {
  online_runners_ = -1;
  total_runners_ = -1;
  runner_ids_.clear();
  online_runner_ids_.clear();
  if (token_.isEmpty() || projects_.rows().isEmpty()) {
    emit snapshotChanged();
    return;
  }
  pending_runner_requests_ = projects_.rows().size();
  auto failed = std::make_shared<bool>(false);
  for (const auto& row : projects_.rows()) {
    request({QUrl(QStringLiteral("https://api.github.com/repos/%1/actions/runners?per_page=100").arg(row.key)),
             [this, failed](const QJsonDocument& document, const QNetworkReply*) {
               for (const auto& value : document.object().value(QStringLiteral("runners")).toArray()) {
                 const auto runner = value.toObject();
                 const auto id = runner.value(QStringLiteral("id")).toInteger();
                 runner_ids_.insert(id);
                 if (runner.value(QStringLiteral("status")).toString() == QStringLiteral("online"))
                   online_runner_ids_.insert(id);
               }
               if (--pending_runner_requests_ == 0) {
                 if (!*failed) {
                   total_runners_ = runner_ids_.size();
                   online_runners_ = online_runner_ids_.size();
                 }
                 emit snapshotChanged();
               }
             },
             [this, failed](QString) {
               *failed = true;
               if (--pending_runner_requests_ == 0) emit snapshotChanged();
             }});
  }
}

void ProjectsService::selectProject(int index) {
  if (index < 0 || index >= projects_.rowCount() || index == selected_index_) {
    return;
  }
  selected_index_ = index;
  selected_key_ = selectedRow()->key;
  emit selectedProjectChanged();
  loadSelectedDetails();
}

void ProjectsService::loadSelectedDetails() {
  const auto* row = selectedRow();
  if (!row) {
    stages_.replace({});
    history_.replace({});
    emit selectedProjectChanged();
    return;
  }
  const auto run_id = row->detail.value(QStringLiteral("runId")).toLongLong();
  const auto repository = row->key;
  const auto history = row->detail.value(QStringLiteral("history")).toList();
  QList<ProjectsListModel::Row> history_rows;
  for (const auto& item : history) {
    const auto entry = item.toMap();
    history_rows.push_back({QString::number(entry.value(QStringLiteral("number")).toInt()),
                            {},
                            {},
                            {},
                            entry.value(QStringLiteral("health")).toString(),
                            {},
                            {}});
  }
  history_.replace(std::move(history_rows));
  request(
      {QUrl(QStringLiteral("https://api.github.com/repos/%1/actions/runs/%2/jobs?per_page=100")
                .arg(repository)
                .arg(run_id)),
       [this](const QJsonDocument& document, const QNetworkReply*) {
         const auto jobs = document.object().value(QStringLiteral("jobs")).toArray();
         QList<ProjectsListModel::Row> cards;
         int successful = 0;
         QString deploy = QStringLiteral("—");
         const int job_count = static_cast<int>(jobs.size());
         const int direct_count = std::min(3, job_count);
         const int shown_count = job_count <= 4 ? job_count : direct_count;
         for (int index = 0; index < shown_count; ++index) {
           const auto job = jobs.at(index).toObject();
           const auto health = conclusionHealth(job);
           successful += job.value(QStringLiteral("conclusion")).toString() == QStringLiteral("success");
           if (isDeployName(job.value(QStringLiteral("name")).toString())) {
             deploy = health;
           }
           cards.push_back({QString::number(index),
                            job.value(QStringLiteral("name")).toString(),
                            {},
                            {},
                            health,
                            job.value(QStringLiteral("status")).toString(),
                            {}});
         }
         for (int index = shown_count; index < job_count; ++index) {
           successful +=
               jobs.at(index).toObject().value(QStringLiteral("conclusion")).toString() == QStringLiteral("success");
         }
         if (job_count > 4) {
           QString hidden_health = QStringLiteral("healthy");
           const QStringList ranks{QStringLiteral("failed"), QStringLiteral("attention"), QStringLiteral("running"),
                                   QStringLiteral("stale"),  QStringLiteral("unknown"),   QStringLiteral("healthy")};
           for (int index = 3; index < job_count; ++index) {
             const auto candidate = conclusionHealth(jobs.at(index).toObject());
             if (ranks.indexOf(candidate) < ranks.indexOf(hidden_health)) hidden_health = candidate;
           }
           cards.push_back(
               {QStringLiteral("more"), QStringLiteral("+%1 jobs").arg(job_count - 3), {}, {}, hidden_health, {}, {}});
         }
         stages_.replace(std::move(cards));
         auto rows = projects_.rows();
         if (selected_index_ >= 0 && selected_index_ < rows.size()) {
           rows[selected_index_].detail.insert(QStringLiteral("jobsSummary"),
                                               QStringLiteral("%1/%2").arg(successful).arg(job_count));
           rows[selected_index_].detail.insert(QStringLiteral("deployStatus"), deploy);
           const auto created = rows[selected_index_].detail.value(QStringLiteral("createdAt")).toDateTime();
           const auto updated = rows[selected_index_].detail.value(QStringLiteral("updatedAt")).toDateTime();
           rows[selected_index_].detail.insert(QStringLiteral("duration"),
                                               created.isValid() && updated.isValid()
                                                   ? QStringLiteral("%1m").arg(created.secsTo(updated) / 60)
                                                   : QString{});
           projects_.replace(std::move(rows));
         }
         emit selectedProjectChanged();
       },
       [this](QString) { stages_.replace({}); }});
  request({QUrl(QStringLiteral("https://api.github.com/repos/%1/actions/runs/%2/artifacts?per_page=100")
                    .arg(repository)
                    .arg(run_id)),
           [this](const QJsonDocument& document, const QNetworkReply*) {
             qint64 bytes = 0;
             for (const auto& value : document.object().value(QStringLiteral("artifacts")).toArray()) {
               const auto artifact = value.toObject();
               if (!artifact.value(QStringLiteral("expired")).toBool())
                 bytes += artifact.value(QStringLiteral("size_in_bytes")).toInteger();
             }
             auto rows = projects_.rows();
             if (selected_index_ >= 0 && selected_index_ < rows.size()) {
               rows[selected_index_].detail.insert(QStringLiteral("artifactSize"), bytesText(bytes));
               projects_.replace(std::move(rows));
             }
             emit selectedProjectChanged();
           },
           [this](QString) { emit selectedProjectChanged(); }});
}

void ProjectsService::request(Request request_value) {
  requests_.enqueue(std::move(request_value));
  startRequests();
}

void ProjectsService::startRequests() {
  while (active_requests_ < kMaximumConcurrentRequests && !requests_.isEmpty()) {
    auto pending = requests_.dequeue();
    QNetworkRequest request(pending.url);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("rpi-dashboard/0.1"));
    request.setTransferTimeout(kRequestTimeoutMs);
    if (!token_.isEmpty()) request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + token_);
    const auto cache_key = pending.url.toString();
    if (etags_.contains(cache_key)) request.setRawHeader("If-None-Match", etags_.value(cache_key));
    auto* reply = network_.get(request);
    if (counting_refresh_requests_) ++refresh_request_count_;
    ++active_requests_;
    connect(reply, &QNetworkReply::finished, this, [this, reply, pending = std::move(pending)] {
      --active_requests_;
      const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      const auto cache_key = reply->request().url().toString();
      const auto has_cached_body = cached_bodies_.contains(cache_key);
      QByteArray body;
      if (status == 304 && has_cached_body)
        body = cached_bodies_.value(cache_key);
      else if (shouldReadReplyBody(reply->error(), status, has_cached_body))
        body = reply->readAll();
      if (reply->error() == QNetworkReply::NoError && ((status >= 200 && status < 300) || status == 304)) {
        QJsonParseError parse_error;
        const auto document = QJsonDocument::fromJson(body, &parse_error);
        if (parse_error.error == QJsonParseError::NoError) {
          const auto etag = reply->rawHeader("ETag");
          if (!etag.isEmpty()) {
            etags_.insert(cache_key, etag);
            cached_bodies_.insert(cache_key, body);
          }
          pending.success(document, reply);
        } else
          pending.failure(QStringLiteral("GitHub returned malformed JSON"));
      } else {
        const auto delay =
            rateLimitBackoffMs(status, reply->rawHeader("Retry-After"), reply->rawHeader("X-RateLimit-Remaining"),
                               reply->rawHeader("X-RateLimit-Reset"), QDateTime::currentSecsSinceEpoch());
        if (delay > 0) poll_timer_.setInterval(std::max(poll_timer_.interval(), delay));
        pending.failure(QStringLiteral("GitHub request failed (%1)").arg(status));
      }
      reply->deleteLater();
      startRequests();
    });
  }
}

void ProjectsService::finishRefresh() {
  last_success_ = QDateTime::currentDateTimeUtc();
  stale_ = false;
  counting_refresh_requests_ = false;
  poll_timer_.setInterval(pollIntervalMs(!token_.isEmpty(), refresh_request_count_));
  setState(projects_.rowCount() == 0 ? QStringLiteral("empty") : QStringLiteral("ready"));
  emit snapshotChanged();
  emit selectedProjectChanged();
  loadRunners();
}

void ProjectsService::failRefresh(const QString& diagnostic) {
  counting_refresh_requests_ = false;
  stale_ = projects_.rowCount() > 0;
  setState(stale_ ? QStringLiteral("ready") : QStringLiteral("error"), diagnostic);
}

void ProjectsService::setState(QString state, QString diagnostic) {
  state_ = std::move(state);
  diagnostics_ = std::move(diagnostic);
  emit stateChanged();
}

QString ProjectsService::ageText(const QDateTime& timestamp) {
  if (!timestamp.isValid()) return {};
  const auto minutes = timestamp.secsTo(QDateTime::currentDateTimeUtc()) / 60;
  if (minutes < 60) return QStringLiteral("%1m ago").arg(std::max<qint64>(0, minutes));
  if (minutes < 1440) return QStringLiteral("%1h ago").arg(minutes / 60);
  return QStringLiteral("%1d ago").arg(minutes / 1440);
}

QString ProjectsService::bytesText(qint64 bytes) {
  if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
  if (bytes < 1024 * 1024) return QStringLiteral("%1 KiB").arg(bytes / 1024.0, 0, 'f', 1);
  return QStringLiteral("%1 MiB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

}  // namespace dashboard::projects
// NOLINTEND
