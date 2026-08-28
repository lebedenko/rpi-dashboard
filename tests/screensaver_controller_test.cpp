#include "screensaver_controller.h"

#include <QImageReader>
#include <QKeyEvent>
#include <QSignalSpy>
#include <QTest>

#include <chrono>

using dashboard::ScreensaverController;
using namespace std::chrono_literals;

// NOLINTBEGIN(readability-convert-member-functions-to-static,readability-identifier-naming)

class EventReceiver final : public QObject {
 public:
  bool event(QEvent* event) override {
    if (event->type() == QEvent::KeyPress) {
      receivedKeyPress = true;
    }
    if (event->type() == QEvent::TouchBegin || event->type() == QEvent::TouchUpdate ||
        event->type() == QEvent::TouchEnd) {
      ++receivedTouchEvents;
    }
    return QObject::event(event);
  }
  bool receivedKeyPress{};
  int receivedTouchEvents{};
};

class ScreensaverControllerTest final : public QObject {
  Q_OBJECT
 private slots:
  void activatesAndNotifies();
  void activityResetsTimer();
  void disabledDoesNotFilterOrActivate();
  void wakeEventIsConsumed();
  void wallpapersArePackagedAndDecodable();
  void wakeTouchSequenceIsConsumed();
};

void ScreensaverControllerTest::activatesAndNotifies() {
  ScreensaverController controller(30ms);
  QSignalSpy changed(&controller, &ScreensaverController::activeChanged);
  QTRY_VERIFY_WITH_TIMEOUT(controller.active(), 200);
  QCOMPARE(changed.count(), 1);
}

void ScreensaverControllerTest::activityResetsTimer() {
  ScreensaverController controller(80ms);
  EventReceiver receiver;
  QTest::qWait(50);
  QKeyEvent event(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
  QCoreApplication::sendEvent(&receiver, &event);
  QTest::qWait(50);
  QVERIFY(!controller.active());
  QTRY_VERIFY_WITH_TIMEOUT(controller.active(), 150);
}

void ScreensaverControllerTest::disabledDoesNotFilterOrActivate() {
  ScreensaverController controller(0ms);
  EventReceiver receiver;
  QKeyEvent event(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
  QCoreApplication::sendEvent(&receiver, &event);
  QVERIFY(receiver.receivedKeyPress);
  QTest::qWait(40);
  QVERIFY(!controller.active());
}

void ScreensaverControllerTest::wakeEventIsConsumed() {
  ScreensaverController controller(20ms);
  EventReceiver receiver;
  QSignalSpy changed(&controller, &ScreensaverController::activeChanged);
  QTRY_VERIFY_WITH_TIMEOUT(controller.active(), 150);
  QKeyEvent event(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
  QVERIFY(QCoreApplication::sendEvent(&receiver, &event));
  QVERIFY(!receiver.receivedKeyPress);
  QVERIFY(!controller.active());
  QCOMPARE(changed.count(), 2);
}

void ScreensaverControllerTest::wallpapersArePackagedAndDecodable() {
  const QStringList codes = {QStringLiteral("01d"), QStringLiteral("01n"), QStringLiteral("02d"), QStringLiteral("02n"),
                             QStringLiteral("03d"), QStringLiteral("03n"), QStringLiteral("04d"), QStringLiteral("04n"),
                             QStringLiteral("09d"), QStringLiteral("09n"), QStringLiteral("10d"), QStringLiteral("10n"),
                             QStringLiteral("11d"), QStringLiteral("11n"), QStringLiteral("13d"), QStringLiteral("13n"),
                             QStringLiteral("50d"), QStringLiteral("50n")};
  for (const auto& code : codes) {
    QImageReader reader(QStringLiteral(":/rpi-dashboard/weather-bg/%1.png").arg(code));
    QVERIFY2(reader.canRead(),
             qPrintable(QStringLiteral("Cannot decode wallpaper %1: %2").arg(code, reader.errorString())));
    QCOMPARE(reader.size(), QSize(1480, 320));
  }
}

void ScreensaverControllerTest::wakeTouchSequenceIsConsumed() {
  ScreensaverController controller(20ms);
  EventReceiver receiver;
  QTRY_VERIFY_WITH_TIMEOUT(controller.active(), 150);
  QEvent begin(QEvent::TouchBegin);
  QEvent update(QEvent::TouchUpdate);
  QEvent end(QEvent::TouchEnd);
  QVERIFY(QCoreApplication::sendEvent(&receiver, &begin));
  QVERIFY(QCoreApplication::sendEvent(&receiver, &update));
  QVERIFY(QCoreApplication::sendEvent(&receiver, &end));
  QCOMPARE(receiver.receivedTouchEvents, 0);
  QVERIFY(!controller.active());
}

QTEST_MAIN(ScreensaverControllerTest)
#include "screensaver_controller_test.moc"
// NOLINTEND(readability-convert-member-functions-to-static,readability-identifier-naming)
