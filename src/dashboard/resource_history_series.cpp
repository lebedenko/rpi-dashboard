#include "resource_history_series.h"

#include <QAbstractItemModel>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGOpacityNode>
#include <QSGVertexColorMaterial>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace dashboard::ui {
namespace {
using geometry::CurveRun;
using geometry::Metric;
using geometry::MetricGeometry;

struct Point {
  float x;
  float y;
};

[[nodiscard]] QList<Point> renderPoints(const CurveRun& run) {
  QList<Point> points;
  points.reserve(run.tessellated.size());
  for (const QPointF& point : run.tessellated) {
    points.append({.x = static_cast<float>(point.x()), .y = static_cast<float>(point.y())});
  }
  return points;
}

void clearChildren(QSGNode* node) {
  while (QSGNode* child = node->firstChild()) {
    node->removeChildNode(child);
    delete child;
  }
}

[[nodiscard]] std::optional<double> optionalRatio(const QVariant& value) {
  if (!value.isValid() || value.isNull()) {
    return std::nullopt;
  }
  return value.toDouble();
}

[[nodiscard]] QList<geometry::Sample> readSamples(QAbstractItemModel* model) {
  QList<geometry::Sample> samples;
  if (model == nullptr) {
    return samples;
  }
  const QHash<int, QByteArray> roles = model->roleNames();
  const int elapsed_role = roles.key(QByteArrayLiteral("elapsedMilliseconds"), -1);
  if (elapsed_role < 0) {
    return samples;
  }
  const int cpu_role = roles.key(QByteArrayLiteral("cpuUsageRatio"), -1);
  const int memory_role = roles.key(QByteArrayLiteral("memoryUsageRatio"), -1);
  samples.reserve(model->rowCount());
  for (int row = 0; row < model->rowCount(); ++row) {
    const QModelIndex index = model->index(row, 0);
    const QVariant cpu = cpu_role >= 0 ? model->data(index, cpu_role) : QVariant{};
    const QVariant memory = memory_role >= 0 ? model->data(index, memory_role) : QVariant{};
    samples.append({.elapsed_milliseconds = model->data(index, elapsed_role).toLongLong(),
                    .cpu_usage_ratio = optionalRatio(cpu),
                    .memory_usage_ratio = optionalRatio(memory)});
  }
  return samples;
}

[[nodiscard]] std::optional<geometry::Sample> readSample(QAbstractItemModel* model, int row) {
  const QList<geometry::Sample> samples = readSamples(model);
  return row >= 0 && row < samples.size() ? std::optional(samples.at(row)) : std::nullopt;
}

[[nodiscard]] QSGGeometryNode* lineNode(const QList<Point>& points, const QColor& color, float width) {
  const float half_width = width / 2.0F;
  auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), static_cast<int>(points.size() * 2));
  geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
  auto* vertices = geometry->vertexDataAsPoint2D();
  const auto unitNormal = [](const Point& start, const Point& end) {
    const float delta_x = end.x - start.x;
    const float delta_y = end.y - start.y;
    const float length = std::hypot(delta_x, delta_y);
    return length > 0.0F ? Point{.x = -delta_y / length, .y = delta_x / length} : Point{.x = 0.0F, .y = 1.0F};
  };
  for (qsizetype index = 0; index < points.size(); ++index) {
    Point normal =
        index == 0 ? unitNormal(points.at(0), points.at(1)) : unitNormal(points.at(index - 1), points.at(index));
    if (index > 0 && index + 1 < points.size()) {
      const Point after = unitNormal(points.at(index), points.at(index + 1));
      const float sum_x = normal.x + after.x;
      const float sum_y = normal.y + after.y;
      const float sum_length = std::hypot(sum_x, sum_y);
      if (sum_length > 0.001F) {
        normal = {.x = sum_x / sum_length, .y = sum_y / sum_length};
      }
    }
    const Point point = points.at(index);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes a vertex buffer pointer.
    vertices[index * 2].set(point.x + (normal.x * half_width), point.y + (normal.y * half_width));
    vertices[(index * 2) + 1].set(point.x - (normal.x * half_width), point.y - (normal.y * half_width));
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

[[nodiscard]] QSGGeometryNode* areaNode(const QList<Point>& points, const QColor& color, float baseline) {
  constexpr std::uint8_t kCurveAlpha = 26;
  auto* geometry =
      new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), static_cast<int>(points.size() * 2));
  geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
  auto* vertices = geometry->vertexDataAsColoredPoint2D();
  const auto premultiply = [&](int channel) {
    return static_cast<std::uint8_t>((channel * static_cast<int>(kCurveAlpha)) / 255);
  };
  for (qsizetype index = 0; index < points.size(); ++index) {
    const Point point = points.at(index);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes a vertex buffer pointer.
    vertices[index * 2].set(point.x, point.y, premultiply(color.red()), premultiply(color.green()),
                            premultiply(color.blue()), kCurveAlpha);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes a vertex buffer pointer.
    vertices[(index * 2) + 1].set(point.x, baseline, 0, 0, 0, 0);
  }
  auto* node = new QSGGeometryNode;
  node->setGeometry(geometry);
  node->setFlag(QSGNode::OwnsGeometry);
  node->setMaterial(new QSGVertexColorMaterial);
  node->setFlag(QSGNode::OwnsMaterial);
  return node;
}

[[nodiscard]] QSGGeometryNode* ringNode(const QPointF& center, const QColor& color, const QColor& background) {
  constexpr int kSections = 20;
  constexpr float kOuterRadius = 4.0F;
  constexpr float kInnerRadius = 2.0F;
  auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), kSections * 12);
  geometry->setDrawingMode(QSGGeometry::DrawTriangles);
  auto* vertices = geometry->vertexDataAsColoredPoint2D();
  const auto write = [&](int index, float x_position, float y_position, const QColor& value) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes a vertex buffer pointer.
    vertices[index].set(x_position, y_position, static_cast<std::uint8_t>(value.red()),
                        static_cast<std::uint8_t>(value.green()), static_cast<std::uint8_t>(value.blue()),
                        static_cast<std::uint8_t>(value.alpha()));
  };
  constexpr float kTau = 6.2831853071795864769F;
  for (int section = 0; section < kSections; ++section) {
    const float angle_1 = kTau * static_cast<float>(section) / kSections;
    const float angle_2 = kTau * static_cast<float>(section + 1) / kSections;
    const QPointF outer_1 = center + QPointF(std::cos(angle_1) * kOuterRadius, std::sin(angle_1) * kOuterRadius);
    const QPointF outer_2 = center + QPointF(std::cos(angle_2) * kOuterRadius, std::sin(angle_2) * kOuterRadius);
    const QPointF inner_1 = center + QPointF(std::cos(angle_1) * kInnerRadius, std::sin(angle_1) * kInnerRadius);
    const QPointF inner_2 = center + QPointF(std::cos(angle_2) * kInnerRadius, std::sin(angle_2) * kInnerRadius);
    const int vertex = section * 12;
    write(vertex, static_cast<float>(center.x()), static_cast<float>(center.y()), background);
    write(vertex + 1, static_cast<float>(inner_1.x()), static_cast<float>(inner_1.y()), background);
    write(vertex + 2, static_cast<float>(inner_2.x()), static_cast<float>(inner_2.y()), background);
    write(vertex + 3, static_cast<float>(inner_1.x()), static_cast<float>(inner_1.y()), color);
    write(vertex + 4, static_cast<float>(outer_1.x()), static_cast<float>(outer_1.y()), color);
    write(vertex + 5, static_cast<float>(outer_2.x()), static_cast<float>(outer_2.y()), color);
    write(vertex + 6, static_cast<float>(inner_1.x()), static_cast<float>(inner_1.y()), color);
    write(vertex + 7, static_cast<float>(outer_2.x()), static_cast<float>(outer_2.y()), color);
    write(vertex + 8, static_cast<float>(inner_2.x()), static_cast<float>(inner_2.y()), color);
    write(vertex + 9, static_cast<float>(center.x()), static_cast<float>(center.y()), background);
    write(vertex + 10, static_cast<float>(inner_2.x()), static_cast<float>(inner_2.y()), background);
    write(vertex + 11, static_cast<float>(inner_1.x()), static_cast<float>(inner_1.y()), background);
  }
  auto* node = new QSGGeometryNode;
  node->setGeometry(geometry);
  node->setFlag(QSGNode::OwnsGeometry);
  node->setMaterial(new QSGVertexColorMaterial);
  node->setFlag(QSGNode::OwnsMaterial);
  return node;
}

void appendSnapshot(QSGNode* parent, const QList<geometry::Sample>& samples,
                    const std::optional<geometry::Sample>& predecessor, const QSizeF& size, const QColor& cpu_color,
                    const QColor& memory_color, const QColor& background) {
  const MetricGeometry cpu = geometry::buildMetricGeometry(samples, Metric::Cpu, size, predecessor);
  const MetricGeometry memory = geometry::buildMetricGeometry(samples, Metric::Memory, size, predecessor);
  const auto appendRuns = [&](const MetricGeometry& metric, const QColor& color, auto make_node) {
    for (const CurveRun& run : metric.runs) {
      parent->appendChildNode(make_node(renderPoints(run), color));
    }
  };

  appendRuns(cpu, cpu_color, [&](const QList<Point>& points, const QColor& color) {
    return areaNode(points, color, static_cast<float>(size.height()));
  });
  appendRuns(memory, memory_color, [&](const QList<Point>& points, const QColor& color) {
    return areaNode(points, color, static_cast<float>(size.height()));
  });
  QColor cpu_glow = cpu_color;
  QColor memory_glow = memory_color;
  cpu_glow.setAlphaF(0.07);
  memory_glow.setAlphaF(0.07);
  appendRuns(cpu, cpu_glow,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 6.0F); });
  appendRuns(memory, memory_glow,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 6.0F); });
  QColor cpu_fringe = cpu_color;
  QColor memory_fringe = memory_color;
  cpu_fringe.setAlphaF(0.35);
  memory_fringe.setAlphaF(0.35);
  appendRuns(cpu, cpu_fringe,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 4.0F); });
  appendRuns(memory, memory_fringe,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 4.0F); });
  appendRuns(cpu, cpu_color,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 2.0F); });
  appendRuns(memory, memory_color,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 2.0F); });
  const qreal marker_inset = std::min(4.0, size.width() / 2.0);
  if (cpu.current_endpoint) {
    QPointF marker = *cpu.current_endpoint;
    marker.setX(std::clamp(marker.x(), marker_inset, size.width() - marker_inset));
    parent->appendChildNode(ringNode(marker, cpu_color, background));
  }
  if (memory.current_endpoint) {
    QPointF marker = *memory.current_endpoint;
    marker.setX(std::clamp(marker.x(), marker_inset, size.width() - marker_inset));
    parent->appendChildNode(ringNode(marker, memory_color, background));
  }
}
}  // namespace

ResourceHistorySeries::ResourceHistorySeries(QQuickItem* parent) : QQuickItem(parent) {
  setFlag(ItemHasContents);
  setClip(true);
  transition_timer_.setInterval(16);
  connect(&transition_timer_, &QTimer::timeout, this, [this] {
    update();
    if (!transition_clock_.isValid() || transition_clock_.elapsed() >= transition_duration_) {
      transition_timer_.stop();
      previous_snapshot_ = {};
    }
  });
  connect(this, &QQuickItem::widthChanged, this, &ResourceHistorySeries::repaintImmediately);
  connect(this, &QQuickItem::heightChanged, this, &ResourceHistorySeries::repaintImmediately);
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
  cacheModel(false);
  emit modelChanged();
}

QColor ResourceHistorySeries::cpuColor() const { return cpu_color_; }
void ResourceHistorySeries::setCpuColor(const QColor& color) {
  if (std::exchange(cpu_color_, color) != color) {
    repaintImmediately();
    emit cpuColorChanged();
  }
}
QColor ResourceHistorySeries::memoryColor() const { return memory_color_; }
void ResourceHistorySeries::setMemoryColor(const QColor& color) {
  if (std::exchange(memory_color_, color) != color) {
    repaintImmediately();
    emit memoryColorChanged();
  }
}
QColor ResourceHistorySeries::plotBackgroundColor() const { return plot_background_color_; }
void ResourceHistorySeries::setPlotBackgroundColor(const QColor& color) {
  if (std::exchange(plot_background_color_, color) != color) {
    repaintImmediately();
    emit plotBackgroundColorChanged();
  }
}
int ResourceHistorySeries::transitionDuration() const { return transition_duration_; }
void ResourceHistorySeries::setTransitionDuration(int duration) {
  duration = std::max(0, duration);
  if (std::exchange(transition_duration_, duration) != duration) {
    repaintImmediately();
    emit transitionDurationChanged();
  }
}

void ResourceHistorySeries::reconnectModel() {
  if (!model_) {
    return;
  }
  const auto refresh = [this] { cacheModel(true); };
  connect(model_, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ResourceHistorySeries::capturePredecessor);
  connect(model_, &QAbstractItemModel::rowsInserted, this, refresh);
  connect(model_, &QAbstractItemModel::rowsRemoved, this, refresh);
  connect(model_, &QAbstractItemModel::dataChanged, this, refresh);
  connect(model_, &QAbstractItemModel::modelReset, this, refresh);
  connect(model_, &QObject::destroyed, this, [this] {
    model_ = nullptr;
    cacheModel(false);
    emit modelChanged();
  });
}

void ResourceHistorySeries::capturePredecessor(const QModelIndex& parent, int first, int last) {
  if (parent.isValid() || first != 0) {
    return;
  }
  const std::optional<geometry::Sample> predecessor = readSample(model_, last);
  if (!predecessor) {
    return;
  }
  snapshot_.predecessor = predecessor;
  previous_snapshot_.predecessor = predecessor;
}

void ResourceHistorySeries::cacheModel(bool animate) {
  Snapshot new_snapshot{.samples = readSamples(model_), .predecessor = snapshot_.predecessor};

  if (animate && transition_duration_ > 0 && (!snapshot_.samples.isEmpty() || transition_timer_.isActive())) {
    if (previous_snapshot_.samples.isEmpty() || !transition_timer_.isActive()) {
      previous_snapshot_ = snapshot_;
    }
    previous_snapshot_.predecessor = new_snapshot.predecessor;
    transition_clock_.restart();
    transition_timer_.start();
  } else {
    previous_snapshot_ = {};
    transition_clock_.invalidate();
    transition_timer_.stop();
  }
  snapshot_ = std::move(new_snapshot);
  update();
}

void ResourceHistorySeries::repaintImmediately() {
  previous_snapshot_ = {};
  transition_clock_.invalidate();
  transition_timer_.stop();
  update();
}

QSGNode* ResourceHistorySeries::updatePaintNode(QSGNode* old_node, UpdatePaintNodeData* update_data) {
  Q_UNUSED(update_data)
  QSGNode* root = old_node != nullptr ? old_node : new QSGNode;
  clearChildren(root);
  if (snapshot_.samples.isEmpty() || width() <= 0 || height() <= 0) {
    return root;
  }

  const QSizeF size(width(), height());
  const bool transitioning =
      !previous_snapshot_.samples.isEmpty() && transition_clock_.isValid() && transition_duration_ > 0;
  const qreal progress =
      transitioning ? std::clamp(static_cast<qreal>(transition_clock_.elapsed()) / transition_duration_, 0.0, 1.0)
                    : 1.0;
  if (transitioning && progress < 1.0) {
    auto* previous = new QSGOpacityNode;
    previous->setOpacity(static_cast<float>(1.0 - progress));
    appendSnapshot(previous, previous_snapshot_.samples, previous_snapshot_.predecessor, size, cpu_color_,
                   memory_color_, plot_background_color_);
    root->appendChildNode(previous);
  }
  auto* current = new QSGOpacityNode;
  current->setOpacity(static_cast<float>(progress));
  appendSnapshot(current, snapshot_.samples, snapshot_.predecessor, size, cpu_color_, memory_color_,
                 plot_background_color_);
  root->appendChildNode(current);
  return root;
}

}  // namespace dashboard::ui
