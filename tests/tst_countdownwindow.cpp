#include "alarm.h"
#include "app-config.h"
#include "app-settings.h"
#include "countdown-window.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>

using namespace std::chrono_literals;

namespace {

// Geometry of everything in the countdown window, in window coordinates.
QMap<QString, QRect> layoutSnapshot(QWidget *window)
{
    QMap<QString, QRect> snapshot;
    for (QWidget *child : window->findChildren<QWidget *>()) {
        if (!child->objectName().isEmpty() && child->isVisible()) {
            snapshot.insert(child->objectName(), QRect(child->mapTo(window, QPoint(0, 0)), child->size()));
        }
    }
    return snapshot;
}

QString describeChanges(const QMap<QString, QRect> &before, const QMap<QString, QRect> &after)
{
    QStringList changes;
    for (auto it = before.cbegin(); it != before.cend(); ++it) {
        const QRect now = after.value(it.key());
        if (now != it.value()) {
            changes << QStringLiteral("%1: (%2,%3 %4x%5) -> (%6,%7 %8x%9)")
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
    return changes.join(QStringLiteral("; "));
}

} // namespace

class TestCountdownWindow : public QObject
{
    Q_OBJECT

private slots:
    void layoutDoesNotJumpWhenStartingAfterStop()
    {
        // Built-in configuration and throw-away settings: the test does not
        // depend on (or change) the user's tTime.ini.
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("user.ini")), dir.filePath(QStringLiteral("defaults.ini")));
        const AppConfig config = AppConfig::defaults();

        Alarm alarm(80min, QStringLiteral("green tea"));
        CountdownWindow window(&alarm, config, settings);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        alarm.start();
        QTest::qWait(50);

        auto *playPause = window.findChild<QToolButton *>(QStringLiteral("playPauseButton"));
        auto *stop = window.findChild<QToolButton *>(QStringLiteral("stopButton"));
        QVERIFY(playPause && stop);

        const auto initial = layoutSnapshot(&window);

        // The user's sequence: press stop, move to start, press start, pointer stays on it.
        QTest::mouseMove(stop);
        QTest::mouseClick(stop, Qt::LeftButton);
        QTest::qWait(50);
        QCOMPARE(alarm.state(), Alarm::State::NotRunning);
        QVERIFY2(layoutSnapshot(&window) == initial,
                 qPrintable(QStringLiteral("after stop: ") + describeChanges(initial, layoutSnapshot(&window))));

        QTest::mouseMove(playPause);
        QTest::qWait(50);
        QVERIFY2(layoutSnapshot(&window) == initial,
                 qPrintable(QStringLiteral("hovering start: ") + describeChanges(initial, layoutSnapshot(&window))));

        QTest::mouseClick(playPause, Qt::LeftButton);
        QTest::qWait(50);
        QCOMPARE(alarm.state(), Alarm::State::Running);
        QVERIFY2(layoutSnapshot(&window) == initial,
                 qPrintable(QStringLiteral("after start: ") + describeChanges(initial, layoutSnapshot(&window))));

        QTest::mouseMove(&window, QPoint(window.width() / 2, window.height() - 5));
        QTest::qWait(50);
        QVERIFY2(layoutSnapshot(&window) == initial,
                 qPrintable(QStringLiteral("pointer away: ") + describeChanges(initial, layoutSnapshot(&window))));
    }
};

QTEST_MAIN(TestCountdownWindow)
#include "tst_countdownwindow.moc"
