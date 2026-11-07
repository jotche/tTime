#include "countdown-font.h"

#include "app-settings.h"
#include "font-weight.h"

#include <QDir>
#include <QFontDatabase>
#include <QStringList>

namespace {

// Bundled fonts (SIL OFL, see src/Resources/fonts/*-OFL.txt), one static file per
// weight: <family>-<style>.ttf, e.g. Oxanium-Light.ttf.
struct BundledFont
{
    const char *id;
    const char *family;
    int minWeight;
    int maxWeight;
};

const BundledFont bundledFonts[] = {
    {"oxanium", "Oxanium", 200, 800},
    {"jura", "Jura", 300, 700},
    {"doto", "Doto", 900, 900},
    {"outfit", "Outfit", 100, 900},
    {"sansation", "Sansation", 300, 400},
};

const int bundledFontCount = sizeof bundledFonts / sizeof bundledFonts[0];

const char *const fontChoices = "oxanium, jura, doto, outfit, sansation";

// Style names of the weights 100, 200, ..., 900.
const char *const weightStyles[] = {"Thin",     "ExtraLight", "Light",     "Regular", "Medium",
                                    "SemiBold", "Bold",       "ExtraBold", "Black"};

const BundledFont *findFont(const QString &id)
{
    for (int i = 0; i < bundledFontCount; ++i) {
        if (id == QLatin1String(bundledFonts[i].id)) {
            return &bundledFonts[i];
        }
    }
    return 0;
}

const BundledFont &fontOrDefault(const QString &id, const char *defaultId)
{
    if (const BundledFont *font = findFont(id.trimmed().toLower())) {
        return *font;
    }
    const BundledFont *font = findFont(QLatin1String(defaultId));
    Q_ASSERT(font);
    return *font;
}

int fontWeight(const BundledFont &font, int weight)
{
    const int rounded = (weight + 50) / 100 * 100;
    return qBound(font.minWeight, rounded, font.maxWeight);
}

QString familyFor(const BundledFont &font, int bundledWeight)
{
    // Every weight but Regular and Bold registers under its own family name
    // ("Oxanium Light"); Bold is the bold style of the plain family.
    const QString family = QLatin1String(font.family);
    if (bundledWeight == 400 || bundledWeight == 700) {
        return family;
    }
    return family + QLatin1Char(' ') + QLatin1String(weightStyles[bundledWeight / 100 - 1]);
}

QFont makeFont(const BundledFont &chosen, int weight, int pointSize)
{
    const int bundled = fontWeight(chosen, weight);
    QFont font(familyFor(chosen, bundled));
    font.setPointSize(pointSize);
    font.setWeight(bundled == 700 ? QFont::Bold : QFont::Normal);
    return font;
}

struct FontSetting
{
    const char *fontKey;
    const char *weightKey;
    const char *sizeKey;
    const char *defaultId;
    int defaultWeight;
    int defaultPointSize;
};

const FontSetting countdownSetting = {"appearance/countdown-font", "appearance/countdown-weight",
                                      "appearance/countdown-size", defaultCountdownFontId,
                                      defaultCountdownWeight,      defaultCountdownPointSize};

const FontSetting messageSetting = {"appearance/message-font", "appearance/message-weight",
                                    "appearance/message-size", defaultMessageFontId,
                                    defaultMessageWeight,      defaultMessagePointSize};

// The key without its group, for warnings ("appearance/countdown-size" -> "countdown-size").
QByteArray keyName(const char *key)
{
    return QString::fromLatin1(key).section(QLatin1Char('/'), 1).toLatin1();
}

int readPointSize(const AppSettings &settings, const char *key, int defaultPointSize)
{
    const QString text = settings.value(QLatin1String(key), defaultPointSize).toString();
    int size = 0;
    if (parsePointSize(text, &size)) {
        return size;
    }
    qWarning("tTime.ini: invalid %s '%s', using %d (a whole number of points, %d-%d)", keyName(key).constData(),
             qPrintable(text), defaultPointSize, minFontPointSize, maxFontPointSize);
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
    const QString fontId = settings.value(QLatin1String(setting.fontKey), QLatin1String(setting.defaultId))
                               .toString()
                               .trimmed()
                               .toLower();
    const BundledFont *font = findFont(fontId);
    if (!font) {
        qWarning("tTime.ini: unknown %s '%s', using '%s' (choices: %s)", keyName(setting.fontKey).constData(),
                 qPrintable(fontId), setting.defaultId, fontChoices);
        font = &fontOrDefault(QString(), setting.defaultId);
    }

    const QString weightText = settings.value(QLatin1String(setting.weightKey), setting.defaultWeight).toString();
    int weight = setting.defaultWeight;
    if (!parseFontWeight(weightText, &weight)) {
        weight = setting.defaultWeight;
        qWarning("tTime.ini: invalid %s '%s', using %d", keyName(setting.weightKey).constData(),
                 qPrintable(weightText), setting.defaultWeight);
    }
    if (weight < font->minWeight || weight > font->maxWeight) {
        qWarning("tTime.ini: %s %d is outside %s's range %d-%d, using the nearest",
                 keyName(setting.weightKey).constData(), weight, font->id, font->minWeight, font->maxWeight);
    }

    FontChoice choice;
    choice.font = font;
    choice.weight = weight;
    choice.pointSize = readPointSize(settings, setting.sizeKey, setting.defaultPointSize);
    return choice;
}

} // namespace

bool parsePointSize(const QString &text, int *size)
{
    bool ok = false;
    const int parsed = text.trimmed().toInt(&ok);
    if (!ok || parsed < minFontPointSize || parsed > maxFontPointSize) {
        return false;
    }
    if (size) {
        *size = parsed;
    }
    return true;
}

int bundledFontWeight(const QString &fontId, int weight)
{
    return fontWeight(fontOrDefault(fontId, defaultCountdownFontId), weight);
}

QString bundledFontFamily(const QString &fontId, int weight)
{
    const BundledFont &font = fontOrDefault(fontId, defaultCountdownFontId);
    return familyFor(font, fontWeight(font, weight));
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
    const FontChoice choice = readFontSetting(settings, countdownSetting);
    return makeCountdownFont(QLatin1String(choice.font->id), choice.weight, choice.pointSize);
}

QFont messageFontFromSettings(const AppSettings &settings)
{
    const FontChoice choice = readFontSetting(settings, messageSetting);
    return makeMessageFont(QLatin1String(choice.font->id), choice.weight, choice.pointSize);
}

int timeUpMessagePointSizeFromSettings(const AppSettings &settings)
{
    return readPointSize(settings, "appearance/time-up-message-size", defaultTimeUpMessagePointSize);
}

void loadBundledFonts()
{
    const QDir fonts(QLatin1String(":/fonts"));
    const QStringList files = fonts.entryList(QStringList(QLatin1String("*.ttf")), QDir::Files, QDir::Name);
    if (files.isEmpty()) {
        qWarning("No bundled fonts found");
    }
    foreach (const QString &file, files) {
        if (QFontDatabase::addApplicationFont(fonts.filePath(file)) < 0) {
            qWarning("Could not load bundled font %s", qPrintable(file));
        }
    }
}
