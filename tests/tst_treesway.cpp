#include "theme.h"
#include "tree-sway.h"

#include <QGuiApplication>
#include <QTest>

#include <cmath>
#include <numbers>

namespace {

QImage treePicture(const QString &theme)
{
    return recolorForTheme(QImage(QStringLiteral(":/images/tree02")), *themeByName(theme));
}

// Largest per-channel difference between two images inside `area`.
int worstDifference(const QImage &a, const QImage &b, const QRect &area)
{
    const QImage x = a.convertToFormat(QImage::Format_ARGB32);
    const QImage y = b.convertToFormat(QImage::Format_ARGB32);
    int worst = 0;
    for (int py = area.top(); py <= area.bottom(); ++py) {
        for (int px = area.left(); px <= area.right(); ++px) {
            const QRgb p = x.pixel(px, py);
            const QRgb q = y.pixel(px, py);
            worst = std::max({worst, std::abs(qRed(p) - qRed(q)), std::abs(qGreen(p) - qGreen(q)),
                              std::abs(qBlue(p) - qBlue(q)), std::abs(qAlpha(p) - qAlpha(q))});
        }
    }
    return worst;
}

double meanDifference(const QImage &a, const QImage &b)
{
    const QImage x = a.convertToFormat(QImage::Format_ARGB32);
    const QImage y = b.convertToFormat(QImage::Format_ARGB32);
    double sum = 0;
    for (int py = 0; py < x.height(); ++py) {
        for (int px = 0; px < x.width(); ++px) {
            const QRgb p = x.pixel(px, py);
            const QRgb q = y.pixel(px, py);
            sum += std::abs(qRed(p) - qRed(q)) + std::abs(qGreen(p) - qGreen(q)) + std::abs(qBlue(p) - qBlue(q))
                + std::abs(qAlpha(p) - qAlpha(q));
        }
    }
    return sum / (4.0 * x.width() * x.height());
}

// Horizontal centre of the white tree pixels in rows [top, bottom), inside the frame.
double inkCentre(const QImage &image, int top, int bottom)
{
    const QImage img = image.convertToFormat(QImage::Format_ARGB32);
    double sum = 0;
    double weight = 0;
    for (int y = top; y < bottom; ++y) {
        for (int x = 12; x < img.width() - 12; ++x) {
            const QRgb c = img.pixel(x, y);
            const double white = std::min({qRed(c), qGreen(c), qBlue(c)}) / 255.0;
            if (white > 0.8) {
                sum += x * white;
                weight += white;
            }
        }
    }
    return weight > 0 ? sum / weight : 0;
}

} // namespace

class TestTreeSway : public QObject
{
    Q_OBJECT

private slots:
    void splitsThePicture_data()
    {
        QTest::addColumn<QString>("theme");
        for (const QString &name : themeNames()) {
            QTest::newRow(qPrintable(name)) << name;
        }
    }

    void splitsThePicture()
    {
        QFETCH(QString, theme);
        const QImage picture = treePicture(theme);
        QCOMPARE(picture.size(), QSize(157, 173));
        const TreeSway sway(picture);
        QVERIFY(sway.isValid());
        // The ground starts below the grass curls, the crown near the top.
        QVERIFY2(sway.groundLine() >= 112 && sway.groundLine() <= 125, qPrintable(QString::number(sway.groundLine())));
        QVERIFY2(sway.crownTop() >= 15 && sway.crownTop() <= 35, qPrintable(QString::number(sway.crownTop())));

        // Without wind the picture is reproduced exactly.
        QCOMPARE(meanDifference(sway.frame(0), picture), 0.0);
        QCOMPARE(meanDifference(sway.frame(0, 2.0, 0), picture), 0.0);
    }

    void frameAndGroundStayStill()
    {
        const TreeSway sway(treePicture(QStringLiteral("blue")));
        QVERIFY(sway.isValid());
        const QImage rest = sway.frame(0).convertToFormat(QImage::Format_ARGB32);
        // The strongest frames the window produces: full sway either way, flutter at any phase.
        for (const double sway_ : {-TreeSway::breezeSway, TreeSway::breezeSway}) {
            for (const double phase : {0.0, 1.7, 3.9}) {
                const QImage moved
                    = sway.frame(sway_, phase, TreeSway::leafFlutter).convertToFormat(QImage::Format_ARGB32);
                for (int y = 0; y < rest.height(); ++y) {
                    for (int x = 0; x < rest.width(); ++x) {
                        const bool still = y >= sway.groundLine() || !sway.insideFrame(x, y) || y < sway.crownTop() - 3;
                        if (still && rest.pixel(x, y) != moved.pixel(x, y)) {
                            QFAIL(qPrintable(QStringLiteral("pixel (%1,%2) moved").arg(x).arg(y)));
                        }
                    }
                }
            }
        }
    }

    void crownMovesBaseStaysPlanted()
    {
        const TreeSway sway(treePicture(QStringLiteral("blue")));
        const QImage left = sway.frame(-TreeSway::breezeSway);
        const QImage right = sway.frame(TreeSway::breezeSway);
        const int crown = sway.crownTop();
        const int ground = sway.groundLine();
        // Upper crown moves almost the full swing between the extremes ...
        const double crownShift = inkCentre(right, crown, crown + 20) - inkCentre(left, crown, crown + 20);
        const double fullSwing = 2 * TreeSway::breezeSway; // from one extreme to the other
        QVERIFY2(crownShift > 0.7 * fullSwing && crownShift < 1.1 * fullSwing, qPrintable(QString::number(crownShift)));
        // ... while the trunk just above the ground barely moves.
        const double baseShift = inkCentre(right, ground - 12, ground) - inkCentre(left, ground - 12, ground);
        QVERIFY2(std::abs(baseShift) < 0.6, qPrintable(QString::number(baseShift)));
    }

    void flutterMovesOnlyTheCrown()
    {
        const TreeSway sway(treePicture(QStringLiteral("blue")));
        const QImage calm = sway.frame(0, 0, 0);
        const QImage shimmer = sway.frame(0, 1.3, TreeSway::leafFlutter);
        const int height = sway.groundLine() - sway.crownTop();
        // Lower part of the tree (trunk, below 40 % of its height): no flutter at all.
        const int trunkHeight = height * 2 / 5;
        const QRect trunk(0, sway.groundLine() - trunkHeight, 157, trunkHeight);
        QCOMPARE(worstDifference(calm, shimmer, trunk), 0);
        // The crown does shimmer.
        const QRect crown(0, sway.crownTop(), 157, height / 3);
        QVERIFY(worstDifference(calm, shimmer, crown) > 20);
    }

    void breezeIsGentleAndLoops()
    {
        double low = 0;
        double high = 0;
        for (int i = 0; i < 600; ++i) {
            const double t = i * 0.01;
            const double b = TreeSway::breeze(t);
            low = std::min(low, b);
            high = std::max(high, b);
            QVERIFY(std::abs(TreeSway::breeze(t + TreeSway::breezePeriod) - b) < 1e-9); // seamless loop
        }
        QVERIFY(low >= -1.0 && high <= 1.0);
        QVERIFY(low < -0.5 && high > 0.5); // it does sway both ways
    }

    void rejectsAPictureWithoutATree()
    {
        QImage plain(64, 64, QImage::Format_ARGB32);
        plain.fill(QColor(30, 90, 160));
        QVERIFY(!TreeSway(plain).isValid());
        QVERIFY(TreeSway(plain).frame(2).isNull());
    }
};

QTEST_MAIN(TestTreeSway)
#include "tst_treesway.moc"
