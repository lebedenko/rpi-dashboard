#include "screensaver_controller.h"

#include <QCoreApplication>
#include <QEvent>

#include <chrono>

namespace dashboard {
namespace {

[[nodiscard]] bool isActivityEvent(QEvent::Type type) {
  switch (type) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::Wheel:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] bool isTouchEvent(QEvent::Type type) {
  return type == QEvent::TouchBegin || type == QEvent::TouchUpdate || type == QEvent::TouchEnd ||
         type == QEvent::TouchCancel;
}

}  // namespace

ScreensaverController::ScreensaverController(int timeoutSeconds, QObject* parent) : QObject(parent) {
  initialize(std::chrono::seconds(timeoutSeconds));
}

ScreensaverController::ScreensaverController(std::chrono::milliseconds timeout, QObject* parent) : QObject(parent) {
  initialize(timeout);
}

ScreensaverController::~ScreensaverController() {
  if (enabled_ && QCoreApplication::instance() != nullptr) {
    QCoreApplication::instance()->removeEventFilter(this);
  }
}

void ScreensaverController::initialize(std::chrono::milliseconds timeout) {
  enabled_ = timeout.count() > 0;
  if (!enabled_) {
    return;
  }
  timer_.setSingleShot(true);
  timer_.setInterval(timeout);
  connect(&timer_, &QTimer::timeout, this, [this] { setActive(true); });
  QCoreApplication::instance()->installEventFilter(this);
  timer_.start();
}

bool ScreensaverController::eventFilter(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)
  if (!enabled_ || !isActivityEvent(event->type())) {
    return false;
  }

  if (consumingTouchSequence_ && isTouchEvent(event->type())) {
    if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
      consumingTouchSequence_ = false;
    }
    return true;
  }

  if (active_) {
    consumingTouchSequence_ = event->type() == QEvent::TouchBegin;
    setActive(false);
    timer_.start();
    return true;
  }

  timer_.start();
  return false;
}

void ScreensaverController::setActive(bool active) {
  if (active_ == active) {
    return;
  }
  active_ = active;
  if (active_) {
    timer_.stop();
  }
  emit activeChanged();
}

}  // namespace dashboard
