#include "countdown-font.h"

#include "app-settings.h"
#include "font-weight.h"

#include <QStringList>

#include <algorithm>
#include <iterator>

namespace {

// Bundled fonts (SIL OFL, see src/Resources/fonts/*-OFL.txt).
// Each lists both family spellings: some Windows font APIs report the
// default instance's name (e.g. "Oxanium ExtraLight"), others just "Oxanium".
struct BundledFont
{
    const char *id = nullptr;
    QStringList families;
    int minWeight = 0;
    int maxWeight = 0;
};

const BundledFont bundledFonts[] = {
    {"oxanium", {QStringLiteral("Oxanium"), QStringLiteral("Oxanium ExtraLight")}, 200, 800},
    {"jura", {QStringLiteral("Jura"), QStringLiteral("Jura Light")}, 300, 700},
    {"doto", {QStringLiteral("Doto"), QStringLiteral("Doto Black")}, 100, 900},
    {"outfit", {QStringLiteral("Outfit"), QStringLiteral("Outfit Thin")}, 100, 900},
    // Two static files, not a variable font (see familiesFor()).
    {"sansation", {QStringLiteral("Sansation")}, 300, 400},
};

const auto fontChoices = "oxanium, jura, doto, outfit, sansation";

const BundledFont *findFont(const QString &id)
{
    const auto *it
        = std::ranges::find_if(bundledFonts, [&](const BundledFont &f) { return id == QLatin1String(f.id); });
    return it != std::end(bundledFonts) ? &*it : nullptr;
}

const BundledFont &fontOrDefault(const QString &id, const char *defaultId)
{
    if (const BundledFont *font = findFont(id.trimmed().toLower())) {
        return *font;
    }
    const BundledFont *font = findFont(QString::fromLatin1(defaultId));
    Q_ASSERT(font);
    return *font;
}

QStringList familiesFor(const BundledFont &font, int weight)
{
    // Sansation Light registers under its own family name on some platforms.
    if (QLatin1String(font.id) == QLatin1String("sansation") && weight < 400) {
        return {QStringLiteral("Sansation Light"), QStringLiteral("Sansation")};
    }
    return font.families;
}

QFont makeFont(const BundledFont &chosen, int weight, int pointSize)
{
    const int clamped = std::clamp(weight, chosen.minWeight, chosen.maxWeight);

    QFont font;
    font.setFamilies(familiesFor(chosen, clamped));
    font.setPointSize(pointSize);
    font.setWeight(QFont::Weight(clamped));
    // Pin the exact weight on the variable font's axis (named instances only
    // cover multiples of 100; this also makes e.g. 350 work). Ignored by static fonts.
    font.setVariableAxis(QFont::Tag("wght"), static_cast<float>(clamped));
    return font;
}

struct FontSetting
{
    QString fontKey;
    QString weightKey;
    QString sizeKey;
    const char *defaultId;
    int defaultWeight;
    int defaultPointSize;
};

int readPointSize(const AppSettings &settings, const QString &key, int defaultPointSize)
{
    const QString text = settings.value(key, defaultPointSize).toString();
    if (const auto size = parsePointSize(text)) {
        return *size;
    }
    qWarning("tTime.ini: invalid %s '%s', using %d (a whole number of points, %d-%d)",
             key.section(QLatin1Char('/'), 1).toLatin1().constData(), qPrintable(text), defaultPointSize,
             minFontPointSize, maxFontPointSize);
    return defaultPointSize;
}

struct FontChoice
{
    const BundledFont *font;
    int weight;
    int pointSize;
};

// Reads a font id, weight and size from `settings`, warning about invalid values.
FontChoice readFontSetting(const AppSettings &settings, const FontSetting &setting)
{
    const QByteArray fontName = setting.fontKey.section(QLatin1Char('/'), 1).toLatin1();
    const QString fontId
        = settings.value(setting.fontKey, QString::fromLatin1(setting.defaultId)).toString().trimmed().toLower();
    const BundledFont *font = findFont(fontId);
    if (!font) {
        qWarning("tTime.ini: unknown %s '%s', using '%s' (choices: %s)", fontName.constData(), qPrintable(fontId),
                 setting.defaultId, fontChoices);
        font = &fontOrDefault(QString(), setting.defaultId);
    }

    const QByteArray weightName = setting.weightKey.section(QLatin1Char('/'), 1).toLatin1();
    const QString weightText = settings.value(setting.weightKey, setting.defaultWeight).toString();
    int weight = setting.defaultWeight;
    if (const auto parsed = parseFontWeight(weightText)) {
        weight = *parsed;
    } else {
        qWarning("tTime.ini: invalid %s '%s', using %d", weightName.constData(), qPrintable(weightText),
                 setting.defaultWeight);
    }
    if (weight < font->minWeight || weight > font->maxWeight) {
        qWarning("tTime.ini: %s %d is outside %s's range %d-%d, using the nearest", weightName.constData(), weight,
                 font->id, font->minWeight, font->maxWeight);
    }
    return {font, weight, readPointSize(settings, setting.sizeKey, setting.defaultPointSize)};
}

} // namespace

std::optional<int> parsePointSize(const QString &text)
{
    bool ok = false;
    const int size = text.trimmed().toInt(&ok);
    if (!ok || size < minFontPointSize || size > maxFontPointSize) {
        return std::nullopt;
    }
    return size;
}

QFont makeCountdownFont(const QString &fontId, int weight, int pointSize)
{
    QFont font = makeFont(fontOrDefault(fontId, defaultCountdownFontId), weight, pointSize);
    font.setLetterSpacing(QFont::PercentageSpacing, 125);
    return font;
}

QFont makeMessageFont(const QString &fontId, int weight, int pointSize)
{
    return makeFont(fontOrDefault(fontId, defaultMessageFontId), weight, pointSize);
}

QFont countdownFontFromSettings(const AppSettings &settings)
{
    const FontChoice choice
        = readFontSetting(settings,
                          {QStringLiteral("appearance/countdown-font"), QStringLiteral("appearance/countdown-weight"),
                           QStringLiteral("appearance/countdown-size"), defaultCountdownFontId, defaultCountdownWeight,
                           defaultCountdownPointSize});
    return makeCountdownFont(QString::fromLatin1(choice.font->id), choice.weight, choice.pointSize);
}

QFont messageFontFromSettings(const AppSettings &settings)
{
    const FontChoice choice
        = readFontSetting(settings,
                          {QStringLiteral("appearance/message-font"), QStringLiteral("appearance/message-weight"),
                           QStringLiteral("appearance/message-size"), defaultMessageFontId, defaultMessageWeight,
                           defaultMessagePointSize});
    return makeMessageFont(QString::fromLatin1(choice.font->id), choice.weight, choice.pointSize);
}

int timeUpMessagePointSizeFromSettings(const AppSettings &settings)
{
    return readPointSize(settings, QStringLiteral("appearance/time-up-message-size"), defaultTimeUpMessagePointSize);
}
