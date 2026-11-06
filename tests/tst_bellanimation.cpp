#include "bell-animation.h"
#include "theme.h"

#include <QGuiApplication>
#include <QTest>

#include <cmath>

namespace {

struct Difference
{
    double mean = 0;
    int worst = 0;
    QPoint where; // position of the worst pixel
    QString describe() const
    {
        return QStringLiteral("mean %1, worst %2 at (%3,%4)").arg(mean).arg(worst).arg(where.x()).arg(where.y());
    }
};

// Mean and maximum per-channel difference between two images inside `area`.
Difference difference(const QImage &a, const QImage &b, const QRect &area)
{
    const QImage x = a.convertToFormat(QImage::Format_ARGB32);
    const QImage y = b.convertToFormat(QImage::Format_ARGB32);
    double sum = 0;
    int worst = 0;
    QPoint where;
    int count = 0;
    for (int py = area.top(); py <= area.bottom(); ++py) {
        for (int px = area.left(); px <= area.right(); ++px) {
            const QRgb p = x.pixel(px, py);
            const QRgb q = y.pixel(px, py);
            for (const int d : {qAbs(qRed(p) - qRed(q)), qAbs(qGreen(p) - qGreen(q)), qAbs(qBlue(p) - qBlue(q)),
                                qAbs(qAlpha(p) - qAlpha(q))}) {
                sum += d;
                if (d > worst) {
                    worst = d;
                    where = QPoint(px, py);
                }
                ++count;
            }
        }
    }
    return {sum / count, worst, where};
}

} // namespace

class TestBellAnimation : public QObject
{
    Q_OBJECT

private slots:
    void splitsTheOriginalIcon()
    {
        const QImage icon(QStringLiteral(":/icons/alarm"));
        QCOMPARE(icon.size(), QSize(128, 128));
        const BellAnimation bell(icon);
        QVERIFY(bell.isValid());

        // The bell hangs from the knob on top of it, near the horizontal centre.
        QVERIFY2(qAbs(bell.pivot().x() - 63.5) < 4, qPrintable(QString::number(bell.pivot().x())));
        QVERIFY2(qAbs(bell.pivot().y() - 20) < 3, qPrintable(QString::number(bell.pivot().y())));

        // Unrotated, the layers put back together look like the original icon.
        const Difference d = difference(bell.frame(0, 0), icon, icon.rect());
        QVERIFY2(d.mean < 1.0 && d.worst < 48, qPrintable(d.describe()));
    }

    void themedIconIsTheRestingFrame_data()
    {
        QTest::addColumn<QString>("theme");
        for (const QString &name : themeNames()) {
            if (name != QLatin1String("blue")) {
                QTest::newRow(qPrintable(name)) << name;
            }
        }
    }

    void themedIconIsTheRestingFrame()
    {
        // The static icon and the animation share the same layers, so the
        // animation starts and ends exactly on the icon people already see.
        QFETCH(QString, theme);
        const Theme t = *themeByName(theme);
        const BellAnimation bell = themedBellAnimation(t);
        QVERIFY(bell.isValid());
        const QImage icon = themedAppPixmap(t).toImage();
        const Difference d = difference(bell.frame(0, 0), icon, icon.rect());
        QVERIFY2(d.worst == 0, qPrintable(d.describe()));
    }

    void themedBellStaysWhite_data() { themedIconIsTheRestingFrame_data(); }

    void themedBellStaysWhite()
    {
        // Only the background is recolored: the bell's pixels (fully opaque in the
        // bell layer) are pure white in the themed icon.
        QFETCH(QString, theme);
        const BellAnimation bell = themedBellAnimation(*themeByName(theme));
        const QImage icon = themedAppPixmap(*themeByName(theme)).toImage();
        const QImage layer = bell.bell().convertToFormat(QImage::Format_ARGB32);
        int checked = 0;
        for (int y = 0; y < layer.height(); ++y) {
            for (int x = 0; x < layer.width(); ++x) {
                if (qAlpha(layer.pixel(x, y)) == 255) {
                    QCOMPARE(QColor(icon.pixel(x, y)), QColor(Qt::white));
                    ++checked;
                }
            }
        }
        QVERIFY(checked > 1000);
    }

    void backgroundStaysStillWhileTheBellMoves()
    {
        const BellAnimation bell = themedBellAnimation(*themeByName(u"hibiscus-red"));
        QVERIFY(bell.isValid());

        // The extremes of the swing used by the "Time's up" window.
        const QImage rest = bell.frame(0, 0);
        for (const QImage &swung : {bell.frame(12, 16), bell.frame(-12, -16)}) {
            // The icon's border (well away from the bell) never changes.
            for (const QRect &area :
                 {QRect(0, 0, 128, 10), QRect(0, 118, 128, 10), QRect(0, 0, 10, 128), QRect(118, 0, 10, 128)}) {
                const Difference d = difference(rest, swung, area);
                QVERIFY2(d.worst == 0, qPrintable(d.describe()));
            }
            // ... while the bell itself has visibly moved.
            const Difference moved = difference(rest, swung, QRect(20, 15, 88, 95));
            QVERIFY2(moved.mean > 5 && moved.worst > 100, qPrintable(moved.describe()));
        }
    }

    void rejectsAnIconWithoutABell()
    {
        QImage plain(64, 64, QImage::Format_ARGB32);
        plain.fill(QColor(30, 90, 160));
        QVERIFY(!BellAnimation(plain).isValid());
        QVERIFY(BellAnimation(plain).frame(10, 10).isNull());
    }
};

QTEST_MAIN(TestBellAnimation)
#include "tst_bellanimation.moc"
