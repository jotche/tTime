#pragma once

#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QStringList>

class AppSettings;
class BellAnimation;
class QPainter;
class QRect;

// Theme of the tTime windows, configurable in tTime.ini:
//
//   [appearance]
//   theme=blue        (see themeNames(): blue, sage, prussian-blue, hibiscus-red,
//                      jade-green, forest-green, pine-green, hunter-green)
//
// A theme currently consists of colors: the windows paint their gradients from
// them (each window applies its own opacities), and the toolbar buttons use the
// highlight colors. It is the place to add other look-related settings later.
struct Theme
{
    Theme() {}
    Theme(const QString &name, const QColor &dark, const QColor &mid, const QColor &light, const QColor &highlight,
          const QColor &highlightBorder)
        : name(name)
        , dark(dark)
        , mid(mid)
        , light(light)
        , highlight(highlight)
        , highlightBorder(highlightBorder)
    {
    }

    QString name;
    QColor dark; // window edges
    QColor mid; // inner band
    QColor light; // centre
    QColor highlight; // hovered / pressed / checked button background
    QColor highlightBorder; // ... and its border
};

const char *const defaultThemeName = "blue";

// The default theme ("blue").
Theme defaultTheme();

// The available themes, default first.
QStringList themeNames();

// Looks up a theme by name (case-insensitive, ignoring "-", "_" and spaces). Short
// forms are accepted too: "sage-green", "prussian", "hibiscus", "jade", "forest",
// "pine", "hunter". Returns false for an unknown name; `theme` may be 0.
bool themeByName(const QString &name, Theme *theme = 0);

// The theme configured in `settings` (appearance/theme); unknown names log a
// warning and use the default.
Theme themeFromSettings(const AppSettings &settings);

// `color` with the given alpha (0-255).
QColor withAlpha(QColor color, int alpha);

// Recolors an artwork made for the blue theme: dark, semi-transparent pixels
// (tinted fills) take the theme's hue and saturation but keep their lightness and
// opacity; light pixels (white artwork) are untouched. Returns `image` unchanged
// for the blue theme.
QImage recolorForTheme(const QImage &image, const Theme &theme);

// Recolors the app icon (drawn in the default blue): colored pixels take the
// theme's hue, with saturation and lightness scaled like the theme's mid color
// relative to the blue one; white/grey artwork is untouched. Returns `image`
// unchanged for the blue theme.
QImage recolorIconForTheme(const QImage &image, const Theme &theme);

// The alarm icon split into background, bell and clapper, with the background
// recolored for this theme and the bell kept pure white.
BellAnimation themedBellAnimation(const Theme &theme);

// The alarm icon in this theme (window, taskbar and tray icon). Cached per theme.
QPixmap themedAppPixmap(const Theme &theme);
QIcon themedAppIcon(const Theme &theme);

// How a frameless tTime window paints its themed background: a top-to-bottom
// gradient (dark edges, light centre) overlaid with a left-to-right one.
struct PanelStyle
{
    PanelStyle()
        : cornerRadius(0)
        , sideEdgeAlpha(150)
        , sideInsetAlpha(100)
        , sideLightAlpha(80)
        , lightBandLeft(false)
    {
    }

    qreal cornerRadius;
    // Opacity of the left/right gradient at the edges, the inset band and the light band.
    int sideEdgeAlpha;
    int sideInsetAlpha;
    int sideLightAlpha;
    // Light band of the left/right gradient: centred (false) or towards the left
    // at 35 %, with the inset colour again at 55 % (true; the countdown window).
    bool lightBandLeft;
};

// Paints the panel into `rect` (border included, kept fully inside `rect`).
void paintPanel(QPainter &painter, const QRect &rect, const Theme &theme, const PanelStyle &style);

// Style sheet for the frameless windows' tool buttons in this theme.
QString toolButtonStyleSheet(const Theme &theme);
