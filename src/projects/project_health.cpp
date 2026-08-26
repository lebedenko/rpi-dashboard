#include "projects/project_health.h"

#include <QSet>

namespace dashboard::projects {

Health healthForRun(const QString& status, const QString& conclusion, const QDateTime& completed_at,
                    const QDateTime& now) {
  if (status.isEmpty()) {
    return Health::Unknown;
  }
  if (status != QStringLiteral("completed")) {
    return Health::Running;
  }
  if (conclusion == QStringLiteral("success")) {
    return completed_at.isValid() && completed_at.daysTo(now) >= 7 ? Health::Stale : Health::Healthy;
  }
  if (QSet<QString>{QStringLiteral("failure"), QStringLiteral("timed_out"), QStringLiteral("startup_failure"),
                    QStringLiteral("action_required")}
          .contains(conclusion)) {
    return Health::Failed;
  }
  if (QSet<QString>{QStringLiteral("neutral"), QStringLiteral("cancelled"), QStringLiteral("skipped")}.contains(
          conclusion)) {
    return Health::Attention;
  }
  if (conclusion == QStringLiteral("stale")) {
    return Health::Stale;
  }
  return Health::Unknown;
}

QString healthKey(Health health) {
  switch (health) {
    case Health::Failed:
      return QStringLiteral("failed");
    case Health::Attention:
      return QStringLiteral("attention");
    case Health::Running:
      return QStringLiteral("running");
    case Health::Stale:
      return QStringLiteral("stale");
    case Health::Healthy:
      return QStringLiteral("healthy");
    case Health::Unknown:
      return QStringLiteral("unknown");
  }
  return QStringLiteral("unknown");
}

int healthRank(Health health) { return static_cast<int>(health); }

}  // namespace dashboard::projects
