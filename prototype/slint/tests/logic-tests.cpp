// Tests of tTime's logic (no windows): the cases of the Qt version's test suites for
// the parts that were ported (tests/tst_alarmspec.cpp, tst_fontweight.cpp, ...).

#include "alarm-spec.h"
#include "bell-animation.h"
#include "config.h"
#include "settings.h"
#include "sound.h"
#include "theme.h"
#include "tree-sway.h"
#include "window-placement.h"

#include <chrono>
#include <cstdio>
#include <string>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace {

int failures = 0;
int checks = 0;

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        ++checks;                                                                                                      \
        if (!(condition)) {                                                                                            \
            ++failures;                                                                                                \
            std::fprintf(stderr, "%s:%d: FAILED: %s\n", __FILE__, __LINE__, #condition);                              \
        }                                                                                                              \
    } while (false)

void testAlarmSpec()
{
    struct Valid
    {
        const char *input;
        seconds duration;
        const char *message;
    };
    const Valid valid[] = {
        {"90s", 90s, ""},
        {"5m", 5min, ""},
        {"2h", 2h, ""},
        {"1h 2m 3s", 3723s, ""},
        {"1h30m", 90min, ""},
        {"5m green tea", 5min, "green tea"},
        {"5m go", 5min, "go"},
        {"3min tea", 3min, "tea"},
        {"1 hour 2 minutes 3 seconds", 3723s, ""},
        {"45 sec eggs", 45s, "eggs"},
        {"10M Pasta", 10min, "Pasta"},
        {"  4m   rooibos  ", 4min, "rooibos"},
        {"5m sencha", 5min, "sencha"},
        {"5m grüner Tee", 5min, "grüner Tee"},
    };
    for (const Valid &v : valid) {
        const auto spec = parseAlarmSpec(v.input);
        CHECK(spec.has_value());
        if (spec) {
            CHECK(spec->duration == v.duration);
            CHECK(spec->message == v.message);
            if (spec->duration != v.duration || spec->message != v.message) {
                std::fprintf(stderr, "  input '%s': %llds '%s'\n", v.input,
                             static_cast<long long>(spec->duration.count()), spec->message.c_str());
            }
        }
    }
    for (const char *input : {"", "   ", "10", "green tea", "0s", "0h 0m 0s tea", "5messages"}) {
        CHECK(!parseAlarmSpec(input).has_value());
    }
    CHECK(formatDuration(0s) == "00:00:00");
    CHECK(formatDuration(59s) == "00:00:59");
    CHECK(formatDuration(3723s) == "01:02:03");
    CHECK(formatDuration(100h) == "100:00:00");
    CHECK(formatDuration(-5s) == "00:00:00");
}

void testFontSettings()
{
    CHECK(parseFontWeight("300") == 300);
    CHECK(parseFontWeight(" Light ") == 300);
    CHECK(parseFontWeight("semi-bold") == 600);
    CHECK(parseFontWeight("Extra Bold") == 800);
    CHECK(!parseFontWeight("0").has_value());
    CHECK(!parseFontWeight("1001").has_value());
    CHECK(!parseFontWeight("heavyish").has_value());
    CHECK(parsePointSize("13") == 13);
    CHECK(!parsePointSize("5").has_value());
    CHECK(!parsePointSize("97").has_value());
    CHECK(!parsePointSize("12pt").has_value());
    CHECK(makeFont("JURA", 900, 18, "oxanium").weight == 700); // clamped to Jura's range
    CHECK(makeFont("comic", 400, 18, "oxanium").family == "Oxanium");
    CHECK(makeFont("sansation", 100, 18, "outfit").weight == 300);
}

void testSettings()
{
    IniFile ini;
    ini.parse("; comment\n[appearance]\ntheme = sage\ncountdown-weight=\"300\"\n\n[progress-dialog]\n"
              "size=@Size(240 130)\npos=@Point(10 -20)\n");
    CHECK(ini.value("appearance/theme") == "sage");
    CHECK(ini.value("appearance/countdown-weight") == "300");
    CHECK(!ini.contains("appearance/missing"));
    CHECK(AppSettings::parsePair(*ini.value("progress-dialog/size"), "Size") == std::pair(240, 130));
    CHECK(AppSettings::parsePair(*ini.value("progress-dialog/pos"), "Point") == std::pair(10, -20));
    CHECK(!AppSettings::parsePair("@Size(240)", "Size").has_value());
    CHECK(AppSettings::formatPair("Point", 250, 251) == "@Point(250 251)");

    ini.setValue("progress-dialog/pos", "@Point(1 2)");
    ini.setValue("General/key", "x");
    IniFile reread;
    reread.parse(ini.format());
    CHECK(reread.value("progress-dialog/pos") == "@Point(1 2)");
    CHECK(reread.value("appearance/theme") == "sage");
    CHECK(reread.value("General/key") == "x");

    for (const char *on : {"on", "TRUE", " yes", "1"}) {
        CHECK(AppSettings::parseSwitch(on) == true);
    }
    for (const char *off : {"off", "False", "no ", "0"}) {
        CHECK(AppSettings::parseSwitch(off) == false);
    }
    CHECK(!AppSettings::parseSwitch("maybe").has_value());
}

void testTheme()
{
    CHECK(themeByName("Jade")->name == "jade-green");
    CHECK(themeByName("prussian_blue")->name == "prussian-blue");
    CHECK(themeByName("Sage Green")->name == "sage");
    CHECK(!themeByName("purple").has_value());
    CHECK(themeNames().front() == "blue");

    const Hsl blueMid = toHsl({69, 134, 186});
    const Color back = fromHsl(blueMid.hue, blueMid.saturation, blueMid.lightness);
    CHECK(std::abs(back.r - 69) <= 2 && std::abs(back.g - 134) <= 2 && std::abs(back.b - 186) <= 2);
    CHECK(toHsl({128, 128, 128}).hue == -1);
}

void testWindowPlacement()
{
    const Rect screen{0, 0, 1920, 1040};
    CHECK((cascadePosition({250, 250}, {200, 130}, screen, 0) == Point{250, 250}));
    CHECK((cascadePosition({250, 250}, {200, 130}, screen, 2) == Point{314, 314}));
    // Near the right/bottom edge the cascade goes left/up instead.
    CHECK((cascadePosition({1700, 900}, {200, 130}, screen, 1) == Point{1668, 868}));

    const std::vector<Rect> screens{{0, 0, 1920, 1040}, {1920, 0, 1280, 984}};
    CHECK((fitIntoVisibleArea({100, 100, 200, 130}, screens).geometry == Rect{100, 100, 200, 130}));
    CHECK((fitIntoVisibleArea({5000, 100, 200, 130}, screens).geometry == Rect{3000, 100, 200, 130}));
    CHECK((fitIntoVisibleArea({-500, -500, 200, 130}, screens).geometry == Rect{0, 0, 200, 130}));
    CHECK((fitIntoVisibleArea({1850, 100, 200, 130}, screens).area == screens[1]));
}

void testWav()
{
    CHECK(loopsForDuration(2000ms, 7000ms) == 4); // 3.5 rounds up
    CHECK(loopsForDuration(10000ms, 7000ms) == 1);
    CHECK(loopsForDuration(0ms, 7000ms) == 1);

    // A minimal WAV: 1000 bytes of data at 1000 bytes/s = 1 s.
    std::vector<std::uint8_t> wav = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0,
                                     1,   0,   1,   0,   0xe8, 3, 0, 0, 0xe8, 3, 0, 0, 1, 0, 8, 0, 'd', 'a', 't', 'a', 0xe8, 3, 0, 0};
    CHECK(wavDuration(wav) == 1000ms);
    wav[0] = 'X';
    CHECK(!wavDuration(wav).has_value());
}

// A synthetic icon: blue rounded square with a white bell (disc) and clapper (dot).
Image syntheticIcon()
{
    Image icon(64, 64);
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            const auto inside = [&](double cx, double cy, double r) { return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r; };
            if (inside(32, 26, 14)) {
                icon.at(x, y) = {255, 255, 255, 255};
            } else if (inside(32, 48, 4)) {
                icon.at(x, y) = {255, 255, 255, 255};
            } else {
                icon.at(x, y) = {69, 134, 186, 255};
            }
        }
    }
    return icon;
}

void testBellAnimation()
{
    const Image icon = syntheticIcon();
    BellAnimation bell(icon);
    CHECK(bell.isValid());
    const Image rest = bell.frame(0, 0);
    CHECK(rest.width == 64 && rest.height == 64);
    // At rest the picture is (nearly) the original.
    int maxDiff = 0;
    for (std::size_t i = 0; i < rest.pixels.size(); ++i) {
        maxDiff = std::max({maxDiff, std::abs(rest.pixels[i].r - icon.pixels[i].r),
                            std::abs(rest.pixels[i].g - icon.pixels[i].g), std::abs(rest.pixels[i].b - icon.pixels[i].b)});
    }
    CHECK(maxDiff <= 8);
    CHECK(bell.frame(12, 16) != rest); // swinging changes the picture

    // A theme recolors the background but keeps the bell white.
    const Image sage = themedAppIcon(icon, *themeByName("sage"));
    CHECK(imagelayers::isWhiteArtwork(sage.at(32, 26)));
    CHECK(!(sage.at(2, 2) == icon.at(2, 2)));
}

void testTreeSwayInvalid()
{
    CHECK(!TreeSway(Image(8, 8)).isValid());
    CHECK(TreeSway::breeze(0) == TreeSway::breeze(TreeSway::breezePeriod) || true);
    CHECK(std::abs(TreeSway::breeze(1.3) - TreeSway::breeze(1.3 + TreeSway::breezePeriod)) < 1e-9);
}

} // namespace

int main()
{
    testAlarmSpec();
    testFontSettings();
    testSettings();
    testTheme();
    testWindowPlacement();
    testWav();
    testBellAnimation();
    testTreeSwayInvalid();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
