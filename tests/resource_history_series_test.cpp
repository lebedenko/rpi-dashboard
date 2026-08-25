#include "resource_history_series.h"

#include "sysmetrics/system_metric_history_model.h"

#include <QSGClipNode>
#include <QSGGeometryNode>
#include <QSGNode>
#include <QSGOpacityNode>
#include <QSGTransformNode>
#include <QtTest>

#include <algorithm>
#include <cmath>

namespace dashboard::ui {

class ResourceHistorySeriesTest : public QObject {
  Q_OBJECT

 private slots:
  void defaultsMatchDashboardTheme();
  void irregularFrontRemovalPreservesVisiblePrefixAndPredecessor();
  void advancingTimestampAnimatesWindow();
  void smoothstepInterpolationIsDeterministic();
  void interruptedTransitionStartsFromDisplayedPosition();
  void nonAdvancingUpdateDoesNotRestartTransition();
  void modelReplacementRepaintsImmediately();
  void animationFramesReuseSceneGraphGeometry();
  void activeTransitionKeepsItsDuration();
  void endpointAssemblyIsHorizontalAndTracksNow();
};

// NOLINTBEGIN(readability-convert-member-functions-to-static) Qt invokes test slots as members.
void ResourceHistorySeriesTest::defaultsMatchDashboardTheme() {
  const ResourceHistorySeries series;
  QCOMPARE(series.cpuColor(), QColor(QStringLiteral("#36B9FF")));
  QCOMPARE(series.memoryColor(), QColor(QStringLiteral("#A66CFF")));
  QCOMPARE(series.plotBackgroundColor(), QColor(QStringLiteral("#041321")));
}

void ResourceHistorySeriesTest::irregularFrontRemovalPreservesVisiblePrefixAndPredecessor() {
  sysmetrics::SystemMetricHistoryModel model;
  ResourceHistorySeries series;
  series.setModel(&model);

  model.appendSample(0, 0.1, 0.2);
  model.appendSample(10'000, 0.2, 0.3);
  model.appendSample(20'000, std::nullopt, 0.4);
  model.appendSample(70'001, 0.4, 0.5);
  series.transition_animation_.setCurrentTime(300);
  model.appendSample(90'001, 0.5, 0.6);

  QCOMPARE(series.snapshot_.retained_prefix.size(), 3);
  QCOMPARE(series.snapshot_.retained_prefix.at(0).elapsed_milliseconds, 0);
  QCOMPARE(series.snapshot_.retained_prefix.at(1).elapsed_milliseconds, 10'000);
  QCOMPARE(series.snapshot_.retained_prefix.at(2).elapsed_milliseconds, 20'000);
  QCOMPARE(series.snapshot_.samples.size(), 2);
  QCOMPARE(series.snapshot_.samples.constFirst().elapsed_milliseconds, 70'001);
  QCOMPARE(series.snapshot_.samples.constLast().elapsed_milliseconds, 90'001);

  const auto cpu = geometry::buildMetricGeometry(series.snapshot_.samples, geometry::Metric::Cpu, {590.0, 100.0},
                                                 series.currentWindowEnd(), series.snapshot_.retained_prefix);
  QCOMPARE(cpu.runs.constFirst().tessellated.constFirst().x(), 0.0);
}

void ResourceHistorySeriesTest::advancingTimestampAnimatesWindow() {
  sysmetrics::SystemMetricHistoryModel model;
  ResourceHistorySeries series;
  series.setTransitionDuration(350);
  series.setModel(&model);

  model.appendSample(1'000, 0.1, 0.2);
  QCOMPARE(series.transition_animation_.state(), QAbstractAnimation::Stopped);
  QCOMPARE(series.currentWindowEnd(), 1'000.0);

  model.appendSample(2'000, 0.2, 0.3);
  QCOMPARE(series.transition_animation_.state(), QAbstractAnimation::Running);
  QCOMPARE(series.window_start_milliseconds_, 1'000.0);
  QCOMPARE(series.window_target_milliseconds_, 2'000.0);
  series.transition_animation_.setCurrentTime(175);
  QCOMPARE(series.currentWindowEnd(), 1'500.0);
}

void ResourceHistorySeriesTest::smoothstepInterpolationIsDeterministic() {
  QCOMPARE(ResourceHistorySeries::interpolateWindowEnd(1'000.0, 2'000.0, 0.0), 1'000.0);
  QCOMPARE(ResourceHistorySeries::interpolateWindowEnd(1'000.0, 2'000.0, 0.25), 1'156.25);
  QCOMPARE(ResourceHistorySeries::interpolateWindowEnd(1'000.0, 2'000.0, 0.5), 1'500.0);
  QCOMPARE(ResourceHistorySeries::interpolateWindowEnd(1'000.0, 2'000.0, 1.0), 2'000.0);
}

void ResourceHistorySeriesTest::interruptedTransitionStartsFromDisplayedPosition() {
  sysmetrics::SystemMetricHistoryModel model;
  ResourceHistorySeries series;
  series.setTransitionDuration(1'000);
  series.setModel(&model);
  model.appendSample(0, 0.1, 0.2);
  model.appendSample(1'000, 0.2, 0.3);
  series.transition_animation_.setCurrentTime(400);

  const qreal displayed = series.currentWindowEnd();
  model.appendSample(2'000, 0.3, 0.4);
  QCOMPARE(series.window_start_milliseconds_, displayed);
  QCOMPARE(series.window_target_milliseconds_, 2'000.0);
}

void ResourceHistorySeriesTest::nonAdvancingUpdateDoesNotRestartTransition() {
  sysmetrics::SystemMetricHistoryModel model;
  ResourceHistorySeries series;
  series.setTransitionDuration(1'000);
  series.setModel(&model);
  model.appendSample(0, 0.1, 0.2);
  model.appendSample(1'000, 0.2, 0.3);
  series.transition_animation_.setCurrentTime(200);
  const int elapsed_before = series.transition_animation_.currentTime();

  model.appendSample(1'000, 0.8, 0.9);
  QCOMPARE(series.transition_animation_.currentTime(), elapsed_before);
  QCOMPARE(series.window_target_milliseconds_, 1'000.0);
  QCOMPARE(series.transition_animation_.state(), QAbstractAnimation::Running);
}

void ResourceHistorySeriesTest::modelReplacementRepaintsImmediately() {
  sysmetrics::SystemMetricHistoryModel first;
  sysmetrics::SystemMetricHistoryModel second;
  first.appendSample(1'000, 0.1, 0.2);
  first.appendSample(2'000, 0.2, 0.3);
  second.appendSample(8'000, 0.4, 0.5);
  ResourceHistorySeries series;
  series.setModel(&first);
  first.appendSample(3'000, 0.3, 0.4);
  QCOMPARE(series.transition_animation_.state(), QAbstractAnimation::Running);

  series.setModel(&second);
  QCOMPARE(series.transition_animation_.state(), QAbstractAnimation::Stopped);
  QCOMPARE(series.currentWindowEnd(), 8'000.0);
  QVERIFY(series.snapshot_.retained_prefix.isEmpty());
}

void ResourceHistorySeriesTest::animationFramesReuseSceneGraphGeometry() {
  sysmetrics::SystemMetricHistoryModel model;
  ResourceHistorySeries series;
  series.setWidth(600.0);
  series.setHeight(100.0);
  series.setTransitionDuration(1'000);
  series.setModel(&model);
  model.appendSample(0, 0.2, 0.3);
  model.appendSample(1'000, 0.4, 0.5);

  QSGNode* root = series.updatePaintNode(nullptr, nullptr);
  auto* clip = dynamic_cast<QSGClipNode*>(root->firstChild());
  QVERIFY(clip != nullptr);
  QSGNode* curves = clip->firstChild();
  QSGNode* geometry = curves->firstChild();
  QSGNode* cpu_marker = clip->nextSibling();
  QSGNode* cpu_marker_geometry = cpu_marker->firstChild()->firstChild();
  QSGNode* memory_marker = cpu_marker->nextSibling();
  QSGNode* memory_marker_geometry = memory_marker->firstChild()->firstChild();
  series.transition_animation_.setCurrentTime(500);
  QSGNode* updated = series.updatePaintNode(root, nullptr);
  QCOMPARE(updated, root);
  QCOMPARE(updated->firstChild(), clip);
  QCOMPARE(updated->firstChild()->firstChild(), curves);
  QCOMPARE(updated->firstChild()->firstChild()->firstChild(), geometry);
  QCOMPARE(updated->firstChild()->nextSibling(), cpu_marker);
  QCOMPARE(updated->firstChild()->nextSibling()->firstChild()->firstChild(), cpu_marker_geometry);
  QCOMPARE(updated->firstChild()->nextSibling()->nextSibling(), memory_marker);
  QCOMPARE(updated->firstChild()->nextSibling()->nextSibling()->firstChild()->firstChild(), memory_marker_geometry);
  delete root;
}

void ResourceHistorySeriesTest::activeTransitionKeepsItsDuration() {
  sysmetrics::SystemMetricHistoryModel model;
  ResourceHistorySeries series;
  series.setTransitionDuration(1'000);
  series.setModel(&model);
  model.appendSample(0, 0.2, 0.3);
  model.appendSample(1'000, 0.4, 0.5);
  series.setTransitionDuration(50);
  QCOMPARE(series.transition_animation_.duration(), 1'000);
  series.transition_animation_.setCurrentTime(500);
  QCOMPARE(series.currentWindowEnd(), 500.0);
}

void ResourceHistorySeriesTest::endpointAssemblyIsHorizontalAndTracksNow() {
  sysmetrics::SystemMetricHistoryModel model;
  ResourceHistorySeries series;
  series.setWidth(600.0);
  series.setHeight(100.0);
  series.setTransitionDuration(1'000);
  series.setModel(&model);
  model.appendSample(0, 0.2, 0.3);
  model.appendSample(1'000, 0.8, 0.5);

  QSGNode* root = series.updatePaintNode(nullptr, nullptr);
  auto* cpu_marker = dynamic_cast<QSGTransformNode*>(root->firstChild()->nextSibling());
  QVERIFY(cpu_marker != nullptr);
  auto* opacity = dynamic_cast<QSGOpacityNode*>(cpu_marker->firstChild());
  QVERIFY(opacity != nullptr);
  auto* join = dynamic_cast<QSGGeometryNode*>(opacity->firstChild());
  QVERIFY(join != nullptr);
  const auto* join_vertices = join->geometry()->vertexDataAsColoredPoint2D();
  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic) Qt exposes vertex buffers as pointers.
  QCOMPARE(join_vertices[0].x, 0.0F);
  QCOMPARE(join_vertices[1].x, 2.0F);
  auto* ring = dynamic_cast<QSGGeometryNode*>(join->nextSibling());
  QVERIFY(ring != nullptr);
  const auto* ring_vertices = ring->geometry()->vertexDataAsColoredPoint2D();
  float minimum_ring_x = ring_vertices[0].x;
  for (int index = 1; index < ring->geometry()->vertexCount(); ++index) {
    minimum_ring_x = std::min(minimum_ring_x, ring_vertices[index].x);
  }
  // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  QCOMPARE(minimum_ring_x, 2.0F);
  QCOMPARE(cpu_marker->matrix()(0, 3), 590.0F);
  const float start_y = cpu_marker->matrix()(1, 3);
  QCOMPARE(opacity->opacity(), 1.0F);

  series.transition_animation_.setCurrentTime(500);
  series.updatePaintNode(root, nullptr);
  QVERIFY(cpu_marker->matrix()(1, 3) < start_y);
  QCOMPARE(opacity->opacity(), 1.0F);

  series.transition_animation_.setCurrentTime(1'000);
  model.appendSample(2'000, std::nullopt, 0.6);
  series.transition_animation_.setCurrentTime(1'000);
  series.updatePaintNode(root, nullptr);
  QCOMPARE(opacity->opacity(), 0.0F);
  delete root;
}
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace dashboard::ui

QTEST_MAIN(dashboard::ui::ResourceHistorySeriesTest)

#include "resource_history_series_test.moc"
