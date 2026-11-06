#include "countdown-font.h"
#include "countdown-label.h"

#include <QApplication>
#include <QFontDatabase>
#include <QFontInfo>
#include <QImage>
#include <QTest>

namespace {

// Horizontal centres of the ink "islands" in a rendered label (one per glyph
// for the countdown fonts, whose glyphs do not touch).
QList<double> glyphCentres(const QImage &image)
{
    QList<double> centres;
    int start = -1;
    for (int x = 0; x <= image.width(); ++x) {
        bool ink = false;
        for (int y = 0; x < image.width() && y < image.height() && !ink; ++y) {
            ink = qGray(image.pixel(x, y)) < 128;
        }
        if (ink && start < 0) {
            start = x;
        } else if (!ink && start >= 0) {
            centres.append((start + x - 1) / 2.0);
            start = -1;
        }
    }
    return centres;
}

int inkPixels(const QImage &image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            count += qGray(image.pixel(x, y)) < 128 ? 1 : 0;
        }
    }
    return count;
}

} // namespace

class TestCountdownLabel : public QObject
{
    Q_OBJECT

private:
    QFont m_font;

    static QImage renderImage(const QFont &font, const QString &text)
    {
        CountdownLabel label;
        label.setFont(font);
        label.setStyleSheet(QStringLiteral("background: white; color: black;"));
        label.setText(text);
        label.resize(label.sizeHint() + QSize(8, 4));
        return label.grab().toImage();
    }

    QList<double> render(const QString &text)
    {
        CountdownLabel label;
        label.setFont(m_font);
        label.setStyleSheet(QStringLiteral("background: white; color: black;"));
        label.setText(QStringLiteral("00:00:00")); // fixed size, like in the countdown window
        label.resize(label.sizeHint() + QSize(8, 4));
        label.setText(text);
        return glyphCentres(label.grab().toImage());
    }

private slots:
    void initTestCase()
    {
        for (const char *file : {"Oxanium-Variable.ttf", "Jura-Variable.ttf", "Doto-Variable.ttf",
                                 "Outfit-Variable.ttf", "Sansation-Light.ttf", "Sansation-Regular.ttf"}) {
            const QString path = QStringLiteral(TTIME_FONT_DIR "/") + QLatin1String(file);
            QVERIFY2(QFontDatabase::addApplicationFont(path) >= 0, file);
        }
        // Jura has proportional digits: the case the fixed slots exist for.
        m_font = makeCountdownFont(QStringLiteral("jura"), 600);
    }

    void glyphPositionsDoNotDependOnOtherDigits()
    {
        // Jura's '1' is much narrower than its '0'. Without fixed slots the last
        // '1' moves when the character before it changes from '0' to '1'.
        const auto a = render(QStringLiteral("00:00:01"));
        const auto b = render(QStringLiteral("00:00:11"));
        QCOMPARE(a.size(), 8);
        QCOMPARE(b.size(), 8);
        QCOMPARE(a.last(), b.last());
    }

    void colonsStayPut()
    {
        const auto a = render(QStringLiteral("11:11:11"));
        const auto b = render(QStringLiteral("00:00:00"));
        const auto c = render(QStringLiteral("09:58:47"));
        QCOMPARE(a.size(), 8);
        QCOMPARE(b.size(), 8);
        QCOMPARE(c.size(), 8);
        for (int colon : {2, 5}) {
            QCOMPARE(a[colon], b[colon]);
            QCOMPARE(c[colon], b[colon]);
        }
    }

    void configuredFontsResolveToBundledFamilies_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<QString>("family");
        QTest::newRow("oxanium") << "oxanium" << "Oxanium";
        QTest::newRow("jura") << "jura" << "Jura";
        QTest::newRow("doto") << "doto" << "Doto";
        QTest::newRow("outfit") << "outfit" << "Outfit";
        QTest::newRow("sansation") << "sansation" << "Sansation";
        QTest::newRow("unknown id -> default") << "comic" << "Oxanium";
    }

    void configuredFontsResolveToBundledFamilies()
    {
        QFETCH(QString, id);
        QFETCH(QString, family);
        const QFontInfo info(makeCountdownFont(id, 300));
        QVERIFY2(info.family().startsWith(family), qPrintable(info.family()));
    }

    void weightChangesStrokeThickness_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<int>("light");
        QTest::addColumn<int>("heavy");
        QTest::newRow("oxanium 300 vs 600") << "oxanium" << 300 << 600;
        QTest::newRow("oxanium 200 vs 800") << "oxanium" << 200 << 800;
        QTest::newRow("jura 300 vs 700") << "jura" << 300 << 700;
        QTest::newRow("doto 300 vs 900") << "doto" << 300 << 900;
        QTest::newRow("outfit 200 vs 700") << "outfit" << 200 << 700;
    }

    void weightChangesStrokeThickness()
    {
        // Proves the variable fonts really render the requested weight.
        QFETCH(QString, id);
        QFETCH(int, light);
        QFETCH(int, heavy);
        const int lightInk = inkPixels(renderImage(makeCountdownFont(id, light), QStringLiteral("00:00:00")));
        const int heavyInk = inkPixels(renderImage(makeCountdownFont(id, heavy), QStringLiteral("00:00:00")));
        QVERIFY2(
            heavyInk > lightInk * 1.3,
            qPrintable(QStringLiteral("ink %1 (w%2) vs %3 (w%4)").arg(lightInk).arg(light).arg(heavyInk).arg(heavy)));
    }

    void weightIsClampedToFontRange()
    {
        QCOMPARE(makeCountdownFont(QStringLiteral("oxanium"), 100).weight(), 200);
        QCOMPARE(makeCountdownFont(QStringLiteral("jura"), 900).weight(), 700);
        QCOMPARE(makeCountdownFont(QStringLiteral("doto"), 900).weight(), 900);
        QCOMPARE(makeCountdownFont(QStringLiteral("oxanium"), 300).weight(), 300);
        QCOMPARE(makeCountdownFont(QStringLiteral("sansation"), 700).weight(), 400);
    }

    void pointSizes()
    {
        QCOMPARE(makeCountdownFont(QStringLiteral("oxanium"), 400).pointSize(), defaultCountdownPointSize);
        QCOMPARE(makeCountdownFont(QStringLiteral("oxanium"), 400, 30).pointSize(), 30);
        QCOMPARE(makeMessageFont(QStringLiteral("outfit"), 300, 11).pointSize(), 11);
        QCOMPARE(parsePointSize(QStringLiteral(" 24 ")), 24);
        QCOMPARE(parsePointSize(QStringLiteral("6")), 6);
        QCOMPARE(parsePointSize(QStringLiteral("96")), 96);
        QVERIFY(!parsePointSize(QStringLiteral("5")).has_value());
        QVERIFY(!parsePointSize(QStringLiteral("97")).has_value());
        QVERIFY(!parsePointSize(QStringLiteral("12.5")).has_value());
        QVERIFY(!parsePointSize(QStringLiteral("big")).has_value());
    }

    void messageFontDefaultsToOutfitLight()
    {
        const QFont font = makeMessageFont(QStringLiteral("unknown"), defaultMessageWeight);
        QCOMPARE(font.weight(), 300);
        QCOMPARE(font.pointSize(), 14);
        const QFontInfo info(font);
        QVERIFY2(info.family().startsWith(QStringLiteral("Outfit")), qPrintable(info.family()));
    }

    void sizeHintIndependentOfDigits()
    {
        CountdownLabel label;
        label.setFont(m_font);
        label.setText(QStringLiteral("11:11:11"));
        const QSize narrow = label.sizeHint();
        label.setText(QStringLiteral("00:00:00"));
        QCOMPARE(label.sizeHint(), narrow);
    }
};

QTEST_MAIN(TestCountdownLabel)
#include "tst_countdownlabel.moc"
