#include "resource_history_series.h"

#include "sysmetrics/system_metric_history_model.h"

#include <QtTest>

namespace dashboard::ui {

class ResourceHistorySeriesTest : public QObject {
  Q_OBJECT

 private slots:
  void frontRemovalPreservesClosestPredecessorInBothSnapshots();
};

void ResourceHistorySeriesTest::
    frontRemovalPreservesClosestPredecessorInBothSnapshots() {  // NOLINT(readability-convert-member-functions-to-static)
  sysmetrics::SystemMetricHistoryModel model;
  ResourceHistorySeries series;
  series.setTransitionDuration(200);
  series.setModel(&model);

  model.appendSample(0, 0.1, 0.2);
  model.appendSample(1'000, 0.2, 0.3);
  model.appendSample(61'001, 0.4, 0.5);

  QVERIFY(series.snapshot_.predecessor.has_value());
  QCOMPARE(series.snapshot_.predecessor->elapsed_milliseconds, 1'000);
  QVERIFY(series.previous_snapshot_.predecessor.has_value());
  QCOMPARE(series.previous_snapshot_.predecessor->elapsed_milliseconds, 1'000);
  QCOMPARE(series.snapshot_.samples.size(), 1);
  QCOMPARE(series.snapshot_.samples.constFirst().elapsed_milliseconds, 61'001);
  QVERIFY(!series.previous_snapshot_.samples.isEmpty());
}

}  // namespace dashboard::ui

QTEST_MAIN(dashboard::ui::ResourceHistorySeriesTest)

#include "resource_history_series_test.moc"
