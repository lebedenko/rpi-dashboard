#pragma once

#include "resource_history_geometry.h"

#include <QAbstractItemModel>
#include <QColor>
#include <QElapsedTimer>
#include <QPointer>
#include <QQuickItem>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace dashboard::ui {

class ResourceHistorySeriesTest;

class ResourceHistorySeries : public QQuickItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
  Q_PROPERTY(QColor cpuColor READ cpuColor WRITE setCpuColor NOTIFY cpuColorChanged)
  Q_PROPERTY(QColor memoryColor READ memoryColor WRITE setMemoryColor NOTIFY memoryColorChanged)
  Q_PROPERTY(QColor plotBackgroundColor READ plotBackgroundColor WRITE setPlotBackgroundColor NOTIFY
                 plotBackgroundColorChanged)
  Q_PROPERTY(
      int transitionDuration READ transitionDuration WRITE setTransitionDuration NOTIFY transitionDurationChanged)

 public:
  explicit ResourceHistorySeries(QQuickItem* parent = nullptr);

  [[nodiscard]] QAbstractItemModel* model() const;
  void setModel(QAbstractItemModel* model);
  [[nodiscard]] QColor cpuColor() const;
  void setCpuColor(const QColor& color);
  [[nodiscard]] QColor memoryColor() const;
  void setMemoryColor(const QColor& color);
  [[nodiscard]] QColor plotBackgroundColor() const;
  void setPlotBackgroundColor(const QColor& color);
  [[nodiscard]] int transitionDuration() const;
  void setTransitionDuration(int duration);

 signals:
  void modelChanged();
  void cpuColorChanged();
  void memoryColorChanged();
  void plotBackgroundColorChanged();
  void transitionDurationChanged();

 protected:
  QSGNode* updatePaintNode(QSGNode* old_node, UpdatePaintNodeData* update_data) override;

 private:
  struct Snapshot {
    QList<geometry::Sample> samples;
    std::optional<geometry::Sample> predecessor;
  };

  friend class ResourceHistorySeriesTest;
  void reconnectModel();
  void capturePredecessor(const QModelIndex& parent, int first, int last);
  void cacheModel(bool animate);
  void repaintImmediately();
  QPointer<QAbstractItemModel> model_;
  Snapshot snapshot_;
  Snapshot previous_snapshot_;
  QColor cpu_color_{QStringLiteral("#35A7FF")};
  QColor memory_color_{QStringLiteral("#A875F5")};
  QColor plot_background_color_{QStringLiteral("#0E1823")};
  int transition_duration_{200};
  QElapsedTimer transition_clock_;
  QTimer transition_timer_;
};

}  // namespace dashboard::ui
