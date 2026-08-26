#include "resource_history_series.h"

#include <QAbstractItemModel>
#include <QSGClipNode>
#include <QSGGeometryNode>
#include <QSGOpacityNode>
#include <QSGTransformNode>
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

struct SceneRoot final : QSGNode {
  SceneRoot() {
    appendChildNode(curve_clip = new QSGClipNode);
    curve_clip->setIsRectangular(true);
    curve_clip->appendChildNode(curves = new QSGTransformNode);
    appendChildNode(cpu_marker = new QSGTransformNode);
    appendChildNode(memory_marker = new QSGTransformNode);
    cpu_marker->appendChildNode(cpu_marker_opacity = new QSGOpacityNode);
    memory_marker->appendChildNode(memory_marker_opacity = new QSGOpacityNode);
  }

  QSGClipNode* curve_clip{};
  QSGTransformNode* curves{};
  QSGTransformNode* cpu_marker{};
  QSGTransformNode* memory_marker{};
  QSGOpacityNode* cpu_marker_opacity{};
  QSGOpacityNode* memory_marker_opacity{};
  MetricGeometry cpu_geometry;
  MetricGeometry memory_geometry;
  quint64 geometry_revision{};
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

[[nodiscard]] QList<geometry::Sample> readSampleRange(QAbstractItemModel* model, int first, int last) {
  const QList<geometry::Sample> samples = readSamples(model);
  if (first < 0 || last < first || last >= samples.size()) {
    return {};
  }
  return samples.sliced(first, (last - first) + 1);
}

[[nodiscard]] QSGGeometryNode* lineNode(const QList<Point>& points, const QColor& color, float width, qreal opacity) {
  QList<QPointF> source;
  source.reserve(points.size());
  for (const Point& point : points) {
    source.append({point.x, point.y});
  }
  const QList<geometry::FeatherVertex> ribbon = geometry::buildFeatheredRibbon(source, width, opacity);
  auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), static_cast<int>(ribbon.size()));
  geometry->setDrawingMode(QSGGeometry::DrawTriangles);
  geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
  auto* vertices = geometry->vertexDataAsColoredPoint2D();
  for (qsizetype index = 0; index < ribbon.size(); ++index) {
    const auto& vertex = ribbon.at(index);
    const auto alpha = static_cast<std::uint8_t>(std::clamp(vertex.alpha, 0.0, 1.0) * 255.0);
    const auto premultiply = [alpha](int channel) {
      return static_cast<std::uint8_t>((channel * static_cast<int>(alpha)) / 255);
    };
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes a vertex buffer pointer.
    vertices[index].set(static_cast<float>(vertex.position.x()), static_cast<float>(vertex.position.y()),
                        premultiply(color.red()), premultiply(color.green()), premultiply(color.blue()), alpha);
  }
  auto* node = new QSGGeometryNode;
  node->setGeometry(geometry);
  node->setFlag(QSGNode::OwnsGeometry);
  node->setMaterial(new QSGVertexColorMaterial);
  node->setFlag(QSGNode::OwnsMaterial);
  return node;
}

[[nodiscard]] QSGGeometryNode* areaNode(const QList<Point>& points, const QColor& color, float baseline) {
  constexpr std::uint8_t kCurveAlpha = 26;
  auto* geometry =
      new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), static_cast<int>(points.size() * 2));
  geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
  geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
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

[[nodiscard]] QSGGeometryNode* joinNode(const QColor& color) {
  return lineNode({{.x = -1.0F, .y = 0.0F}, {.x = 6.0F, .y = 0.0F}}, color, 2.0F, 1.0);
}

[[nodiscard]] QSGGeometryNode* ringNode(const QColor& color, const QColor& background) {
  constexpr int kSections = 48;
  constexpr float kOuterFeatherRadius = 4.5F;
  constexpr float kOuterSolidRadius = 3.5F;
  constexpr float kInnerSolidRadius = 2.5F;
  constexpr float kInnerFeatherRadius = 1.5F;
  constexpr int kVerticesPerSection = 21;
  auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), kSections * kVerticesPerSection);
  geometry->setDrawingMode(QSGGeometry::DrawTriangles);
  geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
  auto* vertices = geometry->vertexDataAsColoredPoint2D();
  const auto write = [&](int index, float x_position, float y_position, const QColor& value) {
    const auto alpha = static_cast<std::uint8_t>(value.alpha());
    const auto premultiply = [alpha](int channel) {
      return static_cast<std::uint8_t>((channel * static_cast<int>(alpha)) / 255);
    };
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes a vertex buffer pointer.
    vertices[index].set(x_position, y_position, premultiply(value.red()), premultiply(value.green()),
                        premultiply(value.blue()), alpha);
  };
  QColor transparent_color = color;
  transparent_color.setAlpha(0);
  constexpr float kTau = 6.2831853071795864769F;
  for (int section = 0; section < kSections; ++section) {
    const float angle_1 = kTau * static_cast<float>(section) / kSections;
    const float angle_2 = kTau * static_cast<float>(section + 1) / kSections;
    constexpr float kCenterX = 6.0F;
    const auto radialPoint = [&](float angle, float radius) {
      return QPointF(kCenterX + (std::cos(angle) * radius), std::sin(angle) * radius);
    };
    int vertex = section * kVerticesPerSection;
    const auto appendBand = [&](float outer_radius, const QColor& outer_color, float inner_radius,
                                const QColor& inner_color) {
      const QPointF outer_1 = radialPoint(angle_1, outer_radius);
      const QPointF outer_2 = radialPoint(angle_2, outer_radius);
      const QPointF inner_1 = radialPoint(angle_1, inner_radius);
      const QPointF inner_2 = radialPoint(angle_2, inner_radius);
      write(vertex++, static_cast<float>(outer_1.x()), static_cast<float>(outer_1.y()), outer_color);
      write(vertex++, static_cast<float>(inner_1.x()), static_cast<float>(inner_1.y()), inner_color);
      write(vertex++, static_cast<float>(inner_2.x()), static_cast<float>(inner_2.y()), inner_color);
      write(vertex++, static_cast<float>(outer_1.x()), static_cast<float>(outer_1.y()), outer_color);
      write(vertex++, static_cast<float>(inner_2.x()), static_cast<float>(inner_2.y()), inner_color);
      write(vertex++, static_cast<float>(outer_2.x()), static_cast<float>(outer_2.y()), outer_color);
    };
    appendBand(kOuterFeatherRadius, transparent_color, kOuterSolidRadius, color);
    appendBand(kOuterSolidRadius, color, kInnerSolidRadius, color);
    appendBand(kInnerSolidRadius, color, kInnerFeatherRadius, background);
    const QPointF inner_1 = radialPoint(angle_1, kInnerFeatherRadius);
    const QPointF inner_2 = radialPoint(angle_2, kInnerFeatherRadius);
    write(vertex++, kCenterX, 0.0F, background);
    write(vertex++, static_cast<float>(inner_1.x()), static_cast<float>(inner_1.y()), background);
    write(vertex, static_cast<float>(inner_2.x()), static_cast<float>(inner_2.y()), background);
  }
  auto* node = new QSGGeometryNode;
  node->setGeometry(geometry);
  node->setFlag(QSGNode::OwnsGeometry);
  node->setMaterial(new QSGVertexColorMaterial);
  node->setFlag(QSGNode::OwnsMaterial);
  return node;
}

void rebuildSnapshot(SceneRoot* root, const QList<geometry::Sample>& samples,
                     const QList<geometry::Sample>& retained_prefix, const QSizeF& size, const QColor& cpu_color,
                     const QColor& memory_color, const QColor& background, qreal window_end_milliseconds) {
  clearChildren(root->curves);
  clearChildren(root->cpu_marker_opacity);
  clearChildren(root->memory_marker_opacity);
  root->cpu_geometry =
      geometry::buildMetricGeometry(samples, Metric::Cpu, size, window_end_milliseconds, retained_prefix);
  root->memory_geometry =
      geometry::buildMetricGeometry(samples, Metric::Memory, size, window_end_milliseconds, retained_prefix);
  const auto appendRuns = [&](const MetricGeometry& metric, const QColor& color, auto make_node) {
    for (const CurveRun& run : metric.runs) {
      root->curves->appendChildNode(make_node(renderPoints(run), color));
    }
  };

  appendRuns(root->cpu_geometry, cpu_color, [&](const QList<Point>& points, const QColor& color) {
    return areaNode(points, color, static_cast<float>(size.height()));
  });
  appendRuns(root->memory_geometry, memory_color, [&](const QList<Point>& points, const QColor& color) {
    return areaNode(points, color, static_cast<float>(size.height()));
  });
  appendRuns(root->cpu_geometry, cpu_color,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 5.0F, 0.04); });
  appendRuns(root->memory_geometry, memory_color,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 5.0F, 0.04); });
  appendRuns(root->cpu_geometry, cpu_color,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 3.0F, 0.12); });
  appendRuns(root->memory_geometry, memory_color,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 3.0F, 0.12); });
  appendRuns(root->cpu_geometry, cpu_color,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 2.0F, 1.0); });
  appendRuns(root->memory_geometry, memory_color,
             [](const QList<Point>& points, const QColor& color) { return lineNode(points, color, 2.0F, 1.0); });
  root->cpu_marker_opacity->appendChildNode(joinNode(cpu_color));
  root->cpu_marker_opacity->appendChildNode(ringNode(cpu_color, background));
  root->memory_marker_opacity->appendChildNode(joinNode(memory_color));
  root->memory_marker_opacity->appendChildNode(ringNode(memory_color, background));
}

void positionMarker(QSGTransformNode* marker, QSGOpacityNode* opacity, const MetricGeometry& metric,
                    qreal local_boundary_x, qreal visible_x) {
  const std::optional<QPointF> intersection = geometry::curveIntersection(metric, local_boundary_x);
  opacity->setOpacity(intersection ? 1.0F : 0.0F);
  if (!intersection) {
    return;
  }
  QMatrix4x4 matrix;
  matrix.translate(static_cast<float>(visible_x), static_cast<float>(intersection->y()));
  marker->setMatrix(matrix);
}
}  // namespace

ResourceHistorySeries::ResourceHistorySeries(QQuickItem* parent) : QQuickItem(parent) {
  setFlag(ItemHasContents);
  setClip(true);
  transition_animation_.setStartValue(0.0);
  transition_animation_.setEndValue(1.0);
  connect(&transition_animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
    window_current_milliseconds_ =
        interpolateWindowEnd(window_start_milliseconds_, window_target_milliseconds_, value.toDouble());
    update();
  });
  connect(&transition_animation_, &QVariantAnimation::finished, this, [this] {
    window_start_milliseconds_ = window_target_milliseconds_;
    window_current_milliseconds_ = window_target_milliseconds_;
    update();
  });
  connect(this, &QQuickItem::widthChanged, this, [this] { rebuildAt(currentWindowEnd()); });
  connect(this, &QQuickItem::heightChanged, this, [this] { rebuildAt(currentWindowEnd()); });
}

QAbstractItemModel* ResourceHistorySeries::model() const { return model_; }

void ResourceHistorySeries::setModel(QAbstractItemModel* model) {
  if (model_ == model) {
    return;
  }
  if (model_ != nullptr) {
    disconnect(model_, nullptr, this, nullptr);
  }
  model_ = model;
  reconnectModel();
  snapshot_ = {};
  cacheModel(false);
  emit modelChanged();
}

QColor ResourceHistorySeries::cpuColor() const { return cpu_color_; }
void ResourceHistorySeries::setCpuColor(const QColor& color) {
  if (std::exchange(cpu_color_, color) != color) {
    rebuildAt(currentWindowEnd());
    emit cpuColorChanged();
  }
}
QColor ResourceHistorySeries::memoryColor() const { return memory_color_; }
void ResourceHistorySeries::setMemoryColor(const QColor& color) {
  if (std::exchange(memory_color_, color) != color) {
    rebuildAt(currentWindowEnd());
    emit memoryColorChanged();
  }
}
QColor ResourceHistorySeries::plotBackgroundColor() const { return plot_background_color_; }
void ResourceHistorySeries::setPlotBackgroundColor(const QColor& color) {
  if (std::exchange(plot_background_color_, color) != color) {
    rebuildAt(currentWindowEnd());
    emit plotBackgroundColorChanged();
  }
}
int ResourceHistorySeries::transitionDuration() const { return transition_duration_; }
void ResourceHistorySeries::setTransitionDuration(int duration) {
  duration = std::max(0, duration);
  if (std::exchange(transition_duration_, duration) != duration) {
    emit transitionDurationChanged();
  }
}

void ResourceHistorySeries::reconnectModel() {
  if (model_ == nullptr) {
    return;
  }
  const auto refresh = [this] { cacheModel(true); };
  connect(model_, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ResourceHistorySeries::captureRemovedPrefix);
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

void ResourceHistorySeries::captureRemovedPrefix(const QModelIndex& parent, int first, int last) {
  if (parent.isValid() || first != 0) {
    return;
  }
  snapshot_.retained_prefix.append(readSampleRange(model_, first, last));
}

void ResourceHistorySeries::cacheModel(bool animate) {
  Snapshot new_snapshot{.samples = readSamples(model_), .retained_prefix = snapshot_.retained_prefix};
  const std::optional<qreal> newest = new_snapshot.samples.isEmpty()
                                          ? std::nullopt
                                          : std::optional<qreal>(new_snapshot.samples.constLast().elapsed_milliseconds);

  if (!animate) {
    transition_animation_.stop();
    has_window_end_ = newest.has_value();
    if (newest) {
      window_start_milliseconds_ = *newest;
      window_current_milliseconds_ = *newest;
      window_target_milliseconds_ = *newest;
    }
  } else if (!has_window_end_ && newest) {
    has_window_end_ = true;
    window_start_milliseconds_ = *newest;
    window_current_milliseconds_ = *newest;
    window_target_milliseconds_ = *newest;
  } else if (newest && *newest > window_target_milliseconds_) {
    const qreal displayed = currentWindowEnd();
    transition_animation_.stop();
    window_start_milliseconds_ = displayed;
    window_current_milliseconds_ = displayed;
    window_target_milliseconds_ = *newest;
    if (transition_duration_ > 0) {
      transition_animation_.setDuration(transition_duration_);
      transition_animation_.start();
    } else {
      window_start_milliseconds_ = window_target_milliseconds_;
      window_current_milliseconds_ = window_target_milliseconds_;
    }
  } else if (newest && *newest < window_target_milliseconds_) {
    window_start_milliseconds_ = *newest;
    window_current_milliseconds_ = *newest;
    window_target_milliseconds_ = *newest;
    transition_animation_.stop();
  }
  if (!new_snapshot.retained_prefix.isEmpty()) {
    const qreal cutoff = currentWindowEnd() - 60'000.0;
    qsizetype first_inside = 0;
    while (first_inside < new_snapshot.retained_prefix.size() &&
           static_cast<qreal>(new_snapshot.retained_prefix.at(first_inside).elapsed_milliseconds) < cutoff) {
      ++first_inside;
    }
    const qsizetype keep_from = first_inside > 0 ? first_inside - 1 : 0;
    if (keep_from > 0) {
      new_snapshot.retained_prefix.remove(0, keep_from);
    }
  }
  snapshot_ = std::move(new_snapshot);
  rebuildAt(currentWindowEnd());
}

void ResourceHistorySeries::rebuildAt(qreal window_end_milliseconds) {
  geometry_origin_milliseconds_ = window_end_milliseconds;
  ++geometry_revision_;
  update();
}

qreal ResourceHistorySeries::interpolateWindowEnd(qreal start, qreal target, qreal progress) {
  const qreal bounded = std::clamp(progress, 0.0, 1.0);
  const qreal smoothstep = bounded * bounded * (3.0 - (2.0 * bounded));
  return start + ((target - start) * smoothstep);
}

qreal ResourceHistorySeries::currentWindowEnd() const { return window_current_milliseconds_; }

QSGNode* ResourceHistorySeries::updatePaintNode(QSGNode* old_node, UpdatePaintNodeData* update_data) {
  Q_UNUSED(update_data)
  auto* root = old_node != nullptr ? dynamic_cast<SceneRoot*>(old_node) : new SceneRoot;
  Q_ASSERT(root != nullptr);
  if (snapshot_.samples.isEmpty() || width() <= 0 || height() <= 0) {
    clearChildren(root->curves);
    clearChildren(root->cpu_marker_opacity);
    clearChildren(root->memory_marker_opacity);
    root->geometry_revision = geometry_revision_;
    return root;
  }

  const QSizeF size(width(), height());
  constexpr qreal kEndpointZoneWidth = 10.0;
  const qreal drawable_width = std::max(0.0, size.width() - kEndpointZoneWidth);
  const QSizeF drawable_size(drawable_width, size.height());
  if (root->geometry_revision != geometry_revision_) {
    root->curve_clip->setClipRect(QRectF(0.0, 0.0, drawable_width, size.height()));
    rebuildSnapshot(root, snapshot_.samples, snapshot_.retained_prefix, drawable_size, cpu_color_, memory_color_,
                    plot_background_color_, geometry_origin_milliseconds_);
    root->geometry_revision = geometry_revision_;
  }
  const qreal translation = -drawable_width * ((currentWindowEnd() - geometry_origin_milliseconds_) / 60'000.0);
  QMatrix4x4 curve_matrix;
  curve_matrix.translate(static_cast<float>(translation), 0.0F);
  root->curves->setMatrix(curve_matrix);
  const qreal local_boundary_x = drawable_width - translation;
  positionMarker(root->cpu_marker, root->cpu_marker_opacity, root->cpu_geometry, local_boundary_x, drawable_width);
  positionMarker(root->memory_marker, root->memory_marker_opacity, root->memory_geometry, local_boundary_x,
                 drawable_width);
  return root;
}

}  // namespace dashboard::ui
