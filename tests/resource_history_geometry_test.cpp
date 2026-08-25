#include "resource_history_geometry.h"

#include <QtTest>

#include <algorithm>
#include <cmath>

using dashboard::ui::geometry::buildMetricGeometry;
using dashboard::ui::geometry::Metric;
using dashboard::ui::geometry::Sample;

class ResourceHistoryGeometryTest : public QObject {
  Q_OBJECT

 private slots:
  void cpuFilteringSuppressesJitterAndRespondsToLargeChanges();
  void memoryFilteringIsCalmerAndHandlesIrregularIntervals();
  void nonPositiveDeltaUpdatesImmediately();
  void gapsResetEachMetricIndependently();
  void joinsAreC1ContinuousAndDoNotOvershoot();
  void tessellationIsDenseAndBounded();
  void retainedPrefixIsFilteredAndClippedAtLeftBoundary();
  void retainedPrefixPreservesMetricGaps();
  void clampsBeforeFilteringAndPreservesHeadroomAndCurrentEligibility();
  void positionsSamplesAgainstAnimatedWindowEnd();
  void curveIntersectionTracksNowAndRespectsGaps();
  void featheredRibbonFadesAtBothEdges();
};

namespace {
constexpr QSizeF kPlotSize{600.0, 100.0};

double ratioForY(qreal y_position) { return 1.0 - ((y_position - 6.0) / 94.0); }

void compareNear(double actual, double expected, double tolerance = 1.0e-9) {
  QVERIFY2(std::abs(actual - expected) <= tolerance,
           qPrintable(QStringLiteral("actual %1, expected %2").arg(actual, 0, 'g', 16).arg(expected, 0, 'g', 16)));
}
}  // namespace

// NOLINTBEGIN(modernize-use-designated-initializers,readability-convert-member-functions-to-static) Compact
// samples make the scenarios readable, and Qt invokes test slots as members.
void ResourceHistoryGeometryTest::cpuFilteringSuppressesJitterAndRespondsToLargeChanges() {
  const QList<Sample> jitter = {{0, 0.50, std::nullopt}, {1'000, 0.51, std::nullopt}};
  const auto jitter_run = buildMetricGeometry(jitter, Metric::Cpu, kPlotSize, 1'000).runs.constFirst();
  const double jitter_expected = 0.50 + ((0.51 - 0.50) * (1.0 - std::exp(-1.0 / 1.8)));
  compareNear(ratioForY(jitter_run.samples.constLast().y()), jitter_expected);

  const QList<Sample> jump = {{0, 0.50, std::nullopt}, {1'000, 0.75, std::nullopt}};
  const auto jump_run = buildMetricGeometry(jump, Metric::Cpu, kPlotSize, 1'000).runs.constFirst();
  const double jump_expected = 0.50 + ((0.75 - 0.50) * (1.0 - std::exp(-1.0 / 0.35)));
  compareNear(ratioForY(jump_run.samples.constLast().y()), jump_expected);
  QVERIFY(ratioForY(jump_run.samples.constLast().y()) - 0.50 >
          20.0 * (ratioForY(jitter_run.samples.constLast().y()) - 0.50));
}

void ResourceHistoryGeometryTest::memoryFilteringIsCalmerAndHandlesIrregularIntervals() {
  const QList<Sample> samples = {{0, std::nullopt, 0.2}, {500, std::nullopt, 0.8}, {2'500, std::nullopt, 0.8}};
  const auto run = buildMetricGeometry(samples, Metric::Memory, kPlotSize, 2'500).runs.constFirst();
  const double after_half_second = 0.2 + (0.6 * (1.0 - std::exp(-0.5 / 3.0)));
  const double after_two_more_seconds = after_half_second + ((0.8 - after_half_second) * (1.0 - std::exp(-2.0 / 3.0)));
  compareNear(ratioForY(run.samples.at(1).y()), after_half_second);
  compareNear(ratioForY(run.samples.at(2).y()), after_two_more_seconds);
  QVERIFY(after_two_more_seconds < 0.6);
}

void ResourceHistoryGeometryTest::nonPositiveDeltaUpdatesImmediately() {
  const QList<Sample> samples = {{1'000, 0.2, std::nullopt}, {1'000, 0.8, std::nullopt}, {500, 0.4, std::nullopt}};
  const auto run = buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 1'000).runs.constFirst();
  compareNear(ratioForY(run.samples.at(1).y()), 0.8);
  compareNear(ratioForY(run.samples.at(2).y()), 0.4);
  const auto& duplicate = run.cubics.constFirst();
  QCOMPARE(duplicate.control_1, duplicate.start + ((duplicate.end - duplicate.start) / 3.0));
  QCOMPARE(duplicate.control_2, duplicate.start + ((duplicate.end - duplicate.start) * (2.0 / 3.0)));
}

void ResourceHistoryGeometryTest::gapsResetEachMetricIndependently() {
  const QList<Sample> samples = {
      {0, 0.2, 0.4},     {1'000, 0.8, 0.8}, {2'000, std::nullopt, 0.8}, {3'000, 0.3, std::nullopt},
      {4'000, 0.4, 0.2}, {5'000, 0.5, 0.3}};
  const auto cpu = buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 5'000);
  const auto memory = buildMetricGeometry(samples, Metric::Memory, kPlotSize, 5'000);
  QCOMPARE(cpu.runs.size(), 2);
  QCOMPARE(memory.runs.size(), 2);
  compareNear(ratioForY(cpu.runs.at(1).samples.constFirst().y()), 0.3);
  compareNear(ratioForY(memory.runs.at(1).samples.constFirst().y()), 0.2);
}

void ResourceHistoryGeometryTest::joinsAreC1ContinuousAndDoNotOvershoot() {
  const QList<Sample> samples = {
      {0, 0.1, std::nullopt}, {7'000, 0.3, std::nullopt}, {41'000, 0.7, std::nullopt}, {60'000, 0.2, std::nullopt}};
  const auto run = buildMetricGeometry(samples, Metric::Cpu, {600.0, 120.0}, 60'000).runs.constFirst();
  for (qsizetype index = 0; index + 1 < run.cubics.size(); ++index) {
    const auto& before = run.cubics.at(index);
    const auto& after = run.cubics.at(index + 1);
    const qreal incoming = (before.end.y() - before.control_2.y()) / (before.end.x() - before.control_2.x());
    const qreal outgoing = (after.control_1.y() - after.start.y()) / (after.control_1.x() - after.start.x());
    compareNear(incoming, outgoing);
  }
  for (const auto& cubic : run.cubics) {
    const qreal minimum_y = std::min(cubic.start.y(), cubic.end.y());
    const qreal maximum_y = std::max(cubic.start.y(), cubic.end.y());
    QVERIFY(cubic.control_1.x() >= cubic.start.x() && cubic.control_1.x() <= cubic.end.x());
    QVERIFY(cubic.control_2.x() >= cubic.start.x() && cubic.control_2.x() <= cubic.end.x());
    QVERIFY(cubic.control_1.y() >= minimum_y && cubic.control_1.y() <= maximum_y);
    QVERIFY(cubic.control_2.y() >= minimum_y && cubic.control_2.y() <= maximum_y);
  }
}

void ResourceHistoryGeometryTest::tessellationIsDenseAndBounded() {
  const QList<Sample> samples = {{0, 0.2, std::nullopt}, {6'000, 0.4, std::nullopt}, {60'000, 0.8, std::nullopt}};
  const auto run = buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 60'000).runs.constFirst();
  QCOMPARE(run.tessellated.size(), 1 + 60 + 64);
  for (const QPointF& point : run.tessellated) {
    QVERIFY(point.x() >= 0.0 && point.x() <= 600.0);
    QVERIFY(point.y() >= 6.0 && point.y() <= 100.0);
  }
}

void ResourceHistoryGeometryTest::retainedPrefixIsFilteredAndClippedAtLeftBoundary() {
  const QList<Sample> samples = {{62'000, 0.8, std::nullopt}, {121'000, 0.4, std::nullopt}};
  const QList<Sample> retained_prefix = {{59'000, 0.1, std::nullopt}, {60'000, 0.2, std::nullopt}};
  const auto run = buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 121'000, retained_prefix).runs.constFirst();
  QCOMPARE(run.cubics.constFirst().start.x(), 0.0);
  QCOMPARE(run.tessellated.constFirst().x(), 0.0);
  QVERIFY(run.tessellated.constFirst().y() > 6.0 && run.tessellated.constFirst().y() < 100.0);
}

void ResourceHistoryGeometryTest::retainedPrefixPreservesMetricGaps() {
  const QList<Sample> samples = {{62'000, 0.8, std::nullopt}};
  const QList<Sample> retained_prefix = {{59'000, 0.2, 0.3}, {60'000, std::nullopt, 0.4}};
  QVERIFY(buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 62'000, retained_prefix).runs.isEmpty());
  QVERIFY(!buildMetricGeometry(samples, Metric::Memory, kPlotSize, 62'000, retained_prefix).runs.isEmpty());
}

void ResourceHistoryGeometryTest::clampsBeforeFilteringAndPreservesHeadroomAndCurrentEligibility() {
  const QList<Sample> samples = {{0, -0.5, 1.5}, {60'000, 2.0, std::nullopt}};
  const auto cpu = buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 60'000);
  const auto memory = buildMetricGeometry(samples, Metric::Memory, kPlotSize, 60'000);
  QCOMPARE(cpu.runs.constFirst().samples.constFirst(), QPointF(0.0, 100.0));
  QCOMPARE(cpu.runs.constFirst().samples.constLast(), QPointF(600.0, 6.0));
  QVERIFY(cpu.current_endpoint.has_value());
  QCOMPARE(*cpu.current_endpoint, QPointF(600.0, 6.0));
  QVERIFY(memory.runs.isEmpty());
  QVERIFY(!memory.current_endpoint.has_value());
}

void ResourceHistoryGeometryTest::positionsSamplesAgainstAnimatedWindowEnd() {
  const QList<Sample> samples = {{0, 0.2, std::nullopt}, {60'000, 0.4, std::nullopt}, {61'000, 0.8, std::nullopt}};
  const auto start = buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 60'000);
  const auto halfway = buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 60'500);
  const auto finish = buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 61'000);
  QVERIFY(start.runs.constFirst().samples.constLast().x() > 600.0);
  QCOMPARE(start.runs.constFirst().samples.at(1).x(), 600.0);
  QVERIFY(halfway.runs.constFirst().samples.constFirst().x() < 600.0);
  QVERIFY(halfway.runs.constFirst().samples.constLast().x() > 600.0);
  QCOMPARE(finish.runs.constFirst().samples.constLast().x(), 600.0);
  QCOMPARE(finish.current_endpoint->x(), 600.0);
}

void ResourceHistoryGeometryTest::curveIntersectionTracksNowAndRespectsGaps() {
  const QList<Sample> samples = {{0, 0.2, std::nullopt},      {30'000, 0.8, std::nullopt},
                                 {60'000, 0.4, std::nullopt}, {61'000, std::nullopt, std::nullopt},
                                 {62'000, 0.7, std::nullopt}, {63'000, 0.6, std::nullopt}};
  const auto geometry = buildMetricGeometry(samples, Metric::Cpu, kPlotSize, 60'000);
  QVERIFY(dashboard::ui::geometry::curveIntersection(geometry, 600.0).has_value());
  QVERIFY(dashboard::ui::geometry::curveIntersection(geometry, 595.0).has_value());
  QVERIFY(!dashboard::ui::geometry::curveIntersection(geometry, 615.0).has_value());
  QVERIFY(dashboard::ui::geometry::curveIntersection(geometry, 625.0).has_value());
  compareNear(dashboard::ui::geometry::curveIntersection(geometry, 630.0)->y(), geometry.current_endpoint->y());
}

void ResourceHistoryGeometryTest::featheredRibbonFadesAtBothEdges() {
  const QList<QPointF> points = {{0.0, 10.0}, {20.0, 10.0}};
  const auto vertices = dashboard::ui::geometry::buildFeatheredRibbon(points, 5.0, 0.12);
  QCOMPARE(vertices.size(), 12);
  QCOMPARE(vertices.at(0).alpha, 0.0);
  QCOMPARE(vertices.at(1).alpha, 0.12);
  QCOMPARE(vertices.at(5).alpha, 0.0);
  QCOMPARE(vertices.at(6).alpha, 0.0);
  QCOMPARE(vertices.at(7).alpha, 0.12);
  QCOMPARE(vertices.at(11).alpha, 0.0);
}
// NOLINTEND(modernize-use-designated-initializers,readability-convert-member-functions-to-static)

QTEST_GUILESS_MAIN(ResourceHistoryGeometryTest)

#include "resource_history_geometry_test.moc"
