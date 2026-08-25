#include "resource_history_series.h"

#include <QAbstractItemModel>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace dashboard::ui {
namespace {
constexpr qreal kWindowMilliseconds = 60'000.0;

struct Point {
  float x;
  float y;
};

void clearChildren(QSGNode* node) {
  while (QSGNode* child = node->firstChild()) {
    node->removeChildNode(child);
    delete child;
  }
}

QSGGeometryNode* lineNode(const QList<Point>& points, const QColor& color) {
  constexpr float kHalfLineWidth = 1.0F;
  const int segment_count = static_cast<int>(points.size()) - 1;
  auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), segment_count * 6);
  geometry->setDrawingMode(QSGGeometry::DrawTriangles);
  auto* vertices = geometry->vertexDataAsPoint2D();
  for (int index = 0; index < segment_count; ++index) {
    const Point start = points.at(index);
    const Point end = points.at(index + 1);
    const float delta_x = end.x - start.x;
    const float delta_y = end.y - start.y;
    const float length = std::hypot(delta_x, delta_y);
    const float normal_x = length > 0.0F ? (-delta_y / length) * kHalfLineWidth : 0.0F;
    const float normal_y = length > 0.0F ? (delta_x / length) * kHalfLineWidth : kHalfLineWidth;
    const int vertex = index * 6;
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes a vertex buffer pointer.
    vertices[vertex].set(start.x + normal_x, start.y + normal_y);
    vertices[vertex + 1].set(start.x - normal_x, start.y - normal_y);
    vertices[vertex + 2].set(end.x + normal_x, end.y + normal_y);
    vertices[vertex + 3].set(end.x + normal_x, end.y + normal_y);
    vertices[vertex + 4].set(start.x - normal_x, start.y - normal_y);
    vertices[vertex + 5].set(end.x - normal_x, end.y - normal_y);
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }
  auto* material = new QSGFlatColorMaterial;
  material->setColor(color);
  auto* node = new QSGGeometryNode;
  node->setGeometry(geometry);
  node->setFlag(QSGNode::OwnsGeometry);
  node->setMaterial(material);
  node->setFlag(QSGNode::OwnsMaterial);
  return node;
}

QSGGeometryNode* downwardGlowNode(const QList<Point>& points, const QColor& color) {
  constexpr float kGlowDepth = 3.0F;
  constexpr std::uint8_t kEdgeAlpha = 12;
  auto* geometry =
      new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), static_cast<int>(points.size() * 2));
  geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
  auto* vertices = geometry->vertexDataAsColoredPoint2D();
  const auto premultiply = [](int channel) { return static_cast<std::uint8_t>((channel * kEdgeAlpha) / 255); };
  const std::uint8_t red = premultiply(color.red());
  const std::uint8_t green = premultiply(color.green());
  const std::uint8_t blue = premultiply(color.blue());
  for (qsizetype index = 0; index < points.size(); ++index) {
    const Point point = points.at(index);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes a vertex buffer pointer.
    vertices[index * 2].set(point.x, point.y, red, green, blue, kEdgeAlpha);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes a vertex buffer pointer.
    vertices[(index * 2) + 1].set(point.x, point.y + kGlowDepth, 0, 0, 0, 0);
  }
  auto* node = new QSGGeometryNode;
  node->setGeometry(geometry);
  node->setFlag(QSGNode::OwnsGeometry);
  node->setMaterial(new QSGVertexColorMaterial);
  node->setFlag(QSGNode::OwnsMaterial);
  return node;
}
}  // namespace

ResourceHistorySeries::ResourceHistorySeries(QQuickItem* parent) : QQuickItem(parent) {
  setFlag(ItemHasContents);
  setClip(true);
  connect(this, &QQuickItem::widthChanged, this, &QQuickItem::update);
  connect(this, &QQuickItem::heightChanged, this, &QQuickItem::update);
}

QAbstractItemModel* ResourceHistorySeries::model() const { return model_; }

void ResourceHistorySeries::setModel(QAbstractItemModel* model) {
  if (model_ == model) {
    return;
  }
  if (model_) {
    disconnect(model_, nullptr, this, nullptr);
  }
  model_ = model;
  reconnectModel();
  cacheModel();
  emit modelChanged();
}

QColor ResourceHistorySeries::cpuColor() const { return cpu_color_; }
void ResourceHistorySeries::setCpuColor(const QColor& color) {
  if (std::exchange(cpu_color_, color) != color) {
    update();
    emit cpuColorChanged();
  }
}
QColor ResourceHistorySeries::memoryColor() const { return memory_color_; }
void ResourceHistorySeries::setMemoryColor(const QColor& color) {
  if (std::exchange(memory_color_, color) != color) {
    update();
    emit memoryColorChanged();
  }
}

void ResourceHistorySeries::reconnectModel() {
  if (!model_) {
    return;
  }
  const auto refresh = [this] { cacheModel(); };
  connect(model_, &QAbstractItemModel::rowsInserted, this, refresh);
  connect(model_, &QAbstractItemModel::rowsRemoved, this, refresh);
  connect(model_, &QAbstractItemModel::dataChanged, this, refresh);
  connect(model_, &QAbstractItemModel::modelReset, this, refresh);
  connect(model_, &QObject::destroyed, this, [this] {
    model_ = nullptr;
    cacheModel();
    emit modelChanged();
  });
}

void ResourceHistorySeries::cacheModel() {
  samples_.clear();
  if (model_) {
    const QHash<int, QByteArray> roles = model_->roleNames();
    const int elapsed_role = roles.key(QByteArrayLiteral("elapsedMilliseconds"), -1);
    const int cpu_role = roles.key(QByteArrayLiteral("cpuUsageRatio"), -1);
    const int memory_role = roles.key(QByteArrayLiteral("memoryUsageRatio"), -1);
    if (elapsed_role >= 0) {
      samples_.reserve(model_->rowCount());
      for (int row = 0; row < model_->rowCount(); ++row) {
        const QModelIndex index = model_->index(row, 0);
        const QVariant cpu = cpu_role >= 0 ? model_->data(index, cpu_role) : QVariant{};
        const QVariant memory = memory_role >= 0 ? model_->data(index, memory_role) : QVariant{};
        samples_.append(
            {.elapsed_milliseconds = model_->data(index, elapsed_role).toLongLong(),
             .cpu_usage_ratio = cpu.isValid() && !cpu.isNull() ? std::optional(cpu.toDouble()) : std::nullopt,
             .memory_usage_ratio =
                 memory.isValid() && !memory.isNull() ? std::optional(memory.toDouble()) : std::nullopt});
      }
    }
  }
  update();
}

QSGNode* ResourceHistorySeries::updatePaintNode(QSGNode* old_node, UpdatePaintNodeData* update_data) {
  Q_UNUSED(update_data)
  QSGNode* root = old_node != nullptr ? old_node : new QSGNode;
  clearChildren(root);
  if (samples_.isEmpty() || width() <= 0 || height() <= 0) {
    return root;
  }

  const qint64 newest = samples_.constLast().elapsed_milliseconds;
  auto segmentsFor = [&](auto value_for_sample) {
    QList<QList<Point>> segments;
    QList<Point> segment;
    auto flush = [&] {
      if (segment.size() >= 2) {
        segments.append(segment);
      }
      segment.clear();
    };
    for (const Sample& sample : std::as_const(samples_)) {
      const std::optional<double> value = value_for_sample(sample);
      if (!value) {
        flush();
        continue;
      }
      const auto age = static_cast<qreal>(newest - sample.elapsed_milliseconds);
      const qreal x_position = width() * (1.0 - (age / kWindowMilliseconds));
      const qreal y_position = height() * (1.0 - std::clamp(*value, 0.0, 1.0));
      segment.append({.x = static_cast<float>(x_position), .y = static_cast<float>(y_position)});
    }
    flush();
    return segments;
  };
  const auto cpu_segments = segmentsFor([](const Sample& sample) { return sample.cpu_usage_ratio; });
  const auto memory_segments = segmentsFor([](const Sample& sample) { return sample.memory_usage_ratio; });
  for (const QList<Point>& segment : cpu_segments) {
    root->appendChildNode(downwardGlowNode(segment, cpu_color_));
  }
  for (const QList<Point>& segment : memory_segments) {
    root->appendChildNode(downwardGlowNode(segment, memory_color_));
  }
  for (const QList<Point>& segment : cpu_segments) {
    root->appendChildNode(lineNode(segment, cpu_color_));
  }
  for (const QList<Point>& segment : memory_segments) {
    root->appendChildNode(lineNode(segment, memory_color_));
  }
  return root;
}

}  // namespace dashboard::ui
