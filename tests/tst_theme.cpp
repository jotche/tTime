#include "theme.h"

#include <QTest>

class TestTheme : public QObject
{
    Q_OBJECT

private slots:
    void blueIsTheOriginalPalette()
    {
        // The default must look exactly like tTime always did.
        const auto blue = themeByName(u"blue");
        QVERIFY(blue);
        QCOMPARE(blue->dark, QColor(3, 66, 124));
        QCOMPARE(blue->mid, QColor(69, 134, 186));
        QCOMPARE(blue->light, QColor(164, 212, 238));
        QCOMPARE(toolButtonStyleSheet(*blue),
                 QStringLiteral("QToolButton { border: 1px none; padding: 0 0 0 1; margin: 0; }"
                                "QToolButton:hover, QToolButton:pressed, QToolButton:checked {"
                                "  background-color: rgba(141, 178, 206, 150);"
                                "  border: 1px solid rgba(186, 209, 225, 220);"
                                "  padding: 0 0 0 1; margin: 0; }"));
    }

    void namesAreForgiving()
    {
        for (const auto *name : {"sage", "Sage", " sage ", "sage-green", "Sage Green", "sage_green"}) {
            const auto theme = themeByName(QString::fromLatin1(name));
            QVERIFY2(theme, name);
            QCOMPARE(theme->name, QStringLiteral("sage"));
        }
        QVERIFY(!themeByName(u"purple"));
        QVERIFY(!themeByName(u""));
    }

    void everyListedThemeExists()
    {
        QCOMPARE(themeNames().first(), QString::fromLatin1(defaultThemeName));
        for (const QString &name : themeNames()) {
            QVERIFY2(themeByName(name), qPrintable(name));
        }
    }

    void themesKeepWhiteTextReadable()
    {
        // The window edges sit behind white text; keep them as dark as the
        // original blue (relative luminance well below white's 1.0).
        for (const QString &name : themeNames()) {
            const QColor dark = themeByName(name)->dark;
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

        QCOMPARE(recolorForTheme(image, *themeByName(u"blue")), image);

        const QImage sage = recolorForTheme(image, *themeByName(u"sage"));
        const QColor fill = QColor::fromRgba(sage.pixel(0, 0));
        QCOMPARE(fill.alpha(), 89); // opacity kept
        QVERIFY(qAbs(fill.lightness() - QColor(5, 14, 52).lightness()) <= 1); // lightness kept
        QVERIFY(fill.green() > fill.blue()); // now green, not navy
        QCOMPARE(sage.pixel(1, 0), image.pixel(1, 0));
        QCOMPARE(sage.pixel(2, 0), image.pixel(2, 0));
    }

    void newThemesAndAliases()
    {
        QCOMPARE(themeByName(u"prussian-blue")->name, QStringLiteral("prussian-blue"));
        QCOMPARE(themeByName(u"Prussian Blue")->name, QStringLiteral("prussian-blue"));
        QCOMPARE(themeByName(u"prussian")->name, QStringLiteral("prussian-blue"));
        QCOMPARE(themeByName(u"hibiscus-red")->name, QStringLiteral("hibiscus-red"));
        QCOMPARE(themeByName(u"hibiscus")->name, QStringLiteral("hibiscus-red"));
        QCOMPARE(
            themeNames(),
            (QStringList{QStringLiteral("blue"), QStringLiteral("sage"), QStringLiteral("prussian-blue"),
                         QStringLiteral("hibiscus-red"), QStringLiteral("jade-green"), QStringLiteral("forest-green"),
                         QStringLiteral("pine-green"), QStringLiteral("hunter-green")}));
        for (const auto &[alias, name] : {std::pair{u"jade", "jade-green"},
                                          {u"Forest Green", "forest-green"},
                                          {u"pine_green", "pine-green"},
                                          {u"HUNTER", "hunter-green"}}) {
            QCOMPARE(themeByName(alias)->name, QString::fromLatin1(name));
        }
    }

    void iconRecolor_data()
    {
        QTest::addColumn<QString>("theme");
        QTest::addColumn<int>("minHue"); // expected hue range of the recolored blue
        QTest::addColumn<int>("maxHue");
        QTest::newRow("sage") << "sage" << 100 << 150;
        QTest::newRow("prussian-blue") << "prussian-blue" << 190 << 215;
        QTest::newRow("hibiscus-red") << "hibiscus-red" << 320 << 359;
        QTest::newRow("jade-green") << "jade-green" << 145 << 170;
        QTest::newRow("forest-green") << "forest-green" << 110 << 130;
        QTest::newRow("pine-green") << "pine-green" << 165 << 185;
        QTest::newRow("hunter-green") << "hunter-green" << 115 << 140;
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

        QCOMPARE(recolorIconForTheme(icon, *themeByName(u"blue")), icon);

        const QImage recolored = recolorIconForTheme(icon, *themeByName(theme));
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
        const QColor prussian = QColor::fromRgba(recolorIconForTheme(icon, *themeByName(u"prussian-blue")).pixel(0, 0));
        QVERIFY(prussian.lightness() < QColor(40, 120, 200).lightness());
    }

    void withAlphaKeepsColour()
    {
        const QColor c = withAlpha(QColor(10, 20, 30), 99);
        QCOMPARE(c, QColor(10, 20, 30, 99));
    }
};

QTEST_GUILESS_MAIN(TestTheme)
#include "tst_theme.moc"
