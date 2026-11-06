#pragma once

#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QStringView>

#include <optional>

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
    QString name;
    QColor dark; // window edges
    QColor mid; // inner band
    QColor light; // centre
    QColor highlight; // hovered / pressed / checked button background
    QColor highlightBorder; // ... and its border
};

inline constexpr const char *defaultThemeName = "blue";

// The default theme ("blue").
[[nodiscard]] Theme defaultTheme();

// The available themes, default first.
[[nodiscard]] QStringList themeNames();

// Looks up a theme by name (case-insensitive, ignoring "-", "_" and spaces). Short
// forms are accepted too: "sage-green", "prussian", "hibiscus", "jade", "forest",
// "pine", "hunter".
[[nodiscard]] std::optional<Theme> themeByName(QStringView name);

// The theme configured in `settings` (appearance/theme); unknown names log a
// warning and use the default.
[[nodiscard]] Theme themeFromSettings(const AppSettings &settings);

// `color` with the given alpha (0-255).
[[nodiscard]] QColor withAlpha(QColor color, int alpha);

// Recolors an artwork made for the blue theme: dark, semi-transparent pixels
// (tinted fills) take the theme's hue and saturation but keep their lightness and
// opacity; light pixels (white artwork) are untouched. Returns `image` unchanged
// for the blue theme.
[[nodiscard]] QImage recolorForTheme(const QImage &image, const Theme &theme);

// Recolors the app icon (drawn in the default blue): colored pixels take the
// theme's hue, with saturation and lightness scaled like the theme's mid color
// relative to the blue one; white/grey artwork is untouched. Returns `image`
// unchanged for the blue theme.
[[nodiscard]] QImage recolorIconForTheme(const QImage &image, const Theme &theme);

// The alarm icon split into background, bell and clapper, with the background
// recolored for this theme and the bell kept pure white.
[[nodiscard]] BellAnimation themedBellAnimation(const Theme &theme);

// The alarm icon in this theme (window, taskbar and tray icon). Cached per theme.
[[nodiscard]] QPixmap themedAppPixmap(const Theme &theme);
[[nodiscard]] QIcon themedAppIcon(const Theme &theme);

// How a frameless tTime window paints its themed background: a top-to-bottom
// gradient (dark edges, light centre) overlaid with a left-to-right one.
struct PanelStyle
{
    qreal cornerRadius = 0;
    // Opacity of the left/right gradient at the edges, the inset band and the light band.
    int sideEdgeAlpha = 150;
    int sideInsetAlpha = 100;
    int sideLightAlpha = 80;
    // Light band of the left/right gradient: centred (false) or towards the left
    // at 35 %, with the inset colour again at 55 % (true; the countdown window).
    bool lightBandLeft = false;
};

// Paints the panel into `rect` (border included, kept fully inside `rect`).
void paintPanel(QPainter &painter, const QRect &rect, const Theme &theme, const PanelStyle &style);

// Style sheet for the frameless windows' tool buttons in this theme.
[[nodiscard]] QString toolButtonStyleSheet(const Theme &theme);
