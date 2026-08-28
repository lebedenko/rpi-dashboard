#include "weather_icon_provider.h"

#include <QBuffer>
#include <QFile>
#include <QImageReader>
#include <QRegularExpression>

namespace dashboard {

WeatherIconProvider::WeatherIconProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

QString WeatherIconProvider::safeIconCode(const QString& identifier) {
  static const QRegularExpression valid(QStringLiteral("^(01|02|03|04|09|10|11|13|50)[dn]$"));
  return valid.match(identifier.section(QLatin1Char('?'), 0, 0)).hasMatch() ? identifier.section(QLatin1Char('?'), 0, 0)
                                                                            : QStringLiteral("03d");
}

QByteArray WeatherIconProvider::recolorStylesheet(const QByteArray& svg, const QString& textColor,
                                                  const QString& neutralColor, const QString& highlightColor) {
  const QByteArray style = QByteArrayLiteral("<style id=\"current-color-scheme\">.ColorScheme-Text { color: ") +
                           textColor.toUtf8() + QByteArrayLiteral("; }.ColorScheme-NeutralText { color: ") +
                           neutralColor.toUtf8() + QByteArrayLiteral("; }.ColorScheme-Highlight { color: ") +
                           highlightColor.toUtf8() + QByteArrayLiteral("; }</style>");
  static const QRegularExpression expression(QStringLiteral("<style id=\"current-color-scheme\">.*?</style>"),
                                             QRegularExpression::DotMatchesEverythingOption);
  const QString replaced = QString::fromUtf8(svg).replace(expression, QString::fromUtf8(style));
  return replaced.toUtf8();
}

QImage WeatherIconProvider::requestImage(const QString& identifier, QSize* size, const QSize& requestedSize) {
  const QStringList parts = identifier.split(QLatin1Char('/'));
  const QString code = safeIconCode(parts.value(0));
  const QString text = parts.value(1, QStringLiteral("8b95a5"));
  const QString neutral = parts.value(2, QStringLiteral("ff9e64"));
  const QString highlight = parts.value(3, QStringLiteral("7657ff"));
  QFile file(QStringLiteral(":/rpi-dashboard/weather-icons/%1.svg").arg(code));
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  QByteArray svg = recolorStylesheet(file.readAll(), QStringLiteral("#") + text, QStringLiteral("#") + neutral,
                                     QStringLiteral("#") + highlight);
  QBuffer buffer(&svg);
  buffer.open(QIODevice::ReadOnly);
  QImageReader reader(&buffer, "svg");
  const QSize target = requestedSize.isValid() ? requestedSize : QSize(64, 64);
  reader.setScaledSize(target);
  QImage image = reader.read();
  if (size != nullptr) {
    *size = image.size();
  }
  return image;
}

}  // namespace dashboard
