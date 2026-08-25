#pragma once

#include <QAbstractItemModel>
#include <QColor>
#include <QPointer>
#include <QQuickItem>
#include <QtQmlIntegration/qqmlintegration.h>

#include <optional>

namespace dashboard::ui {

class ResourceHistorySeries : public QQuickItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
  Q_PROPERTY(QColor cpuColor READ cpuColor WRITE setCpuColor NOTIFY cpuColorChanged)
  Q_PROPERTY(QColor memoryColor READ memoryColor WRITE setMemoryColor NOTIFY memoryColorChanged)

 public:
  explicit ResourceHistorySeries(QQuickItem* parent = nullptr);

  [[nodiscard]] QAbstractItemModel* model() const;
  void setModel(QAbstractItemModel* model);
  [[nodiscard]] QColor cpuColor() const;
  void setCpuColor(const QColor& color);
  [[nodiscard]] QColor memoryColor() const;
  void setMemoryColor(const QColor& color);

 signals:
  void modelChanged();
  void cpuColorChanged();
  void memoryColorChanged();

 protected:
  QSGNode* updatePaintNode(QSGNode* old_node, UpdatePaintNodeData* update_data) override;

 private:
  struct Sample {
    qint64 elapsed_milliseconds;
    std::optional<double> cpu_usage_ratio;
    std::optional<double> memory_usage_ratio;
  };

  void reconnectModel();
  void cacheModel();
  QPointer<QAbstractItemModel> model_;
  QList<Sample> samples_;
  QColor cpu_color_{QStringLiteral("#2F9BFF")};
  QColor memory_color_{QStringLiteral("#B889FF")};
};

}  // namespace dashboard::ui
