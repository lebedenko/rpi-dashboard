#include "projects/projects_service.h"

#include "projects/github_credentials.h"
#include "projects/project_health.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using dashboard::projects::Health;
using dashboard::projects::healthForRun;
using dashboard::projects::loadGitHubCredential;
using dashboard::projects::pollIntervalMs;
using dashboard::projects::ProjectsListModel;
using dashboard::projects::rateLimitBackoffMs;
using dashboard::projects::shouldReadReplyBody;

class ProjectsServiceTest : public QObject {
  Q_OBJECT

 private slots:
  void mapsWorkflowHealth_data();  // NOLINT(readability-identifier-naming)
  void mapsWorkflowHealth();
  void marksOldSuccessStale();
  void listModelPublishesProviderNeutralRoles();
  void failedNetworkReplyIsNotRead();
  void authenticatedPollingUsesOneMinuteBaseline();
  void anonymousPollingUsesMeasuredRequestCount_data();  // NOLINT(readability-identifier-naming)
  void anonymousPollingUsesMeasuredRequestCount();
  void ignoresInapplicableRateLimitHeaders_data();  // NOLINT(readability-identifier-naming)
  void ignoresInapplicableRateLimitHeaders();
  void appliesRateLimitBackoff_data();  // NOLINT(readability-identifier-naming)
  void appliesRateLimitBackoff();
  void loadsEnvironmentCredentialWhenFileIsNotConfigured();
  void fileCredentialTakesPrecedenceAndRemovesNewlines();
  void invalidFileCredentialFallsBackAnonymously_data();  // NOLINT(readability-identifier-naming)
  void invalidFileCredentialFallsBackAnonymously();
};

void ProjectsServiceTest::
    mapsWorkflowHealth_data() {  // NOLINT(readability-identifier-naming,readability-convert-member-functions-to-static)
  QTest::addColumn<QString>("status");
  QTest::addColumn<QString>("conclusion");
  QTest::addColumn<Health>("expected");
  QTest::newRow("queued") << QStringLiteral("queued") << QString{} << Health::Running;
  QTest::newRow("success") << QStringLiteral("completed") << QStringLiteral("success") << Health::Healthy;
  QTest::newRow("failure") << QStringLiteral("completed") << QStringLiteral("failure") << Health::Failed;
  QTest::newRow("timed-out") << QStringLiteral("completed") << QStringLiteral("timed_out") << Health::Failed;
  QTest::newRow("action-required") << QStringLiteral("completed") << QStringLiteral("action_required")
                                   << Health::Failed;
  QTest::newRow("cancelled") << QStringLiteral("completed") << QStringLiteral("cancelled") << Health::Attention;
  QTest::newRow("stale") << QStringLiteral("completed") << QStringLiteral("stale") << Health::Stale;
  QTest::newRow("missing") << QString{} << QString{} << Health::Unknown;
}

void ProjectsServiceTest::mapsWorkflowHealth() {  // NOLINT(readability-convert-member-functions-to-static)
  QFETCH(QString, status);
  QFETCH(QString, conclusion);
  QFETCH(Health, expected);
  QCOMPARE(healthForRun(status, conclusion, QDateTime::currentDateTimeUtc()), expected);
}

void ProjectsServiceTest::marksOldSuccessStale() {  // NOLINT(readability-convert-member-functions-to-static)
  const auto now = QDateTime::fromString(QStringLiteral("2026-08-26T12:00:00Z"), Qt::ISODate);
  QCOMPARE(healthForRun(QStringLiteral("completed"), QStringLiteral("success"), now.addDays(-6), now), Health::Healthy);
  QCOMPARE(healthForRun(QStringLiteral("completed"), QStringLiteral("success"), now.addDays(-7), now), Health::Stale);
}

void ProjectsServiceTest::
    listModelPublishesProviderNeutralRoles() {  // NOLINT(readability-convert-member-functions-to-static)
  ProjectsListModel model;
  model.replace({{.key = QStringLiteral("owner/repo"),
                  .name = QStringLiteral("repo"),
                  .branch = QStringLiteral("main"),
                  .health = QStringLiteral("failed"),
                  .status = QStringLiteral("completed"),
                  .detail = {}}});
  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(model.data(model.index(0), ProjectsListModel::KeyRole).toString(), QStringLiteral("owner/repo"));
  QCOMPARE(model.data(model.index(0), ProjectsListModel::HealthRole).toString(), QStringLiteral("failed"));
  QCOMPARE(model.roleNames().value(ProjectsListModel::BranchRole), QByteArrayLiteral("branch"));
}

void ProjectsServiceTest::failedNetworkReplyIsNotRead() {  // NOLINT(readability-convert-member-functions-to-static)
  QVERIFY(!shouldReadReplyBody(QNetworkReply::SslHandshakeFailedError, 0, false));
  QVERIFY(!shouldReadReplyBody(QNetworkReply::ConnectionRefusedError, 0, false));
  QVERIFY(!shouldReadReplyBody(QNetworkReply::NoError, 304, true));
  QVERIFY(shouldReadReplyBody(QNetworkReply::NoError, 200, false));
}

void ProjectsServiceTest::
    authenticatedPollingUsesOneMinuteBaseline() {  // NOLINT(readability-convert-member-functions-to-static)
  QCOMPARE(pollIntervalMs(true, 200), 60'000);
}

void ProjectsServiceTest::
    anonymousPollingUsesMeasuredRequestCount_data() {  // NOLINT(readability-identifier-naming,readability-convert-member-functions-to-static)
  QTest::addColumn<int>("requestCount");
  QTest::addColumn<int>("expectedIntervalMs");
  QTest::newRow("minimum") << 1 << 15 * 60'000;
  QTest::newRow("floor-boundary") << 12 << 15 * 60'000;
  QTest::newRow("above-floor") << 13 << 936'000;
  QTest::newRow("larger-refresh") << 50 << 60 * 60'000;
}

void ProjectsServiceTest::
    anonymousPollingUsesMeasuredRequestCount() {  // NOLINT(readability-convert-member-functions-to-static)
  QFETCH(int, requestCount);
  QFETCH(int, expectedIntervalMs);
  const auto interval = pollIntervalMs(false, requestCount);
  QCOMPARE(interval, expectedIntervalMs);
  QVERIFY(interval >= 15 * 60'000);
  QVERIFY(static_cast<qint64>(requestCount) * 60 * 60'000 / interval <= 50);
}

void ProjectsServiceTest::
    ignoresInapplicableRateLimitHeaders_data() {  // NOLINT(readability-identifier-naming,readability-convert-member-functions-to-static)
  QTest::addColumn<int>("status");
  QTest::addColumn<QByteArray>("retryAfter");
  QTest::addColumn<QByteArray>("remaining");
  QTest::newRow("ordinary-forbidden") << 403 << QByteArray{} << QByteArrayLiteral("42");
  QTest::newRow("not-found") << 404 << QByteArrayLiteral("120") << QByteArrayLiteral("42");
  QTest::newRow("server-error") << 500 << QByteArrayLiteral("120") << QByteArrayLiteral("42");
}

void ProjectsServiceTest::
    ignoresInapplicableRateLimitHeaders() {  // NOLINT(readability-convert-member-functions-to-static)
  QFETCH(int, status);
  QFETCH(QByteArray, retryAfter);
  QFETCH(QByteArray, remaining);
  QCOMPARE(rateLimitBackoffMs(status, retryAfter, remaining, QByteArrayLiteral("2000"), 1000), 0);
}

void ProjectsServiceTest::
    appliesRateLimitBackoff_data() {  // NOLINT(readability-identifier-naming,readability-convert-member-functions-to-static)
  QTest::addColumn<int>("status");
  QTest::addColumn<QByteArray>("retryAfter");
  QTest::addColumn<QByteArray>("remaining");
  QTest::addColumn<QByteArray>("reset");
  QTest::addColumn<int>("expectedDelayMs");
  QTest::newRow("forbidden-retry-after") << 403 << QByteArrayLiteral("120") << QByteArrayLiteral("42")
                                         << QByteArrayLiteral("2000") << 120'000;
  QTest::newRow("too-many-requests") << 429 << QByteArrayLiteral("90") << QByteArrayLiteral("42") << QByteArray{}
                                     << 90'000;
  QTest::newRow("exhausted-quota") << 403 << QByteArray{} << QByteArrayLiteral("0") << QByteArrayLiteral("1120")
                                   << 120'000;
}

void ProjectsServiceTest::appliesRateLimitBackoff() {  // NOLINT(readability-convert-member-functions-to-static)
  QFETCH(int, status);
  QFETCH(QByteArray, retryAfter);
  QFETCH(QByteArray, remaining);
  QFETCH(QByteArray, reset);
  QFETCH(int, expectedDelayMs);
  QCOMPARE(rateLimitBackoffMs(status, retryAfter, remaining, reset, 1000), expectedDelayMs);
}

void ProjectsServiceTest::
    loadsEnvironmentCredentialWhenFileIsNotConfigured() {  // NOLINT(readability-convert-member-functions-to-static)
  const auto credential = loadGitHubCredential({}, QByteArrayLiteral("development-token"));
  QCOMPARE(credential.token, QByteArrayLiteral("development-token"));
  QVERIFY(credential.diagnostic.isEmpty());
}

void ProjectsServiceTest::
    fileCredentialTakesPrecedenceAndRemovesNewlines() {  // NOLINT(readability-convert-member-functions-to-static)
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const auto path = directory.filePath(QStringLiteral("github-token"));
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  QCOMPARE(file.write("file-token\r\n"), 12);
  file.close();

  const auto credential = loadGitHubCredential(QFile::encodeName(path), QByteArrayLiteral("environment-token"));
  QCOMPARE(credential.token, QByteArrayLiteral("file-token"));
  QVERIFY(credential.diagnostic.isEmpty());
}

void ProjectsServiceTest::
    invalidFileCredentialFallsBackAnonymously_data() {  // NOLINT(readability-identifier-naming,readability-convert-member-functions-to-static)
  QTest::addColumn<bool>("createEmptyFile");
  QTest::newRow("missing") << false;
  QTest::newRow("empty") << true;
}

void ProjectsServiceTest::
    invalidFileCredentialFallsBackAnonymously() {  // NOLINT(readability-convert-member-functions-to-static)
  QFETCH(bool, createEmptyFile);
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const auto path = directory.filePath(QStringLiteral("github-token"));
  if (createEmptyFile) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
  }

  const auto secret = QByteArrayLiteral("must-not-appear");
  const auto credential = loadGitHubCredential(QFile::encodeName(path), secret);
  QVERIFY(credential.token.isEmpty());
  QVERIFY(!credential.diagnostic.isEmpty());
  QVERIFY(!credential.diagnostic.contains(QString::fromUtf8(secret)));
}

QTEST_GUILESS_MAIN(ProjectsServiceTest)

#include "projects_service_test.moc"
