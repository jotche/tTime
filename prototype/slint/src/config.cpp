#include "config.h"

#include "settings.h"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <utility>

namespace {

// Bundled fonts (SIL OFL, see src/Resources/fonts/*-OFL.txt), registered by ui/theme.slint.
struct BundledFont
{
    const char *id;
    const char *family;
    int minWeight;
    int maxWeight;
};

constexpr BundledFont bundledFonts[] = {
    {"oxanium", "Oxanium", 200, 800}, {"jura", "Jura", 300, 700},          {"doto", "Doto", 100, 900},
    {"outfit", "Outfit", 100, 900},   {"sansation", "Sansation", 300, 400},
};

constexpr const char *fontChoices = "oxanium, jura, doto, outfit, sansation";

const BundledFont *findFont(const std::string &id)
{
    const auto *it = std::ranges::find_if(bundledFonts, [&](const BundledFont &f) { return id == f.id; });
    return it != std::end(bundledFonts) ? it : nullptr;
}

std::optional<int> parseInt(const std::string &text)
{
    const std::string t = trimmed(text);
    int value = 0;
    const auto [end, error] = std::from_chars(t.data(), t.data() + t.size(), value);
    if (error != std::errc() || end != t.data() + t.size() || t.empty()) {
        return std::nullopt;
    }
    return value;
}

std::string shortKey(const std::string &key)
{
    return key.substr(key.find('/') + 1);
}

int readPointSize(const AppSettings &settings, const std::string &key, int defaultSize)
{
    const std::string text = settings.value(key, std::to_string(defaultSize));
    if (const auto size = parsePointSize(text)) {
        return *size;
    }
    std::fprintf(stderr, "tTime.ini: invalid %s '%s', using %d (a whole number of points, %d-%d)\n",
                 shortKey(key).c_str(), text.c_str(), defaultSize, minFontPointSize, maxFontPointSize);
    return defaultSize;
}

FontChoice readFont(const AppSettings &settings, const std::string &fontKey, const std::string &weightKey,
                    const std::string &sizeKey, const char *defaultId, int defaultWeight, int defaultSize)
{
    const std::string id = toLowerAscii(trimmed(settings.value(fontKey, defaultId)));
    if (!findFont(id)) {
        std::fprintf(stderr, "tTime.ini: unknown %s '%s', using '%s' (choices: %s)\n", shortKey(fontKey).c_str(),
                     id.c_str(), defaultId, fontChoices);
    }
    const std::string weightText = settings.value(weightKey, std::to_string(defaultWeight));
    int weight = defaultWeight;
    if (const auto parsed = parseFontWeight(weightText)) {
        weight = *parsed;
    } else {
        std::fprintf(stderr, "tTime.ini: invalid %s '%s', using %d\n", shortKey(weightKey).c_str(), weightText.c_str(),
                     defaultWeight);
    }
    const BundledFont *font = findFont(id) ? findFont(id) : findFont(defaultId);
    if (weight < font->minWeight || weight > font->maxWeight) {
        std::fprintf(stderr, "tTime.ini: %s %d is outside %s's range %d-%d, using the nearest\n",
                     shortKey(weightKey).c_str(), weight, font->id, font->minWeight, font->maxWeight);
    }
    return makeFont(id, weight, readPointSize(settings, sizeKey, defaultSize), defaultId);
}

} // namespace

std::optional<int> parseFontWeight(const std::string &text)
{
    if (const auto number = parseInt(text)) {
        return *number >= 1 && *number <= 1000 ? number : std::nullopt;
    }
    std::string key = toLowerAscii(trimmed(text));
    std::erase_if(key, [](char c) { return c == '-' || c == ' ' || c == '_'; });
    static constexpr std::pair<const char *, int> names[] = {
        {"thin", 100},      {"hairline", 100},  {"extralight", 200}, {"ultralight", 200},
        {"light", 300},     {"regular", 400},   {"normal", 400},     {"book", 400},
        {"medium", 500},    {"semibold", 600},  {"demibold", 600},   {"bold", 700},
        {"extrabold", 800}, {"ultrabold", 800}, {"black", 900},      {"heavy", 900},
    };
    for (const auto &[name, weight] : names) {
        if (key == name) {
            return weight;
        }
    }
    return std::nullopt;
}

std::optional<int> parsePointSize(const std::string &text)
{
    const auto size = parseInt(text);
    if (!size || *size < minFontPointSize || *size > maxFontPointSize) {
        return std::nullopt;
    }
    return size;
}

FontChoice makeFont(const std::string &id, int weight, int pointSize, const char *defaultId)
{
    const BundledFont *font = findFont(toLowerAscii(trimmed(id)));
    if (!font) {
        font = findFont(defaultId);
    }
    return {font->id, font->family, std::clamp(weight, font->minWeight, font->maxWeight), pointSize};
}

float pointsToPixels(int pointSize)
{
#ifdef __APPLE__
    return static_cast<float>(pointSize);
#else
    return static_cast<float>(pointSize) * 96.0F / 72.0F;
#endif
}

AppConfig AppConfig::fromSettings(const AppSettings &settings)
{
    AppConfig config;
    const std::string themeName = settings.value("appearance/theme", defaultThemeName);
    if (const auto theme = themeByName(themeName)) {
        config.theme = *theme;
    } else {
        std::string names;
        for (const std::string &name : themeNames()) {
            names += (names.empty() ? "" : ", ") + name;
        }
        std::fprintf(stderr, "tTime.ini: unknown theme '%s', using '%s' (choices: %s)\n", themeName.c_str(),
                     defaultThemeName, names.c_str());
    }
    config.countdownFont = readFont(settings, "appearance/countdown-font", "appearance/countdown-weight",
                                    "appearance/countdown-size", defaultCountdownFontId, defaultCountdownWeight,
                                    defaultCountdownPointSize);
    config.messageFont = readFont(settings, "appearance/message-font", "appearance/message-weight",
                                  "appearance/message-size", defaultMessageFontId, defaultMessageWeight,
                                  defaultMessagePointSize);
    config.timeUpMessagePointSize
        = readPointSize(settings, "appearance/time-up-message-size", defaultTimeUpMessagePointSize);

    const std::string animations = settings.value("appearance/animations", "on");
    if (const auto on = AppSettings::parseSwitch(animations)) {
        config.animations = *on;
    } else {
        std::fprintf(stderr, "tTime.ini: invalid animations '%s', using 'on' (choices: on, off)\n", animations.c_str());
    }
    return config;
}

AppConfig AppConfig::defaults()
{
    AppConfig config;
    config.countdownFont
        = makeFont(defaultCountdownFontId, defaultCountdownWeight, defaultCountdownPointSize, defaultCountdownFontId);
    config.messageFont = makeFont(defaultMessageFontId, defaultMessageWeight, defaultMessagePointSize, defaultMessageFontId);
    return config;
}
