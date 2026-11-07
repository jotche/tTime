#include "countdown-font.h"
#include "countdown-label.h"
#include "testutil.h"

#include <QApplication>
#include <QFontDatabase>
#include <QFontInfo>
#include <QImage>
#include <QPixmap>
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
        label.setStyleSheet(s("background: white; color: black;"));
        label.setText(text);
        label.resize(label.sizeHint() + QSize(8, 4));
        return QPixmap::grabWidget(&label).toImage();
    }

    QList<double> render(const QString &text)
    {
        CountdownLabel label;
        label.setFont(m_font);
        label.setStyleSheet(s("background: white; color: black;"));
        label.setText(s("00:00:00")); // fixed size, like in the countdown window
        label.resize(label.sizeHint() + QSize(8, 4));
        label.setText(text);
        return glyphCentres(QPixmap::grabWidget(&label).toImage());
    }

private slots:
    void initTestCase()
    {
        loadBundledFonts();
        // Jura has proportional digits: the case the fixed slots exist for.
        m_font = makeCountdownFont(s("jura"), 600);
    }

    void glyphPositionsDoNotDependOnOtherDigits()
    {
        // Jura's '1' is much narrower than its '0'. Without fixed slots the last
        // '1' moves when the character before it changes from '0' to '1'.
        const QList<double> a = render(s("00:00:01"));
        const QList<double> b = render(s("00:00:11"));
        QCOMPARE(a.size(), 8);
        QCOMPARE(b.size(), 8);
        QCOMPARE(a.last(), b.last());
    }

    void colonsStayPut()
    {
        const QList<double> a = render(s("11:11:11"));
        const QList<double> b = render(s("00:00:00"));
        const QList<double> c = render(s("09:58:47"));
        QCOMPARE(a.size(), 8);
        QCOMPARE(b.size(), 8);
        QCOMPARE(c.size(), 8);
        QCOMPARE(a[2], b[2]);
        QCOMPARE(c[2], b[2]);
        QCOMPARE(a[5], b[5]);
        QCOMPARE(c[5], b[5]);
    }

    void configuredFontsResolveToBundledFamilies_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<int>("weight");
        QTest::addColumn<QString>("family");
        QTest::newRow("oxanium") << s("oxanium") << 300 << s("Oxanium Light");
        QTest::newRow("oxanium bold") << s("oxanium") << 700 << s("Oxanium");
        QTest::newRow("jura") << s("jura") << 300 << s("Jura Light");
        QTest::newRow("doto") << s("doto") << 300 << s("Doto Black");
        QTest::newRow("outfit") << s("outfit") << 100 << s("Outfit Thin");
        QTest::newRow("sansation") << s("sansation") << 300 << s("Sansation Light");
        QTest::newRow("sansation regular") << s("sansation") << 400 << s("Sansation");
        QTest::newRow("unknown id -> default") << s("comic") << 400 << s("Oxanium");
    }

    void configuredFontsResolveToBundledFamilies()
    {
        QFETCH(QString, id);
        QFETCH(int, weight);
        QFETCH(QString, family);
        QCOMPARE(bundledFontFamily(id, weight), family);
        const QFontInfo info(makeCountdownFont(id, weight));
        QCOMPARE(info.family(), family); // the bundled font, not a fallback
    }

    void weightChangesStrokeThickness_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<int>("light");
        QTest::addColumn<int>("heavy");
        QTest::newRow("oxanium 300 vs 600") << s("oxanium") << 300 << 600;
        QTest::newRow("oxanium 200 vs 800") << s("oxanium") << 200 << 800;
        QTest::newRow("jura 300 vs 700") << s("jura") << 300 << 700;
        QTest::newRow("outfit 200 vs 700") << s("outfit") << 200 << 700;
    }

    void weightChangesStrokeThickness()
    {
        // Proves the bundled weights really render differently.
        QFETCH(QString, id);
        QFETCH(int, light);
        QFETCH(int, heavy);
        const int lightInk = inkPixels(renderImage(makeCountdownFont(id, light), s("00:00:00")));
        const int heavyInk = inkPixels(renderImage(makeCountdownFont(id, heavy), s("00:00:00")));
        QVERIFY2(heavyInk > lightInk * 1.3,
                 qPrintable(s("ink %1 (w%2) vs %3 (w%4)").arg(lightInk).arg(light).arg(heavyInk).arg(heavy)));
    }

    void weightIsRoundedAndClampedToFontRange()
    {
        QCOMPARE(bundledFontWeight(s("oxanium"), 100), 200);
        QCOMPARE(bundledFontWeight(s("jura"), 900), 700);
        QCOMPARE(bundledFontWeight(s("doto"), 300), 900);
        QCOMPARE(bundledFontWeight(s("oxanium"), 300), 300);
        QCOMPARE(bundledFontWeight(s("oxanium"), 349), 300);
        QCOMPARE(bundledFontWeight(s("oxanium"), 350), 400);
        QCOMPARE(bundledFontWeight(s("sansation"), 700), 400);
        QCOMPARE(makeCountdownFont(s("oxanium"), 700).weight(), int(QFont::Bold));
        QCOMPARE(makeCountdownFont(s("oxanium"), 300).weight(), int(QFont::Normal));
    }

    void pointSizes()
    {
        QCOMPARE(makeCountdownFont(s("oxanium"), 400).pointSize(), defaultCountdownPointSize);
        QCOMPARE(makeCountdownFont(s("oxanium"), 400, 30).pointSize(), 30);
        QCOMPARE(makeMessageFont(s("outfit"), 300, 11).pointSize(), 11);
        int size = 0;
        QVERIFY(parsePointSize(s(" 24 "), &size));
        QCOMPARE(size, 24);
        QVERIFY(parsePointSize(s("6"), &size));
        QCOMPARE(size, 6);
        QVERIFY(parsePointSize(s("96"), &size));
        QCOMPARE(size, 96);
        QVERIFY(!parsePointSize(s("5")));
        QVERIFY(!parsePointSize(s("97")));
        QVERIFY(!parsePointSize(s("12.5")));
        QVERIFY(!parsePointSize(s("big")));
    }

    void messageFontDefaultsToOutfitLight()
    {
        const QFont font = makeMessageFont(s("unknown"), defaultMessageWeight);
        QCOMPARE(font.family(), s("Outfit Light"));
        QCOMPARE(font.pointSize(), defaultMessagePointSize);
        QCOMPARE(QFontInfo(font).family(), s("Outfit Light"));
    }

    void sizeHintIndependentOfDigits()
    {
        CountdownLabel label;
        label.setFont(m_font);
        label.setText(s("11:11:11"));
        const QSize narrow = label.sizeHint();
        label.setText(s("00:00:00"));
        QCOMPARE(label.sizeHint(), narrow);
    }
};

QTEST_MAIN(TestCountdownLabel)
#include "tst_countdownlabel.moc"
