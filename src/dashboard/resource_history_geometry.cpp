#include "resource_history_geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace dashboard::ui::geometry {
namespace {
constexpr qreal kWindowMilliseconds = 60'000.0;
constexpr qreal kHeadroom = 6.0;
constexpr qreal kPixelsPerPoint = 1.0;
constexpr int kMaximumSubdivisions = 64;
constexpr qreal kCpuSlowTimeConstantSeconds = 1.8;
constexpr qreal kCpuFastTimeConstantSeconds = 0.35;
constexpr qreal kCpuSlowChange = 0.03;
constexpr qreal kCpuFastChange = 0.25;
constexpr qreal kMemoryTimeConstantSeconds = 3.0;

struct TimedPoint {
  qreal time;
  QPointF point;
};

struct FilterState {
  qreal raw_ratio;
  qreal filtered_ratio;
  qint64 elapsed_milliseconds;
};

[[nodiscard]] std::optional<double> valueFor(const Sample& sample, Metric metric) {
  return metric == Metric::Cpu ? sample.cpu_usage_ratio : sample.memory_usage_ratio;
}

[[nodiscard]] QPointF cubicPoint(const CubicSegment& segment, qreal progress) {
  const qreal inverse = 1.0 - progress;
  return (inverse * inverse * inverse * segment.start) + (3.0 * inverse * inverse * progress * segment.control_1) +
         (3.0 * inverse * progress * progress * segment.control_2) + (progress * progress * progress * segment.end);
}

[[nodiscard]] qreal timeConstant(Metric metric, qreal raw_change) {
  if (metric == Metric::Memory) {
    return kMemoryTimeConstantSeconds;
  }
  const qreal progress = std::clamp((raw_change - kCpuSlowChange) / (kCpuFastChange - kCpuSlowChange), 0.0, 1.0);
  return kCpuSlowTimeConstantSeconds + (progress * (kCpuFastTimeConstantSeconds - kCpuSlowTimeConstantSeconds));
}

[[nodiscard]] qreal filterRatio(qreal raw_ratio, qint64 elapsed_milliseconds, Metric metric,
                                std::optional<FilterState>& state) {
  if (!state) {
    state =
        FilterState{.raw_ratio = raw_ratio, .filtered_ratio = raw_ratio, .elapsed_milliseconds = elapsed_milliseconds};
    return raw_ratio;
  }
  const qint64 delta_milliseconds = elapsed_milliseconds - state->elapsed_milliseconds;
  qreal filtered = raw_ratio;
  if (delta_milliseconds > 0) {
    const qreal delta_seconds = static_cast<qreal>(delta_milliseconds) / 1'000.0;
    const qreal alpha = 1.0 - std::exp(-delta_seconds / timeConstant(metric, std::abs(raw_ratio - state->raw_ratio)));
    filtered = state->filtered_ratio + ((raw_ratio - state->filtered_ratio) * alpha);
  }
  state = FilterState{.raw_ratio = raw_ratio, .filtered_ratio = filtered, .elapsed_milliseconds = elapsed_milliseconds};
  return filtered;
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

void completeRun(QList<TimedPoint>& points, QList<CurveRun>& runs, qreal cutoff) {
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
    cubic.start.setX(std::max(cubic.start.x(), 0.0));
    cubic.control_1.setX(std::max(cubic.control_1.x(), 0.0));
    cubic.control_2.setX(std::max(cubic.control_2.x(), 0.0));
    cubic.end.setX(std::max(cubic.end.x(), 0.0));
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
                                   qreal window_end_milliseconds, const QList<Sample>& retained_prefix) {
  MetricGeometry result;
  if (samples.isEmpty() || plot_size.width() <= 0.0 || plot_size.height() <= 0.0) {
    return result;
  }

  const qreal cutoff = window_end_milliseconds - kWindowMilliseconds;
  const qreal usable_height = std::max(0.0, plot_size.height() - kHeadroom);
  QList<TimedPoint> run;
  QList<Sample> render_samples = retained_prefix;
  render_samples.append(samples);
  std::optional<FilterState> filter_state;
  std::optional<qreal> newest_filtered_ratio;
  for (qsizetype index = 0; index < render_samples.size(); ++index) {
    const Sample& sample = render_samples.at(index);
    const std::optional<double> value = valueFor(sample, metric);
    if (!value) {
      completeRun(run, result.runs, cutoff);
      filter_state.reset();
      continue;
    }
    const auto time = static_cast<qreal>(sample.elapsed_milliseconds);
    const qreal x_position = plot_size.width() * ((time - cutoff) / kWindowMilliseconds);
    const qreal raw_ratio = std::clamp(*value, 0.0, 1.0);
    const qreal ratio = filterRatio(raw_ratio, sample.elapsed_milliseconds, metric, filter_state);
    run.append({.time = time, .point = {x_position, kHeadroom + (usable_height * (1.0 - ratio))}});
    if (index + 1 == render_samples.size()) {
      newest_filtered_ratio = ratio;
    }
  }
  completeRun(run, result.runs, cutoff);

  if (newest_filtered_ratio) {
    const qreal newest_x =
        plot_size.width() *
        ((static_cast<qreal>(samples.constLast().elapsed_milliseconds) - cutoff) / kWindowMilliseconds);
    result.current_endpoint = QPointF{newest_x, kHeadroom + (usable_height * (1.0 - *newest_filtered_ratio))};
  }
  return result;
}

std::optional<QPointF> curveIntersection(const MetricGeometry& geometry, qreal x_position) {
  constexpr qreal kEpsilon = 1.0e-6;
  for (const CurveRun& run : geometry.runs) {
    for (const CubicSegment& cubic : run.cubics) {
      const qreal minimum_x = std::min(cubic.start.x(), cubic.end.x());
      const qreal maximum_x = std::max(cubic.start.x(), cubic.end.x());
      if (x_position < minimum_x - kEpsilon || x_position > maximum_x + kEpsilon) {
        continue;
      }
      if (std::abs(cubic.end.x() - cubic.start.x()) <= kEpsilon) {
        if (std::abs(x_position - cubic.start.x()) <= kEpsilon) {
          return cubic.end;
        }
        continue;
      }
      qreal lower = 0.0;
      qreal upper = 1.0;
      for (int iteration = 0; iteration < 32; ++iteration) {
        const qreal middle = (lower + upper) / 2.0;
        if (cubicPoint(cubic, middle).x() < x_position) {
          lower = middle;
        } else {
          upper = middle;
        }
      }
      QPointF intersection = cubicPoint(cubic, (lower + upper) / 2.0);
      intersection.setX(x_position);
      return intersection;
    }
  }
  if (geometry.current_endpoint && std::abs(geometry.current_endpoint->x() - x_position) <= kEpsilon) {
    return geometry.current_endpoint;
  }
  return std::nullopt;
}

QList<FeatherVertex> buildFeatheredRibbon(const QList<QPointF>& points, qreal width, qreal layer_opacity) {
  QList<FeatherVertex> vertices;
  if (points.size() < 2 || width <= 0.0 || layer_opacity <= 0.0) {
    return vertices;
  }
  const qreal outer_half_width = width / 2.0;
  const qreal inner_half_width = std::max(0.0, outer_half_width - 1.0);
  QList<QPointF> normals;
  normals.reserve(points.size());
  const auto unitNormal = [](const QPointF& start, const QPointF& end) {
    const QPointF delta = end - start;
    const qreal length = std::hypot(delta.x(), delta.y());
    return length > 0.0 ? QPointF{-delta.y() / length, delta.x() / length} : QPointF{0.0, 1.0};
  };
  for (qsizetype index = 0; index < points.size(); ++index) {
    QPointF normal =
        index == 0 ? unitNormal(points.at(0), points.at(1)) : unitNormal(points.at(index - 1), points.at(index));
    if (index > 0 && index + 1 < points.size()) {
      const QPointF sum = normal + unitNormal(points.at(index), points.at(index + 1));
      const qreal length = std::hypot(sum.x(), sum.y());
      if (length > 0.001) {
        normal = sum / length;
      }
    }
    normals.append(normal);
  }
  vertices.reserve((points.size() - 1) * 12);
  const auto appendSide = [&](qsizetype first, qsizetype second, qreal direction) {
    const QPointF first_outer = points.at(first) + (normals.at(first) * outer_half_width * direction);
    const QPointF first_inner = points.at(first) + (normals.at(first) * inner_half_width * direction);
    const QPointF second_outer = points.at(second) + (normals.at(second) * outer_half_width * direction);
    const QPointF second_inner = points.at(second) + (normals.at(second) * inner_half_width * direction);
    vertices.append({.position = first_outer, .alpha = 0.0});
    vertices.append({.position = first_inner, .alpha = layer_opacity});
    vertices.append({.position = second_inner, .alpha = layer_opacity});
    vertices.append({.position = first_outer, .alpha = 0.0});
    vertices.append({.position = second_inner, .alpha = layer_opacity});
    vertices.append({.position = second_outer, .alpha = 0.0});
  };
  for (qsizetype index = 0; index + 1 < points.size(); ++index) {
    appendSide(index, index + 1, 1.0);
    appendSide(index, index + 1, -1.0);
  }
  return vertices;
}

}  // namespace dashboard::ui::geometry
