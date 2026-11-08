#include "theme.h"

#include "bell-animation.h"
#include "settings.h"

#include <algorithm>
#include <cmath>

namespace {

struct ThemeEntry
{
    const char *shortName;
    Theme theme;
};

const ThemeEntry themeTable[] = {
    // The original tTime blue.
    {"blue", {"blue", {3, 66, 124}, {69, 134, 186}, {164, 212, 238}, {141, 178, 206}, {186, 209, 225}}},
    // Sage green with the same light/dark structure as the blue.
    {"sagegreen", {"sage", {44, 78, 56}, {112, 146, 114}, {196, 216, 188}, {150, 176, 142}, {200, 216, 192}}},
    {"prussian", {"prussian-blue", {0, 40, 68}, {24, 78, 112}, {140, 170, 190}, {110, 145, 170}, {170, 195, 212}}},
    {"hibiscus", {"hibiscus-red", {112, 18, 48}, {182, 49, 96}, {238, 172, 190}, {205, 120, 145}, {240, 190, 205}}},
    {"jade", {"jade-green", {0, 72, 46}, {24, 150, 104}, {160, 226, 200}, {110, 190, 160}, {180, 230, 210}}},
    {"forest", {"forest-green", {16, 64, 16}, {40, 128, 40}, {170, 215, 165}, {120, 175, 115}, {190, 222, 185}}},
    {"pine", {"pine-green", {0, 58, 54}, {16, 118, 108}, {150, 208, 200}, {105, 165, 158}, {175, 215, 210}}},
    {"hunter", {"hunter-green", {30, 56, 34}, {64, 104, 70}, {176, 200, 172}, {130, 160, 128}, {192, 212, 190}}},
};

std::string normalizedThemeKey(const std::string &name)
{
    std::string key = toLowerAscii(trimmed(name));
    std::erase_if(key, [](char c) { return c == '-' || c == ' ' || c == '_'; });
    return key;
}

std::uint8_t clampByte(long value)
{
    return static_cast<std::uint8_t>(std::clamp<long>(value, 0, 255));
}

} // namespace

Hsl toHsl(Color color)
{
    const double r = color.r / 255.0;
    const double g = color.g / 255.0;
    const double b = color.b / 255.0;
    const double max = std::max({r, g, b});
    const double min = std::min({r, g, b});
    const double delta = max - min;
    const double lightness = (max + min) / 2;
    Hsl hsl;
    hsl.lightness = static_cast<int>(std::lround(lightness * 65535)) >> 8;
    if (delta <= 0) {
        return hsl; // grey: no hue, no saturation
    }
    const double saturation = lightness < 0.5 ? delta / (max + min) : delta / (2 - max - min);
    double hue = 0;
    if (max == r) {
        hue = (g - b) / delta;
    } else if (max == g) {
        hue = 2 + (b - r) / delta;
    } else {
        hue = 4 + (r - g) / delta;
    }
    hue *= 60;
    if (hue < 0) {
        hue += 360;
    }
    hsl.hue = static_cast<int>(hue) % 360;
    hsl.saturation = static_cast<int>(std::lround(saturation * 65535)) >> 8;
    return hsl;
}

Color fromHsl(int hue, int saturation, int lightness)
{
    const double s = saturation / 255.0;
    const double l = lightness / 255.0;
    if (hue < 0 || s <= 0) {
        const auto v = clampByte(std::lround(l * 255));
        return {v, v, v};
    }
    const double c = (1 - std::abs(2 * l - 1)) * s;
    const double h = (hue % 360) / 60.0;
    const double x = c * (1 - std::abs(std::fmod(h, 2.0) - 1));
    double r = 0;
    double g = 0;
    double b = 0;
    switch (static_cast<int>(h)) {
    case 0: r = c; g = x; break;
    case 1: r = x; g = c; break;
    case 2: g = c; b = x; break;
    case 3: g = x; b = c; break;
    case 4: r = x; b = c; break;
    default: r = c; b = x; break;
    }
    const double m = l - c / 2;
    return {clampByte(std::lround((r + m) * 255)), clampByte(std::lround((g + m) * 255)),
            clampByte(std::lround((b + m) * 255))};
}

Theme defaultTheme()
{
    return themeTable[0].theme;
}

std::vector<std::string> themeNames()
{
    std::vector<std::string> names;
    for (const ThemeEntry &entry : themeTable) {
        names.push_back(entry.theme.name);
    }
    return names;
}

std::optional<Theme> themeByName(const std::string &name)
{
    const std::string key = normalizedThemeKey(name);
    for (const ThemeEntry &entry : themeTable) {
        if (key == normalizedThemeKey(entry.theme.name) || key == normalizedThemeKey(entry.shortName)) {
            return entry.theme;
        }
    }
    return std::nullopt;
}

Image recolorForTheme(const Image &image, const Theme &theme)
{
    if (theme.name == "blue") {
        return image;
    }
    Image result = image;
    const Hsl target = toHsl(theme.dark);
    for (Rgba &p : result.pixels) {
        const Hsl pixel = toHsl({p.r, p.g, p.b});
        if (p.a == 0 || p.a >= 200 || pixel.lightness >= 128) {
            continue; // transparent, solid artwork or light (white) strokes
        }
        const Color c = fromHsl(target.hue, target.saturation, pixel.lightness);
        p = {c.r, c.g, c.b, p.a};
    }
    return result;
}

Image recolorIconForTheme(const Image &image, const Theme &theme)
{
    if (theme.name == "blue") {
        return image;
    }
    // Map the blue reference (the default theme's mid color) onto the theme's mid
    // color: same hue, saturation and lightness scaled proportionally.
    const Hsl reference = toHsl(defaultTheme().mid);
    const Hsl target = toHsl(theme.mid);
    const double saturationScale = static_cast<double>(target.saturation) / std::max(1, reference.saturation);
    const double lightnessScale = static_cast<double>(target.lightness) / std::max(1, reference.lightness);

    Image result = image;
    for (Rgba &p : result.pixels) {
        const Hsl pixel = toHsl({p.r, p.g, p.b});
        if (p.a == 0 || pixel.saturation < 40) {
            continue; // transparent, or white/grey artwork (the bell)
        }
        const int saturation = std::clamp(static_cast<int>(std::lround(pixel.saturation * saturationScale)), 0, 255);
        const int lightness = std::clamp(static_cast<int>(std::lround(pixel.lightness * lightnessScale)), 0, 255);
        const Color c = fromHsl(target.hue, saturation, lightness);
        p = {c.r, c.g, c.b, p.a};
    }
    return result;
}

Image themedAppIcon(const Image &icon, const Theme &theme)
{
    if (theme.name == "blue") {
        return icon;
    }
    // Recolor only the background and put the white bell back on top, so the bell
    // keeps clean white edges in every theme.
    BellAnimation layers(icon);
    if (!layers.isValid()) {
        return recolorIconForTheme(icon, theme);
    }
    layers.setBackground(recolorIconForTheme(layers.background(), theme));
    return layers.frame(0, 0);
}
