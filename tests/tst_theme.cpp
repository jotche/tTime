#include "testutil.h"
#include "theme.h"

#include <QTest>

namespace {

Theme theme(const char *name)
{
    Theme t;
    themeByName(s(name), &t);
    return t;
}

} // namespace

class TestTheme : public QObject
{
    Q_OBJECT

private slots:
    void blueIsTheOriginalPalette()
    {
        // The default must look exactly like tTime always did.
        Theme blue;
        QVERIFY(themeByName(s("blue"), &blue));
        QCOMPARE(blue.dark, QColor(3, 66, 124));
        QCOMPARE(blue.mid, QColor(69, 134, 186));
        QCOMPARE(blue.light, QColor(164, 212, 238));
        QCOMPARE(toolButtonStyleSheet(blue), s("QToolButton { border: 1px none; padding: 0 0 0 1; margin: 0; }"
                                               "QToolButton:hover, QToolButton:pressed, QToolButton:checked {"
                                               "  background-color: rgba(141, 178, 206, 150);"
                                               "  border: 1px solid rgba(186, 209, 225, 220);"
                                               "  padding: 0 0 0 1; margin: 0; }"));
    }

    void namesAreForgiving()
    {
        const char *const names[] = {"sage", "Sage", " sage ", "sage-green", "Sage Green", "sage_green"};
        for (int i = 0; i < 6; ++i) {
            Theme t;
            QVERIFY2(themeByName(s(names[i]), &t), names[i]);
            QCOMPARE(t.name, s("sage"));
        }
        QVERIFY(!themeByName(s("purple")));
        QVERIFY(!themeByName(QString()));
    }

    void everyListedThemeExists()
    {
        QCOMPARE(themeNames().first(), s(defaultThemeName));
        foreach (const QString &name, themeNames()) {
            QVERIFY2(themeByName(name), qPrintable(name));
        }
    }

    void themesKeepWhiteTextReadable()
    {
        // The window edges sit behind white text; keep them as dark as the
        // original blue (relative luminance well below white's 1.0).
        foreach (const QString &name, themeNames()) {
            Theme t;
            themeByName(name, &t);
            const QColor dark = t.dark;
            const double luminance = 0.2126 * dark.redF() + 0.7152 * dark.greenF() + 0.0722 * dark.blueF();
            QVERIFY2(luminance < 0.35, qPrintable(name));
        }
    }

    void recolourLeavesBlueAndWhiteAlone()
    {
        QImage image(3, 1, QImage::Format_ARGB32);
        image.setPixel(0, 0, qRgba(5, 14, 52, 89)); // translucent navy fill
        image.setPixel(1, 0, qRgba(255, 255, 255, 255)); // white artwork
        image.setPixel(2, 0, qRgba(230, 230, 240, 120)); // light, antialiased edge

        QCOMPARE(recolorForTheme(image, theme("blue")), image);

        const QImage sage = recolorForTheme(image, theme("sage"));
        const QColor fill = QColor::fromRgba(sage.pixel(0, 0));
        QCOMPARE(fill.alpha(), 89); // opacity kept
        QVERIFY(qAbs(fill.lightness() - QColor(5, 14, 52).lightness()) <= 1); // lightness kept
        QVERIFY(fill.green() > fill.blue()); // now green, not navy
        QCOMPARE(sage.pixel(1, 0), image.pixel(1, 0));
        QCOMPARE(sage.pixel(2, 0), image.pixel(2, 0));
    }

    void newThemesAndAliases()
    {
        QCOMPARE(theme("prussian-blue").name, s("prussian-blue"));
        QCOMPARE(theme("Prussian Blue").name, s("prussian-blue"));
        QCOMPARE(theme("prussian").name, s("prussian-blue"));
        QCOMPARE(theme("hibiscus-red").name, s("hibiscus-red"));
        QCOMPARE(theme("hibiscus").name, s("hibiscus-red"));
        QCOMPARE(themeNames(), QStringList() << s("blue") << s("sage") << s("prussian-blue") << s("hibiscus-red")
                                             << s("jade-green") << s("forest-green") << s("pine-green")
                                             << s("hunter-green"));
        QCOMPARE(theme("jade").name, s("jade-green"));
        QCOMPARE(theme("Forest Green").name, s("forest-green"));
        QCOMPARE(theme("pine_green").name, s("pine-green"));
        QCOMPARE(theme("HUNTER").name, s("hunter-green"));
    }

    void iconRecolor_data()
    {
        QTest::addColumn<QString>("theme");
        QTest::addColumn<int>("minHue"); // expected hue range of the recolored blue
        QTest::addColumn<int>("maxHue");
        QTest::newRow("sage") << s("sage") << 100 << 150;
        QTest::newRow("prussian-blue") << s("prussian-blue") << 190 << 215;
        QTest::newRow("hibiscus-red") << s("hibiscus-red") << 320 << 359;
        QTest::newRow("jade-green") << s("jade-green") << 145 << 170;
        QTest::newRow("forest-green") << s("forest-green") << 110 << 130;
        QTest::newRow("pine-green") << s("pine-green") << 165 << 185;
        QTest::newRow("hunter-green") << s("hunter-green") << 115 << 140;
    }

    void iconRecolor()
    {
        QFETCH(QString, theme);
        QFETCH(int, minHue);
        QFETCH(int, maxHue);

        QImage icon(3, 1, QImage::Format_ARGB32);
        icon.setPixel(0, 0, qRgba(40, 120, 200, 255)); // the icon's blue body
        icon.setPixel(1, 0, qRgba(255, 255, 255, 255)); // white bell
        icon.setPixel(2, 0, qRgba(0, 0, 0, 0)); // transparent

        QCOMPARE(recolorIconForTheme(icon, ::theme("blue")), icon);

        Theme t;
        QVERIFY(themeByName(theme, &t));
        const QImage recolored = recolorIconForTheme(icon, t);
        const QColor body = QColor::fromRgba(recolored.pixel(0, 0));
        QVERIFY2(body.hslHue() >= minHue && body.hslHue() <= maxHue, qPrintable(QString::number(body.hslHue())));
        QCOMPARE(body.alpha(), 255);
        QCOMPARE(recolored.pixel(1, 0), icon.pixel(1, 0));
        QCOMPARE(recolored.pixel(2, 0), icon.pixel(2, 0));
    }

    void prussianIconIsDarkerThanBlue()
    {
        QImage icon(1, 1, QImage::Format_ARGB32);
        icon.setPixel(0, 0, qRgba(40, 120, 200, 255));
        const QColor prussian = QColor::fromRgba(recolorIconForTheme(icon, theme("prussian-blue")).pixel(0, 0));
        QVERIFY(prussian.lightness() < QColor(40, 120, 200).lightness());
    }

    void withAlphaKeepsColour()
    {
        const QColor c = withAlpha(QColor(10, 20, 30), 99);
        QCOMPARE(c, QColor(10, 20, 30, 99));
    }
};

QTEST_MAIN(TestTheme)
#include "tst_theme.moc"
