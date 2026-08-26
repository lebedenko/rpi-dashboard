#pragma once

#include <QDateTime>
#include <QString>

#include <cstdint>

namespace dashboard::projects {

enum class Health : std::uint8_t { Failed, Attention, Running, Stale, Unknown, Healthy };

[[nodiscard]] Health healthForRun(const QString& status, const QString& conclusion, const QDateTime& completed_at,
                                  const QDateTime& now = QDateTime::currentDateTimeUtc());
[[nodiscard]] QString healthKey(Health health);
[[nodiscard]] int healthRank(Health health);

}  // namespace dashboard::projects
