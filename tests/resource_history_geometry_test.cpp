#include "resource_history_geometry.h"

#include <QtTest>

#include <algorithm>

using dashboard::ui::geometry::buildMetricGeometry;
using dashboard::ui::geometry::Metric;
using dashboard::ui::geometry::Sample;

class ResourceHistoryGeometryTest : public QObject {
  Q_OBJECT

 private slots:
  void curvesPassThroughEveryRealSample();
  void joinsAreC1ContinuousForNonuniformTimestamps();
  void monotoneSectionsAndExtremaDoNotOvershoot();
  void duplicateTimestampUsesLinearFallback();
  void tessellationIsDenseAndBounded();
  void predecessorCurveIsClippedAtLeftBoundary();
  void missingPredecessorValuePreservesGap();
  void metricGapsSplitIndependently();
  void clampsRatiosAndPreservesHeadroomAndCurrentEligibility();
};

// NOLINTBEGIN(modernize-use-designated-initializers) Compact samples make the test scenarios readable.
void ResourceHistoryGeometryTest::
    curvesPassThroughEveryRealSample() {  // NOLINT(readability-convert-member-functions-to-static)
  const QList<Sample> samples = {{0, 0.2, 0.7}, {20'000, 0.8, 0.5}, {40'000, 0.4, 0.6}};
  const auto geometry = buildMetricGeometry(samples, Metric::Cpu, {600.0, 100.0});

  QCOMPARE(geometry.runs.size(), 1);
  const auto& run = geometry.runs.constFirst();
  QCOMPARE(run.samples.size(), 3);
  QCOMPARE(run.cubics.size(), 2);
  QCOMPARE(run.tessellated.constFirst(), run.samples.constFirst());
  QCOMPARE(run.cubics.at(0).start, run.samples.at(0));
  QCOMPARE(run.cubics.at(0).end, run.samples.at(1));
  QCOMPARE(run.cubics.at(1).start, run.samples.at(1));
  QCOMPARE(run.cubics.at(1).end, run.samples.at(2));
  QVERIFY(run.tessellated.contains(run.samples.at(1)));
  QCOMPARE(run.tessellated.constLast(), run.samples.constLast());
}

void ResourceHistoryGeometryTest::
    joinsAreC1ContinuousForNonuniformTimestamps() {  // NOLINT(readability-convert-member-functions-to-static)
  const QList<Sample> samples = {
      {0, 0.1, std::nullopt}, {7'000, 0.3, std::nullopt}, {41'000, 0.7, std::nullopt}, {60'000, 0.9, std::nullopt}};
  const auto& run = buildMetricGeometry(samples, Metric::Cpu, {600.0, 120.0}).runs.constFirst();
  for (qsizetype index = 0; index + 1 < run.cubics.size(); ++index) {
    const auto& before = run.cubics.at(index);
    const auto& after = run.cubics.at(index + 1);
    const qreal incoming = (before.end.y() - before.control_2.y()) / (before.end.x() - before.control_2.x());
    const qreal outgoing = (after.control_1.y() - after.start.y()) / (after.control_1.x() - after.start.x());
    QVERIFY(std::abs(incoming - outgoing) < 1.0e-9);
  }
}

void ResourceHistoryGeometryTest::
    monotoneSectionsAndExtremaDoNotOvershoot() {  // NOLINT(readability-convert-member-functions-to-static)
  const QList<Sample> samples = {
      {0, 0.1, std::nullopt}, {1'000, 0.9, std::nullopt}, {59'000, 0.2, std::nullopt}, {60'000, 0.8, std::nullopt}};
  const auto geometry = buildMetricGeometry(samples, Metric::Cpu, {960.0, 120.0});
  const auto& run = geometry.runs.constFirst();

  for (const auto& cubic : run.cubics) {
    const qreal minimum_y = std::min(cubic.start.y(), cubic.end.y());
    const qreal maximum_y = std::max(cubic.start.y(), cubic.end.y());
    QVERIFY(cubic.control_1.x() >= cubic.start.x());
    QVERIFY(cubic.control_1.x() <= cubic.end.x());
    QVERIFY(cubic.control_2.x() >= cubic.start.x());
    QVERIFY(cubic.control_2.x() <= cubic.end.x());
    QVERIFY(cubic.control_1.y() >= minimum_y && cubic.control_1.y() <= maximum_y);
    QVERIFY(cubic.control_2.y() >= minimum_y && cubic.control_2.y() <= maximum_y);
  }
  for (const QPointF& point : run.tessellated) {
    QVERIFY(point.x() >= 0.0 && point.x() <= 960.0);
    QVERIFY(point.y() >= 6.0 && point.y() <= 120.0);
  }
  QCOMPARE(run.cubics.at(0).control_2.y(), run.cubics.at(0).end.y());
  QCOMPARE(run.cubics.at(1).control_1.y(), run.cubics.at(1).start.y());
}

void ResourceHistoryGeometryTest::
    duplicateTimestampUsesLinearFallback() {  // NOLINT(readability-convert-member-functions-to-static)
  const QList<Sample> samples = {
      {0, 0.2, std::nullopt}, {30'000, 0.4, std::nullopt}, {30'000, 0.8, std::nullopt}, {60'000, 0.6, std::nullopt}};
  const auto& cubic = buildMetricGeometry(samples, Metric::Cpu, {600.0, 100.0}).runs.constFirst().cubics.at(1);
  QCOMPARE(cubic.control_1, cubic.start + ((cubic.end - cubic.start) / 3.0));
  QCOMPARE(cubic.control_2, cubic.start + ((cubic.end - cubic.start) * (2.0 / 3.0)));
}

void ResourceHistoryGeometryTest::
    tessellationIsDenseAndBounded() {  // NOLINT(readability-convert-member-functions-to-static)
  const QList<Sample> samples = {{0, 0.2, std::nullopt}, {6'000, 0.4, std::nullopt}, {60'000, 0.8, std::nullopt}};
  const auto& run = buildMetricGeometry(samples, Metric::Cpu, {600.0, 100.0}).runs.constFirst();
  QCOMPARE(run.tessellated.size(), 1 + 30 + 32);
  for (const QPointF& point : run.tessellated) {
    QVERIFY(point.x() >= 0.0 && point.x() <= 600.0);
    QVERIFY(point.y() >= 6.0 && point.y() <= 100.0);
  }
}

void ResourceHistoryGeometryTest::
    predecessorCurveIsClippedAtLeftBoundary() {  // NOLINT(readability-convert-member-functions-to-static)
  const QList<Sample> samples = {{62'000, 0.8, std::nullopt}, {121'000, 0.4, std::nullopt}};
  const Sample predecessor = {60'000, 0.2, std::nullopt};
  const auto& run = buildMetricGeometry(samples, Metric::Cpu, {600.0, 100.0}, predecessor).runs.constFirst();
  QCOMPARE(run.cubics.constFirst().start.x(), 0.0);
  QCOMPARE(run.tessellated.constFirst().x(), 0.0);
  QVERIFY(run.tessellated.constFirst().y() > 6.0 && run.tessellated.constFirst().y() < 100.0);
}

void ResourceHistoryGeometryTest::
    missingPredecessorValuePreservesGap() {  // NOLINT(readability-convert-member-functions-to-static)
  const QList<Sample> samples = {{62'000, 0.8, std::nullopt}};
  const Sample predecessor = {60'000, std::nullopt, std::nullopt};
  const auto geometry = buildMetricGeometry(samples, Metric::Cpu, {600.0, 100.0}, predecessor);
  QVERIFY(geometry.runs.isEmpty());
}

void ResourceHistoryGeometryTest::
    metricGapsSplitIndependently() {  // NOLINT(readability-convert-member-functions-to-static)
  const QList<Sample> samples = {
      {0, 0.2, 0.4},      {10'000, 0.3, 0.5}, {20'000, std::nullopt, 0.6}, {30'000, 0.5, std::nullopt},
      {40'000, 0.6, 0.7}, {50'000, 0.7, 0.8}};

  const auto cpu = buildMetricGeometry(samples, Metric::Cpu, {600.0, 100.0});
  const auto memory = buildMetricGeometry(samples, Metric::Memory, {600.0, 100.0});
  QCOMPARE(cpu.runs.size(), 2);
  QCOMPARE(cpu.runs.at(0).samples.size(), 2);
  QCOMPARE(cpu.runs.at(1).samples.size(), 3);
  QCOMPARE(memory.runs.size(), 2);
  QCOMPARE(memory.runs.at(0).samples.size(), 3);
  QCOMPARE(memory.runs.at(1).samples.size(), 2);
}

void ResourceHistoryGeometryTest::
    clampsRatiosAndPreservesHeadroomAndCurrentEligibility() {  // NOLINT(readability-convert-member-functions-to-static)
  const QList<Sample> samples = {{0, -0.5, 1.5}, {60'000, 2.0, std::nullopt}};
  const auto cpu = buildMetricGeometry(samples, Metric::Cpu, {600.0, 100.0});
  const auto memory = buildMetricGeometry(samples, Metric::Memory, {600.0, 100.0});

  QCOMPARE(cpu.runs.constFirst().samples.constFirst(), QPointF(0.0, 100.0));
  QCOMPARE(cpu.runs.constFirst().samples.constLast(), QPointF(600.0, 6.0));
  QVERIFY(cpu.current_endpoint.has_value());
  QCOMPARE(*cpu.current_endpoint, QPointF(600.0, 6.0));
  QVERIFY(memory.runs.isEmpty());
  QVERIFY(!memory.current_endpoint.has_value());
}
// NOLINTEND(modernize-use-designated-initializers)

QTEST_GUILESS_MAIN(ResourceHistoryGeometryTest)

#include "resource_history_geometry_test.moc"
