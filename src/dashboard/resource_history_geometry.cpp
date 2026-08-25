#include "resource_history_geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace dashboard::ui::geometry {
namespace {
constexpr qreal kWindowMilliseconds = 60'000.0;
constexpr qreal kHeadroom = 6.0;
constexpr qreal kPixelsPerPoint = 2.0;
constexpr int kMaximumSubdivisions = 32;

struct TimedPoint {
  qreal time;
  QPointF point;
};

[[nodiscard]] std::optional<double> valueFor(const Sample& sample, Metric metric) {
  return metric == Metric::Cpu ? sample.cpu_usage_ratio : sample.memory_usage_ratio;
}

[[nodiscard]] QPointF cubicPoint(const CubicSegment& segment, qreal progress) {
  const qreal inverse = 1.0 - progress;
  return (inverse * inverse * inverse * segment.start) + (3.0 * inverse * inverse * progress * segment.control_1) +
         (3.0 * inverse * progress * progress * segment.control_2) + (progress * progress * progress * segment.end);
}

[[nodiscard]] qreal endpointTangent(qreal first_slope, qreal second_slope, qreal first_width, qreal second_width) {
  qreal tangent = ((((2.0 * first_width) + second_width) * first_slope) - (first_width * second_slope)) /
                  (first_width + second_width);
  if (tangent * first_slope <= 0.0) {
    return 0.0;
  }
  if (first_slope * second_slope < 0.0 && std::abs(tangent) > std::abs(3.0 * first_slope)) {
    return 3.0 * first_slope;
  }
  return tangent;
}

[[nodiscard]] QList<qreal> monotoneTangents(const QList<TimedPoint>& points) {
  QList<qreal> tangents(points.size(), 0.0);
  if (points.size() < 2) {
    return tangents;
  }
  QList<qreal> widths;
  QList<qreal> slopes;
  widths.reserve(points.size() - 1);
  slopes.reserve(points.size() - 1);
  for (qsizetype index = 0; index + 1 < points.size(); ++index) {
    const qreal width = points.at(index + 1).time - points.at(index).time;
    widths.append(width);
    slopes.append(width > 0.0 ? (points.at(index + 1).point.y() - points.at(index).point.y()) / width : 0.0);
  }
  if (points.size() == 2) {
    tangents[0] = slopes.constFirst();
    tangents[1] = slopes.constFirst();
    return tangents;
  }
  tangents[0] = widths.at(0) > 0.0 && widths.at(1) > 0.0
                    ? endpointTangent(slopes.at(0), slopes.at(1), widths.at(0), widths.at(1))
                    : 0.0;
  for (qsizetype index = 1; index + 1 < points.size(); ++index) {
    const qreal before_width = widths.at(index - 1);
    const qreal after_width = widths.at(index);
    const qreal before_slope = slopes.at(index - 1);
    const qreal after_slope = slopes.at(index);
    if (before_width <= 0.0 || after_width <= 0.0 || before_slope * after_slope <= 0.0) {
      tangents[index] = 0.0;
      continue;
    }
    const qreal before_weight = (2.0 * after_width) + before_width;
    const qreal after_weight = after_width + (2.0 * before_width);
    tangents[index] = (before_weight + after_weight) / ((before_weight / before_slope) + (after_weight / after_slope));
  }
  const qsizetype last = points.size() - 1;
  tangents[last] =
      widths.at(last - 1) > 0.0 && widths.at(last - 2) > 0.0
          ? endpointTangent(slopes.at(last - 1), slopes.at(last - 2), widths.at(last - 1), widths.at(last - 2))
          : 0.0;
  return tangents;
}

[[nodiscard]] CubicSegment splitRight(const CubicSegment& cubic, qreal progress) {
  const QPointF first = cubic.start + ((cubic.control_1 - cubic.start) * progress);
  const QPointF second = cubic.control_1 + ((cubic.control_2 - cubic.control_1) * progress);
  const QPointF third = cubic.control_2 + ((cubic.end - cubic.control_2) * progress);
  const QPointF fourth = first + ((second - first) * progress);
  const QPointF fifth = second + ((third - second) * progress);
  return {.start = fourth + ((fifth - fourth) * progress), .control_1 = fifth, .control_2 = third, .end = cubic.end};
}

void completeRun(QList<TimedPoint>& points, QList<CurveRun>& runs, qreal cutoff, qreal plot_width) {
  if (points.size() < 2) {
    points.clear();
    return;
  }
  CurveRun run;
  const QList<qreal> tangents = monotoneTangents(points);
  for (const TimedPoint& point : std::as_const(points)) {
    if (point.time >= cutoff) {
      run.samples.append(point.point);
    }
  }
  for (qsizetype index = 0; index + 1 < points.size(); ++index) {
    const TimedPoint start = points.at(index);
    const TimedPoint end = points.at(index + 1);
    const qreal width = end.time - start.time;
    const bool linear_fallback = width <= 0.0;
    const qreal minimum_y = std::min(start.point.y(), end.point.y());
    const qreal maximum_y = std::max(start.point.y(), end.point.y());
    CubicSegment cubic{
        .start = start.point,
        .control_1 = start.point + ((end.point - start.point) / 3.0),
        .control_2 = start.point + ((end.point - start.point) * (2.0 / 3.0)),
        .end = end.point,
    };
    if (!linear_fallback) {
      cubic.control_1.setY(std::clamp(start.point.y() + (tangents.at(index) * width / 3.0), minimum_y, maximum_y));
      cubic.control_2.setY(std::clamp(end.point.y() - (tangents.at(index + 1) * width / 3.0), minimum_y, maximum_y));
    }
    if (end.time < cutoff) {
      continue;
    }
    if (start.time < cutoff && width > 0.0) {
      cubic = splitRight(cubic, std::clamp((cutoff - start.time) / width, 0.0, 1.0));
      cubic.start.setX(0.0);
    }
    cubic.start.setX(std::clamp(cubic.start.x(), 0.0, plot_width));
    cubic.control_1.setX(std::clamp(cubic.control_1.x(), 0.0, plot_width));
    cubic.control_2.setX(std::clamp(cubic.control_2.x(), 0.0, plot_width));
    cubic.end.setX(std::clamp(cubic.end.x(), 0.0, plot_width));
    if (run.tessellated.isEmpty()) {
      run.tessellated.append(cubic.start);
    }
    run.cubics.append(cubic);
    const int subdivisions =
        std::clamp(static_cast<int>(std::ceil(std::abs(cubic.end.x() - cubic.start.x()) / kPixelsPerPoint)), 1,
                   kMaximumSubdivisions);
    for (int step = 1; step <= subdivisions; ++step) {
      run.tessellated.append(cubicPoint(cubic, static_cast<qreal>(step) / subdivisions));
    }
  }
  if (!run.cubics.isEmpty()) {
    runs.append(run);
  }
  points.clear();
}
}  // namespace

MetricGeometry buildMetricGeometry(const QList<Sample>& samples, Metric metric, const QSizeF& plot_size,
                                   const std::optional<Sample>& predecessor) {
  MetricGeometry result;
  if (samples.isEmpty() || plot_size.width() <= 0.0 || plot_size.height() <= 0.0) {
    return result;
  }

  const qint64 newest = samples.constLast().elapsed_milliseconds;
  const qreal cutoff = static_cast<qreal>(newest) - kWindowMilliseconds;
  const qreal usable_height = std::max(0.0, plot_size.height() - kHeadroom);
  QList<TimedPoint> run;
  QList<Sample> render_samples = samples;
  if (predecessor && predecessor->elapsed_milliseconds < samples.constFirst().elapsed_milliseconds) {
    render_samples.prepend(*predecessor);
  }
  for (const Sample& sample : std::as_const(render_samples)) {
    const std::optional<double> value = valueFor(sample, metric);
    if (!value) {
      completeRun(run, result.runs, cutoff, plot_size.width());
      continue;
    }
    const auto time = static_cast<qreal>(sample.elapsed_milliseconds);
    const qreal x_position = plot_size.width() * ((time - cutoff) / kWindowMilliseconds);
    const qreal ratio = std::clamp(*value, 0.0, 1.0);
    run.append({.time = time, .point = {x_position, kHeadroom + (usable_height * (1.0 - ratio))}});
  }
  completeRun(run, result.runs, cutoff, plot_size.width());

  if (const auto newest_value = valueFor(samples.constLast(), metric); newest_value) {
    const qreal ratio = std::clamp(*newest_value, 0.0, 1.0);
    result.current_endpoint = QPointF{plot_size.width(), kHeadroom + (usable_height * (1.0 - ratio))};
  }
  return result;
}

}  // namespace dashboard::ui::geometry
