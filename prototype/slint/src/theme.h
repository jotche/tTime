#pragma once

#include "image.h"

#include <optional>
#include <string>
#include <vector>

struct Color
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    friend bool operator==(const Color &, const Color &) = default;
};

// Theme of the tTime windows, configurable in tTime.ini ([appearance] theme=blue):
// the colors the windows paint their gradients with, and the highlight colors of
// the toolbar buttons.
struct Theme
{
    std::string name;
    Color dark; // window edges
    Color mid; // inner band
    Color light; // centre
    Color highlight; // hovered / pressed / checked button background
    Color highlightBorder; // ... and its border
};

inline constexpr const char *defaultThemeName = "blue";

[[nodiscard]] Theme defaultTheme();
// The available themes, default first.
[[nodiscard]] std::vector<std::string> themeNames();
// Looks up a theme by name (case-insensitive, ignoring "-", "_" and spaces); short
// forms such as "jade" or "prussian" are accepted too.
[[nodiscard]] std::optional<Theme> themeByName(const std::string &name);

// Recolors an artwork made for the blue theme: dark, semi-transparent pixels take the
// theme's hue and saturation but keep their lightness and opacity. (Not premultiplied.)
[[nodiscard]] Image recolorForTheme(const Image &image, const Theme &theme);

// Recolors the app icon (drawn in blue): colored pixels take the theme's hue, with
// saturation and lightness scaled like the theme's mid color; white artwork is untouched.
[[nodiscard]] Image recolorIconForTheme(const Image &image, const Theme &theme);

// The app icon in this theme, with the bell kept pure white. (Not premultiplied.)
[[nodiscard]] Image themedAppIcon(const Image &icon, const Theme &theme);

// HSL like QColor: hue 0-359 (-1 for grey), saturation and lightness 0-255.
struct Hsl
{
    int hue = -1;
    int saturation = 0;
    int lightness = 0;
};
[[nodiscard]] Hsl toHsl(Color color);
[[nodiscard]] Color fromHsl(int hue, int saturation, int lightness);
