#include "window-placement.h"

#include <QTest>

class TestWindowPlacement : public QObject
{
    Q_OBJECT

private:
    const QRect screen{0, 0, 1920, 1040}; // available area (taskbar excluded)
    const QSize window{200, 95};

private slots:
    void firstSlotUsesBasePosition() { QCOMPARE(cascadePosition({250, 250}, window, screen, 0), QPoint(250, 250)); }

    void cascadesDownAndRight()
    {
        QCOMPARE(cascadePosition({250, 250}, window, screen, 1), QPoint(282, 282));
        QCOMPARE(cascadePosition({250, 250}, window, screen, 3), QPoint(346, 346));
        QCOMPARE(cascadePosition({250, 250}, window, screen, 2, 10), QPoint(270, 270));
    }

    void exactlyFittingAtTheEdgeIsAllowed()
    {
        // right edge of the window lands on the last visible column (1919)
        QCOMPARE(cascadePosition({1688, 100}, window, screen, 1).x(), 1720);
    }

    void reversesOnlyTheAxisThatWouldSpillOut()
    {
        // near the right edge: x goes left, y still goes down
        QCOMPARE(cascadePosition({1700, 100}, window, screen, 1), QPoint(1668, 132));
        // near the bottom edge: y goes up, x still goes right
        QCOMPARE(cascadePosition({100, 930}, window, screen, 1), QPoint(132, 898));
        // in the bottom-right corner: both go back
        QCOMPARE(cascadePosition({1700, 930}, window, screen, 2), QPoint(1636, 866));
    }

    void staysVisibleWhenNeitherDirectionFits()
    {
        const QRect tiny{0, 0, 260, 120};
        const QPoint p = cascadePosition({30, 10}, window, tiny, 1);
        QVERIFY(tiny.contains(QRect(p, window)));
    }

    void respectsScreenOrigin()
    {
        // second monitor to the left of the primary one
        const QRect left{-1920, 0, 1920, 1040};
        QCOMPARE(cascadePosition({-300, 100}, window, left, 1), QPoint(-268, 132)); // still fits
        QCOMPARE(cascadePosition({-230, 100}, window, left, 1), QPoint(-262, 132)); // would cross x=0
        QCOMPARE(cascadePosition({-1800, 100}, window, left, 1), QPoint(-1768, 132));
    }

    // --- fitIntoVisibleArea ------------------------------------------------

    void visibleWindowIsUnchanged()
    {
        const QRect w(QPoint(250, 250), window);
        const auto placed = fitIntoVisibleArea(w, {screen});
        QCOMPARE(placed.geometry, w);
        QCOMPARE(placed.area, screen);
    }

    void movesByShortestDistance_data()
    {
        QTest::addColumn<QPoint>("saved");
        QTest::addColumn<QPoint>("expected");
        // window 200x95 on 1920x1040: valid x 0..1720, y 0..945
        QTest::newRow("far right") << QPoint(5000, 300) << QPoint(1720, 300);
        QTest::newRow("far left") << QPoint(-5000, 300) << QPoint(0, 300);
        QTest::newRow("below (under taskbar)") << QPoint(500, 1000) << QPoint(500, 945);
        QTest::newRow("above") << QPoint(500, -40) << QPoint(500, 0);
        QTest::newRow("partly off right edge") << QPoint(1800, 300) << QPoint(1720, 300);
        QTest::newRow("bottom-right corner") << QPoint(3000, 3000) << QPoint(1720, 945);
        QTest::newRow("top-left corner") << QPoint(-300, -300) << QPoint(0, 0);
    }

    void movesByShortestDistance()
    {
        QFETCH(QPoint, saved);
        QFETCH(QPoint, expected);
        const auto placed = fitIntoVisibleArea(QRect(saved, window), {screen});
        QCOMPARE(placed.geometry.topLeft(), expected);
        QCOMPARE(placed.geometry.size(), window);
        QVERIFY(screen.contains(placed.geometry));
    }

    void shrinksWindowLargerThanScreen()
    {
        const QRect small(0, 0, 800, 600);
        const auto placed = fitIntoVisibleArea(QRect(100, 100, 1000, 300), {small});
        QCOMPARE(placed.geometry, QRect(0, 100, 800, 300));
    }

    void picksScreenWithMostOverlap()
    {
        const QRect left(0, 0, 1920, 1040);
        const QRect right(1920, 0, 1920, 1040);
        // mostly on the right screen, straddling the border
        const auto placed = fitIntoVisibleArea(QRect(1900, 100, 200, 95), {left, right});
        QCOMPARE(placed.area, right);
        QCOMPARE(placed.geometry.topLeft(), QPoint(1920, 100));
    }

    void picksNearestScreenWhenOffScreen()
    {
        const QRect primary(0, 0, 1920, 1040);
        const QRect secondary(1920, 200, 1280, 984); // smaller monitor on the right, lower
        // remembered on a third monitor (now unplugged) far to the right
        auto placed = fitIntoVisibleArea(QRect(4000, 500, 200, 95), {primary, secondary});
        QCOMPARE(placed.area, secondary);
        QCOMPARE(placed.geometry.topLeft(), QPoint(3000, 500));
        // remembered above the secondary monitor's top: nearest is still secondary
        placed = fitIntoVisibleArea(QRect(2500, 50, 200, 95), {primary, secondary});
        QCOMPARE(placed.area, secondary);
        QCOMPARE(placed.geometry.topLeft(), QPoint(2500, 200));
        // remembered far to the left: primary
        placed = fitIntoVisibleArea(QRect(-3000, 500, 200, 95), {primary, secondary});
        QCOMPARE(placed.area, primary);
        QCOMPARE(placed.geometry.topLeft(), QPoint(0, 500));
    }
};

QTEST_GUILESS_MAIN(TestWindowPlacement)
#include "tst_windowplacement.moc"
