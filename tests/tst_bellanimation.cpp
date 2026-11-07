#include "bell-animation.h"
#include "testutil.h"
#include "theme.h"

#include <QTest>

namespace {

struct Difference
{
    Difference()
        : mean(0)
        , worst(0)
    {
    }

    double mean;
    int worst;
    QPoint where; // position of the worst pixel
    QString describe() const
    {
        return s("mean %1, worst %2 at (%3,%4)").arg(mean).arg(worst).arg(where.x()).arg(where.y());
    }
};

// Mean and maximum per-channel difference between two images inside `area`.
Difference difference(const QImage &a, const QImage &b, const QRect &area)
{
    const QImage x = a.convertToFormat(QImage::Format_ARGB32);
    const QImage y = b.convertToFormat(QImage::Format_ARGB32);
    Difference result;
    double sum = 0;
    int count = 0;
    for (int py = area.top(); py <= area.bottom(); ++py) {
        for (int px = area.left(); px <= area.right(); ++px) {
            const QRgb p = x.pixel(px, py);
            const QRgb q = y.pixel(px, py);
            const int channels[] = {qAbs(qRed(p) - qRed(q)), qAbs(qGreen(p) - qGreen(q)), qAbs(qBlue(p) - qBlue(q)),
                                    qAbs(qAlpha(p) - qAlpha(q))};
            for (int i = 0; i < 4; ++i) {
                sum += channels[i];
                if (channels[i] > result.worst) {
                    result.worst = channels[i];
                    result.where = QPoint(px, py);
                }
                ++count;
            }
        }
    }
    result.mean = sum / count;
    return result;
}

Theme theme(const QString &name)
{
    Theme t;
    themeByName(name, &t);
    return t;
}

} // namespace

class TestBellAnimation : public QObject
{
    Q_OBJECT

private slots:
    void splitsTheOriginalIcon()
    {
        const QImage icon(s(":/icons/alarm"));
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
        foreach (const QString &name, themeNames()) {
            if (name != s("blue")) {
                QTest::newRow(qPrintable(name)) << name;
            }
        }
    }

    void themedIconIsTheRestingFrame()
    {
        // The static icon and the animation share the same layers, so the
        // animation starts and ends exactly on the icon people already see.
        QFETCH(QString, theme);
        const Theme t = ::theme(theme);
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
        const BellAnimation bell = themedBellAnimation(::theme(theme));
        const QImage icon = themedAppPixmap(::theme(theme)).toImage();
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
        const BellAnimation bell = themedBellAnimation(theme(s("hibiscus-red")));
        QVERIFY(bell.isValid());

        // The extremes of the swing used by the "Time's up" window.
        const QImage rest = bell.frame(0, 0);
        const QImage swungFrames[] = {bell.frame(12, 16), bell.frame(-12, -16)};
        const QRect border[] = {QRect(0, 0, 128, 10), QRect(0, 118, 128, 10), QRect(0, 0, 10, 128),
                                QRect(118, 0, 10, 128)};
        for (int i = 0; i < 2; ++i) {
            const QImage &swung = swungFrames[i];
            // The icon's border (well away from the bell) never changes.
            for (int j = 0; j < 4; ++j) {
                const Difference d = difference(rest, swung, border[j]);
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
        plain.fill(QColor(30, 90, 160).rgba());
        QVERIFY(!BellAnimation(plain).isValid());
        QVERIFY(BellAnimation(plain).frame(10, 10).isNull());
    }
};

QTEST_MAIN(TestBellAnimation)
#include "tst_bellanimation.moc"
