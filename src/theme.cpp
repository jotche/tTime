#include "theme.h"

#include "app-settings.h"
#include "bell-animation.h"

#include <QIcon>
#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>

namespace {

const char *const themeKey = "appearance/theme";

// The original tTime blue.
Theme blueTheme()
{
    return Theme(QLatin1String("blue"), QColor(3, 66, 124), QColor(69, 134, 186), QColor(164, 212, 238),
                 QColor(141, 178, 206), QColor(186, 209, 225));
}

// Sage green with the same light/dark structure as the blue, so white text keeps
// the same contrast.
Theme sageTheme()
{
    return Theme(QLatin1String("sage"), QColor(44, 78, 56), QColor(112, 146, 114), QColor(196, 216, 188),
                 QColor(150, 176, 142), QColor(200, 216, 192));
}

// Prussian blue: deeper and greyer than the default blue.
Theme prussianBlueTheme()
{
    return Theme(QLatin1String("prussian-blue"), QColor(0, 40, 68), QColor(24, 78, 112), QColor(140, 170, 190),
                 QColor(110, 145, 170), QColor(170, 195, 212));
}

// Hibiscus red: a deep pink-red.
Theme hibiscusRedTheme()
{
    return Theme(QLatin1String("hibiscus-red"), QColor(112, 18, 48), QColor(182, 49, 96), QColor(238, 172, 190),
                 QColor(205, 120, 145), QColor(240, 190, 205));
}

// Jade green (#00A86B): a bright, slightly bluish green.
Theme jadeGreenTheme()
{
    return Theme(QLatin1String("jade-green"), QColor(0, 72, 46), QColor(24, 150, 104), QColor(160, 226, 200),
                 QColor(110, 190, 160), QColor(180, 230, 210));
}

// Forest green (#228B22): a classic, fairly saturated green.
Theme forestGreenTheme()
{
    return Theme(QLatin1String("forest-green"), QColor(16, 64, 16), QColor(40, 128, 40), QColor(170, 215, 165),
                 QColor(120, 175, 115), QColor(190, 222, 185));
}

// Pine green (#01796F): a deep teal green.
Theme pineGreenTheme()
{
    return Theme(QLatin1String("pine-green"), QColor(0, 58, 54), QColor(16, 118, 108), QColor(150, 208, 200),
                 QColor(105, 165, 158), QColor(175, 215, 210));
}

// Hunter green (#355E3B): a dark, muted green.
Theme hunterGreenTheme()
{
    return Theme(QLatin1String("hunter-green"), QColor(30, 56, 34), QColor(64, 104, 70), QColor(176, 200, 172),
                 QColor(130, 160, 128), QColor(192, 212, 190));
}

// All themes, default first. Names are matched ignoring case, "-", "_" and
// spaces; the short name (e.g. "jade" for "jade-green") is accepted too.
struct ThemeEntry
{
    const char *name;
    const char *shortName;
    Theme (*make)();
};

const ThemeEntry themeTable[] = {
    {"blue", "blue", blueTheme},
    {"sage", "sagegreen", sageTheme},
    {"prussian-blue", "prussian", prussianBlueTheme},
    {"hibiscus-red", "hibiscus", hibiscusRedTheme},
    {"jade-green", "jade", jadeGreenTheme},
    {"forest-green", "forest", forestGreenTheme},
    {"pine-green", "pine", pineGreenTheme},
    {"hunter-green", "hunter", hunterGreenTheme},
};

const int themeCount = sizeof themeTable / sizeof themeTable[0];

QString normalizedThemeKey(const QString &name)
{
    QString key = name.trimmed().toLower();
    key.remove(QLatin1Char('-')).remove(QLatin1Char(' ')).remove(QLatin1Char('_'));
    return key;
}

QString rgba(const QColor &color, int alpha)
{
    return QString::fromLatin1("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(alpha);
}

// Fills (or outlines) the panel shape.
void drawPanelShape(QPainter &painter, const QRectF &outline, const PanelStyle &style, const QColor &pen,
                    const QBrush &brush)
{
    painter.setPen(pen);
    painter.setBrush(brush);
    if (style.cornerRadius > 0) {
        painter.drawRoundedRect(outline, style.cornerRadius, style.cornerRadius);
    } else {
        painter.drawRect(outline);
    }
}

} // namespace

Theme defaultTheme()
{
    return themeTable[0].make(); // the table lists the default (defaultThemeName) first
}

QStringList themeNames()
{
    QStringList names;
    for (int i = 0; i < themeCount; ++i) {
        names << QString::fromLatin1(themeTable[i].name);
    }
    return names;
}

bool themeByName(const QString &name, Theme *theme)
{
    const QString key = normalizedThemeKey(name);
    for (int i = 0; i < themeCount; ++i) {
        const ThemeEntry &entry = themeTable[i];
        if (key == normalizedThemeKey(QString::fromLatin1(entry.name))
            || key == normalizedThemeKey(QString::fromLatin1(entry.shortName))) {
            if (theme) {
                *theme = entry.make();
            }
            return true;
        }
    }
    return false;
}

Theme themeFromSettings(const AppSettings &settings)
{
    const QString name = settings.value(QLatin1String(themeKey), QLatin1String(defaultThemeName)).toString();
    Theme theme;
    if (themeByName(name, &theme)) {
        return theme;
    }
    qWarning("tTime.ini: unknown theme '%s', using '%s' (choices: %s)", qPrintable(name), defaultThemeName,
             qPrintable(themeNames().join(QLatin1String(", "))));
    return defaultTheme();
}

QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

QImage recolorForTheme(const QImage &image, const Theme &theme)
{
    if (theme.name == QLatin1String("blue")) {
        return image;
    }
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const int hue = theme.dark.hslHue();
    const int saturation = theme.dark.hslSaturation();
    for (int y = 0; y < result.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            const QColor pixel = QColor::fromRgba(line[x]);
            if (pixel.alpha() == 0 || pixel.alpha() >= 200 || pixel.lightness() >= 128) {
                continue; // transparent, solid artwork or light (white) strokes
            }
            line[x] = QColor::fromHsl(hue, saturation, pixel.lightness(), pixel.alpha()).rgba();
        }
    }
    return result;
}

QImage recolorIconForTheme(const QImage &image, const Theme &theme)
{
    if (theme.name == QLatin1String("blue")) {
        return image;
    }
    // Map the blue reference (the default theme's mid color) onto the theme's
    // mid color: same hue, saturation and lightness scaled proportionally.
    const QColor reference = blueTheme().mid;
    const int hue = theme.mid.hslHue();
    const double saturationScale
        = static_cast<double>(theme.mid.hslSaturation()) / qMax(1, reference.hslSaturation());
    const double lightnessScale = static_cast<double>(theme.mid.lightness()) / qMax(1, reference.lightness());

    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < result.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            const QColor pixel = QColor::fromRgba(line[x]);
            if (pixel.alpha() == 0 || pixel.hslSaturation() < 40) {
                continue; // transparent, or white/grey artwork (the bell)
            }
            const int saturation = qBound(0, qRound(pixel.hslSaturation() * saturationScale), 255);
            const int lightness = qBound(0, qRound(pixel.lightness() * lightnessScale), 255);
            line[x] = QColor::fromHsl(hue, saturation, lightness, pixel.alpha()).rgba();
        }
    }
    return result;
}

BellAnimation themedBellAnimation(const Theme &theme)
{
    BellAnimation animation(QImage(QLatin1String(":/icons/alarm")));
    if (animation.isValid()) {
        animation.setBackground(recolorIconForTheme(animation.background(), theme));
    }
    return animation;
}

QPixmap themedAppPixmap(const Theme &theme)
{
    // QPixmapCache (not a static container): Qt clears it before the
    // application object is destroyed. An evicted entry is simply rebuilt.
    const QString key = QLatin1String("ttime/app-icon/") + theme.name;
    QPixmap pixmap;
    if (!QPixmapCache::find(key, &pixmap)) {
        const QImage original(QLatin1String(":/icons/alarm"));
        QImage themed = original;
        if (theme.name != QLatin1String("blue")) {
            // Recolor only the background and put the white bell back on top, so
            // the bell keeps clean white edges in every theme. Fall back to
            // recoloring the whole icon if it has no separable bell.
            const BellAnimation layers = themedBellAnimation(theme);
            themed = layers.isValid() ? layers.frame(0, 0) : recolorIconForTheme(original, theme);
        }
        pixmap = QPixmap::fromImage(themed);
        QPixmapCache::insert(key, pixmap);
    }
    return pixmap;
}

QIcon themedAppIcon(const Theme &theme)
{
    return QIcon(themedAppPixmap(theme));
}

void paintPanel(QPainter &painter, const QRect &rect, const Theme &theme, const PanelStyle &style)
{
    const qreal inset = 20; // width of the dark-to-inset transition, in pixels
    const qreal h = qMax(rect.height(), 1);
    const qreal w = qMax(rect.width(), 1);

    QLinearGradient topToBottom(rect.left(), rect.top(), rect.left(), rect.top() + h);
    topToBottom.setColorAt(0.0, withAlpha(theme.dark, 235));
    topToBottom.setColorAt(qMin<qreal>(inset / h, 0.49), withAlpha(theme.mid, 190));
    topToBottom.setColorAt(0.5, withAlpha(theme.light, 150));
    topToBottom.setColorAt(qMax<qreal>(1.0 - inset / h, 0.51), withAlpha(theme.mid, 190));
    topToBottom.setColorAt(1.0, withAlpha(theme.dark, 235));

    const QColor sideEdge = withAlpha(theme.dark, style.sideEdgeAlpha);
    const QColor sideInset = withAlpha(theme.mid, style.sideInsetAlpha);
    const QColor sideLight = withAlpha(theme.light, style.sideLightAlpha);
    QLinearGradient leftToRight(rect.left(), rect.top(), rect.left() + w, rect.top());
    leftToRight.setColorAt(0.0, sideEdge);
    if (style.lightBandLeft) {
        leftToRight.setColorAt(qMin<qreal>(inset / w, 0.34), sideInset);
        leftToRight.setColorAt(0.35, sideLight);
        leftToRight.setColorAt(0.55, sideInset);
    } else {
        leftToRight.setColorAt(qMin<qreal>(inset / w, 0.49), sideInset);
        leftToRight.setColorAt(0.5, sideLight);
        leftToRight.setColorAt(qMax<qreal>(1.0 - inset / w, 0.51), sideInset);
    }
    leftToRight.setColorAt(1.0, sideEdge);

    // A 1px outline drawn on rect() would be centred on the edge, so half of it
    // would fall outside the window; draw it on the pixel centres instead.
    const QRectF outline = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    drawPanelShape(painter, outline, style, QColor(0, 0, 0, 32), topToBottom);
    drawPanelShape(painter, outline, style, QColor(220, 220, 220, 100), leftToRight);
    painter.restore();
}

QString toolButtonStyleSheet(const Theme &theme)
{
    return QString::fromLatin1("QToolButton { border: 1px none; padding: 0 0 0 1; margin: 0; }"
                               "QToolButton:hover, QToolButton:pressed, QToolButton:checked {"
                               "  background-color: %1;"
                               "  border: 1px solid %2;"
                               "  padding: 0 0 0 1; margin: 0; }")
        .arg(rgba(theme.highlight, 150), rgba(theme.highlightBorder, 220));
}
