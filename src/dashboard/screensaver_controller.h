#pragma once

#include <QObject>
#include <QTimer>

#include <chrono>

namespace dashboard {

class ScreensaverController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool active READ active NOTIFY activeChanged)

 public:
  explicit ScreensaverController(int timeoutSeconds, QObject* parent = nullptr);
  explicit ScreensaverController(std::chrono::milliseconds timeout, QObject* parent = nullptr);
  ~ScreensaverController() override;
  Q_DISABLE_COPY_MOVE(ScreensaverController)

  [[nodiscard]] bool active() const { return active_; }

 signals:
  void activeChanged();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void initialize(std::chrono::milliseconds timeout);
  void setActive(bool active);

  QTimer timer_;
  bool active_{};
  bool enabled_{};
  bool consumingTouchSequence_{};
};

}  // namespace dashboard
