#include "alarm.h"
#include "app-config.h"
#include "app-settings.h"
#include "countdown-window.h"

#include <QApplication>
#include <QFontMetrics>
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

    void layoutFollowsTheSpacingRules()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("user.ini")), dir.filePath(QStringLiteral("defaults.ini")));
        const AppConfig config = AppConfig::defaults();

        Alarm alarm(80min, QStringLiteral("green tea"));
        CountdownWindow window(&alarm, config, settings);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        const auto at = [&](const char *name) {
            auto *child = window.findChild<QWidget *>(QLatin1String(name));
            return child ? QRect(child->mapTo(&window, QPoint(0, 0)), child->size()) : QRect();
        };
        const QRect play = at("playPauseButton");
        const QRect time = at("timeLabel");
        const QRect message = at("descriptionLabel");
        const QRect windowActions = at("windowActions");

        // Window padding 12px top and bottom, 8px left and right; 8px between the
        // action row and the content; the content is indented 4px and has 4px
        // between the countdown and the message.
        QCOMPARE(play.topLeft(), QPoint(8, 12));
        QCOMPARE(time.left(), 8 + 4);
        QCOMPARE(time.top(), play.bottom() + 1 + 8);
        QCOMPARE(message.left(), 8 + 4);
        QCOMPARE(message.top(), time.bottom() + 1 + 4);

        // The countdown label ends at the digits' baseline: no descender space below them.
        const QFontMetrics metrics(config.countdownFont);
        QCOMPARE(time.height(), metrics.height() - metrics.descent());
        QCOMPARE(message.right(), window.width() - 8 - 1);

        // Window actions: 8px from the top-right corner, also after a resize.
        QCOMPARE(windowActions.topRight(), QPoint(window.width() - 8 - 1, 8));
        window.resize(window.width() + 60, window.height() + 20);
        QCOMPARE(at("windowActions").topRight(), QPoint(window.width() - 8 - 1, 8));

        // At its narrowest, the window actions still keep clear of the action row.
        window.resize(1, 1);
        QTest::qWait(50);
        QCOMPARE(window.width(), 200); // minimum width
        QVERIFY2(at("restartButton").right() + 8 < at("windowActions").left(),
                 qPrintable(QStringLiteral("restart ends at %1, window actions start at %2")
                                .arg(at("restartButton").right())
                                .arg(at("windowActions").left())));
    }

    void heightFollowsTheContent()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("user.ini")), dir.filePath(QStringLiteral("defaults.ini")));
        const AppConfig config = AppConfig::defaults();

        Alarm alarm(80min, QStringLiteral("a long message that has to wrap over several lines in a narrow window"));
        CountdownWindow window(&alarm, config, settings);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        auto *message = window.findChild<QWidget *>(QStringLiteral("descriptionLabel"));
        QVERIFY(message);
        const auto bottomPadding = [&] { return window.height() - (message->geometry().bottom() + 1); };

        // 12px below the message, nothing more, at any width.
        QCOMPARE(window.width(), 200);
        QCOMPARE(bottomPadding(), 12);
        const int narrowHeight = window.height();

        window.resize(600, 40); // the requested height is ignored
        QTest::qWait(50);
        QCOMPARE(window.width(), 600);
        QCOMPARE(bottomPadding(), 12);
        QVERIFY2(window.height() < narrowHeight, "a wider window should need fewer message lines");

        window.resize(200, 500);
        QTest::qWait(50);
        QCOMPARE(window.height(), narrowHeight);
    }
};

QTEST_MAIN(TestCountdownWindow)
#include "tst_countdownwindow.moc"
