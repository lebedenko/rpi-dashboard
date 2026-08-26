#include "projects/projects_service.h"

#include "projects/github_credentials.h"
#include "projects/project_health.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using dashboard::projects::Health;
using dashboard::projects::healthForRun;
using dashboard::projects::loadGitHubCredential;
using dashboard::projects::ProjectsListModel;
using dashboard::projects::shouldReadReplyBody;

class ProjectsServiceTest : public QObject {
  Q_OBJECT

 private slots:
  void mapsWorkflowHealth_data();  // NOLINT(readability-identifier-naming)
  void mapsWorkflowHealth();
  void marksOldSuccessStale();
  void listModelPublishesProviderNeutralRoles();
  void failedNetworkReplyIsNotRead();
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
