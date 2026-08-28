#pragma once

#include <QQuickImageProvider>

namespace dashboard {

class WeatherIconProvider final : public QQuickImageProvider {
 public:
  WeatherIconProvider();
  [[nodiscard]] QImage requestImage(const QString& identifier, QSize* size, const QSize& requestedSize) override;
  [[nodiscard]] static QString safeIconCode(const QString& identifier);
  [[nodiscard]] static QByteArray recolorStylesheet(const QByteArray& svg, const QString& textColor,
                                                    const QString& neutralColor, const QString& highlightColor);
};

}  // namespace dashboard
