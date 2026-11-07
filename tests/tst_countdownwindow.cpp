#include "alarm.h"
#include "app-config.h"
#include "app-settings.h"
#include "countdown-font.h"
#include "countdown-window.h"
#include "testutil.h"

#include <QApplication>
#include <QFontMetrics>
#include <QMap>
#include <QTest>
#include <QToolButton>

#include <cstdlib>

namespace {

typedef QMap<QString, QRect> Snapshot;

// Geometry of everything in the countdown window, in window coordinates.
Snapshot layoutSnapshot(QWidget *window)
{
    Snapshot snapshot;
    foreach (QWidget *child, window->findChildren<QWidget *>()) {
        if (!child->objectName().isEmpty() && child->isVisible()) {
            snapshot.insert(child->objectName(), QRect(child->mapTo(window, QPoint(0, 0)), child->size()));
        }
    }
    return snapshot;
}

QString describeChanges(const Snapshot &before, const Snapshot &after)
{
    QStringList changes;
    for (Snapshot::const_iterator it = before.constBegin(); it != before.constEnd(); ++it) {
        const QRect now = after.value(it.key());
        if (now != it.value()) {
            changes << s("%1: (%2,%3 %4x%5) -> (%6,%7 %8x%9)")
                           .arg(it.key())
                           .arg(it->x())
                           .arg(it->y())
                           .arg(it->width())
                           .arg(it->height())
                           .arg(now.x())
                           .arg(now.y())
                           .arg(now.width())
                           .arg(now.height());
        }
    }
    return changes.join(s("; "));
}

QRect geometryIn(QWidget &window, const char *name)
{
    QWidget *child = window.findChild<QWidget *>(QLatin1String(name));
    return child ? QRect(child->mapTo(&window, QPoint(0, 0)), child->size()) : QRect();
}

const qint64 eightyMinutes = 80 * 60;

} // namespace

class TestCountdownWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { loadBundledFonts(); }

    void layoutDoesNotJumpWhenStartingAfterStop()
    {
        // Built-in configuration and throw-away settings: the test does not
        // depend on (or change) the user's tTime.ini.
        TemporaryDir dir;
        AppSettings settings(dir.filePath(s("user.ini")), dir.filePath(s("defaults.ini")));
        const AppConfig config = AppConfig::defaults();

        Alarm alarm(eightyMinutes, s("green tea"));
        CountdownWindow window(&alarm, config, settings);
        window.show();
        QTest::qWaitForWindowShown(&window);
        alarm.start();
        QTest::qWait(50);

        QToolButton *playPause = window.findChild<QToolButton *>(s("playPauseButton"));
        QToolButton *stop = window.findChild<QToolButton *>(s("stopButton"));
        QVERIFY(playPause && stop);

        const Snapshot initial = layoutSnapshot(&window);

        // The user's sequence: press stop, move to start, press start, pointer stays on it.
        QTest::mouseMove(stop);
        QTest::mouseClick(stop, Qt::LeftButton);
        QTest::qWait(50);
        QCOMPARE(alarm.state(), Alarm::NotRunning);
        QVERIFY2(layoutSnapshot(&window) == initial,
                 qPrintable(s("after stop: ") + describeChanges(initial, layoutSnapshot(&window))));

        QTest::mouseMove(playPause);
        QTest::qWait(50);
        QVERIFY2(layoutSnapshot(&window) == initial,
                 qPrintable(s("hovering start: ") + describeChanges(initial, layoutSnapshot(&window))));

        QTest::mouseClick(playPause, Qt::LeftButton);
        QTest::qWait(50);
        QCOMPARE(alarm.state(), Alarm::Running);
        QVERIFY2(layoutSnapshot(&window) == initial,
                 qPrintable(s("after start: ") + describeChanges(initial, layoutSnapshot(&window))));

        QTest::mouseMove(&window, QPoint(window.width() / 2, window.height() - 5));
        QTest::qWait(50);
        QVERIFY2(layoutSnapshot(&window) == initial,
                 qPrintable(s("pointer away: ") + describeChanges(initial, layoutSnapshot(&window))));
    }

    void layoutFollowsTheSpacingRules()
    {
        TemporaryDir dir;
        AppSettings settings(dir.filePath(s("user.ini")), dir.filePath(s("defaults.ini")));
        const AppConfig config = AppConfig::defaults();

        Alarm alarm(eightyMinutes, s("green tea"));
        CountdownWindow window(&alarm, config, settings);
        window.show();
        QTest::qWaitForWindowShown(&window);

        const QRect play = geometryIn(window, "playPauseButton");
        const QRect time = geometryIn(window, "timeLabel");
        const QRect message = geometryIn(window, "descriptionLabel");
        const QRect windowActions = geometryIn(window, "windowActions");

        // Window padding 12px top and bottom, 8px left and right; 8px between the
        // action row and the content; the content is indented 4px and has 4px
        // between the countdown and the message.
        QCOMPARE(play.topLeft(), QPoint(8, 12));
        QCOMPARE(time.left(), 8 + 4);
        QCOMPARE(time.top(), play.bottom() + 1 + 8);
        QCOMPARE(message.left(), 8 + 4);
        QCOMPARE(message.top(), time.bottom() + 1 + 4);

        // The countdown label ends at the digits' baseline: no descender space below them.
        // Exactly the descender is taken off; the text height itself may round 1px
        // differently per platform.
        const QFontMetrics metrics(config.countdownFont);
        QWidget *timeLabel = window.findChild<QWidget *>(s("timeLabel"));
        QVERIFY(timeLabel);
        QCOMPARE(timeLabel->contentsMargins().bottom(), -metrics.descent());
        const int baselineHeight = metrics.height() - metrics.descent();
        QVERIFY2(std::abs(time.height() - baselineHeight) <= 1,
                 qPrintable(s("height %1, expected %2").arg(time.height()).arg(baselineHeight)));
        QCOMPARE(message.right(), window.width() - 8 - 1);

        // Window actions: 8px from the top-right corner, also after a resize.
        QCOMPARE(windowActions.topRight(), QPoint(window.width() - 8 - 1, 8));
        window.resize(window.width() + 60, window.height() + 20);
        QCOMPARE(geometryIn(window, "windowActions").topRight(), QPoint(window.width() - 8 - 1, 8));

        // At its narrowest, the window actions still keep clear of the action row.
        window.resize(1, 1);
        QTest::qWait(50);
        QCOMPARE(window.width(), 200); // minimum width
        QVERIFY2(geometryIn(window, "restartButton").right() + 8 < geometryIn(window, "windowActions").left(),
                 qPrintable(s("restart ends at %1, window actions start at %2")
                                .arg(geometryIn(window, "restartButton").right())
                                .arg(geometryIn(window, "windowActions").left())));
    }

    void heightFollowsTheContent()
    {
        TemporaryDir dir;
        AppSettings settings(dir.filePath(s("user.ini")), dir.filePath(s("defaults.ini")));
        const AppConfig config = AppConfig::defaults();

        Alarm alarm(eightyMinutes, s("a long message that has to wrap over several lines in a narrow window"));
        CountdownWindow window(&alarm, config, settings);
        window.show();
        QTest::qWaitForWindowShown(&window);

        QWidget *message = window.findChild<QWidget *>(s("descriptionLabel"));
        QVERIFY(message);

        // 12px below the message, nothing more, at any width.
        QCOMPARE(window.width(), 200);
        QCOMPARE(window.height() - (message->geometry().bottom() + 1), 12);
        const int narrowHeight = window.height();

        window.resize(600, 40); // the requested height is ignored
        QTest::qWait(50);
        QCOMPARE(window.width(), 600);
        QCOMPARE(window.height() - (message->geometry().bottom() + 1), 12);
        QVERIFY2(window.height() < narrowHeight, "a wider window should need fewer message lines");

        window.resize(200, 500);
        QTest::qWait(50);
        QCOMPARE(window.height(), narrowHeight);
    }

    void ctrlDragChangesOnlyTheWidth()
    {
        TemporaryDir dir;
        AppSettings settings(dir.filePath(s("user.ini")), dir.filePath(s("defaults.ini")));
        Alarm alarm(eightyMinutes, s("green tea"));
        CountdownWindow window(&alarm, AppConfig::defaults(), settings);
        window.show();
        QTest::qWaitForWindowShown(&window);

        const QSize before = window.size();
        const QPoint grip(window.width() / 2, window.height() - 5);
        QTest::mousePress(&window, Qt::LeftButton, Qt::ControlModifier, grip);
        QMouseEvent move(QEvent::MouseMove, grip + QPoint(80, 30), window.mapToGlobal(grip + QPoint(80, 30)),
                         Qt::NoButton, Qt::LeftButton, Qt::ControlModifier);
        QApplication::sendEvent(&window, &move);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::ControlModifier, grip + QPoint(80, 30));
        QCOMPARE(window.width(), before.width() + 80);
        QVERIFY(window.height() <= before.height()); // follows the content, not the mouse
    }
};

QTEST_MAIN(TestCountdownWindow)
#include "tst_countdownwindow.moc"
