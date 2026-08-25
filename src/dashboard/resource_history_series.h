#pragma once

#include "resource_history_geometry.h"

#include <QAbstractItemModel>
#include <QColor>
#include <QPointer>
#include <QQuickItem>
#include <QVariantAnimation>
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
    QList<geometry::Sample> retained_prefix;
  };

  friend class ResourceHistorySeriesTest;
  [[nodiscard]] static qreal interpolateWindowEnd(qreal start, qreal target, qreal progress);
  [[nodiscard]] qreal currentWindowEnd() const;
  void rebuildAt(qreal window_end_milliseconds);
  void reconnectModel();
  void captureRemovedPrefix(const QModelIndex& parent, int first, int last);
  void cacheModel(bool animate);
  QPointer<QAbstractItemModel> model_;
  Snapshot snapshot_;
  QColor cpu_color_{QStringLiteral("#36B9FF")};
  QColor memory_color_{QStringLiteral("#A66CFF")};
  QColor plot_background_color_{QStringLiteral("#041321")};
  int transition_duration_{350};
  qreal window_start_milliseconds_{};
  qreal window_current_milliseconds_{};
  qreal window_target_milliseconds_{};
  qreal geometry_origin_milliseconds_{};
  bool has_window_end_{false};
  quint64 geometry_revision_{};
  QVariantAnimation transition_animation_;
};

}  // namespace dashboard::ui
