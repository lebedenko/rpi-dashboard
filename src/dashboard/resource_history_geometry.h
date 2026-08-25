#pragma once

#include <QList>
#include <QPointF>
#include <QSizeF>

#include <cstdint>
#include <optional>

namespace dashboard::ui::geometry {

struct Sample {
  qint64 elapsed_milliseconds{};
  std::optional<double> cpu_usage_ratio;
  std::optional<double> memory_usage_ratio;
};

enum class Metric : std::uint8_t { Cpu, Memory };

struct CubicSegment {
  QPointF start;
  QPointF control_1;
  QPointF control_2;
  QPointF end;
};

struct CurveRun {
  QList<QPointF> samples;
  QList<CubicSegment> cubics;
  QList<QPointF> tessellated;
};

struct MetricGeometry {
  QList<CurveRun> runs;
  std::optional<QPointF> current_endpoint;
};

struct FeatherVertex {
  QPointF position;
  qreal alpha{};
};

[[nodiscard]] MetricGeometry buildMetricGeometry(const QList<Sample>& samples, Metric metric, const QSizeF& plot_size,
                                                 qreal window_end_milliseconds,
                                                 const QList<Sample>& retained_prefix = {});
[[nodiscard]] std::optional<QPointF> curveIntersection(const MetricGeometry& geometry, qreal x_position);
[[nodiscard]] QList<FeatherVertex> buildFeatheredRibbon(const QList<QPointF>& points, qreal width, qreal layer_opacity);

}  // namespace dashboard::ui::geometry
