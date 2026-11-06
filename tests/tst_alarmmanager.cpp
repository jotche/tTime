#include "alarm-holder.h"
#include "alarm-manager.h"
#include "alarm.h"
#include "app-config.h"
#include "app-settings.h"
#include "new-alarm-window.h"
#include "time-up-window.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QSignalSpy>
#include <QSystemTrayIcon>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>

#include <memory>

// The life of alarms from start to disposal, through the real manager, holders
// and windows - in particular the paths where a window asks to be disposed of
// from inside its own signal handler (the old "segfault when quitting from the
// context menu").
class TestAlarmManager : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    std::unique_ptr<AppSettings> m_settings;
    AppConfig m_config = AppConfig::defaults();

    // Waits until the holder is gone, the way the event loop would dispose of it.
    static bool disposed(const QPointer<AlarmHolder> &holder)
    {
        return QTest::qWaitFor([&] { return holder.isNull(); }, 2000);
    }

    static QPointer<AlarmHolder> waitForTimeUp(AlarmManager &manager)
    {
        QPointer<AlarmHolder> holder = manager.alarms().front();
        QSignalSpy finished(holder.data(), &AlarmHolder::finished);
        if (!finished.wait(4000)) {
            return {};
        }
        return holder;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_config.alarmSound = QUrl(); // no sound in tests
    }

    void init()
    {
        m_settings = std::make_unique<AppSettings>(m_dir.filePath(QStringLiteral("user.ini")),
                                                   m_dir.filePath(QStringLiteral("defaults.ini")));
    }

    void cleanup()
    {
        QTest::qWait(50); // let deferred deletions run before the next test
        m_settings.reset();
    }

    void validCommandLineStartsAnAlarm()
    {
        AlarmManager manager(m_config, *m_settings);
        manager.start(QStringLiteral("1h tea"));
        QCOMPARE(manager.alarms().size(), std::size_t(1));
        QVERIFY(!manager.newAlarmWindow()->isVisible());
        AlarmHolder *holder = manager.alarms().front();
        QCOMPARE(holder->alarm()->state(), Alarm::State::Running);
        QCOMPARE(holder->alarm()->message(), QStringLiteral("tea"));
        QVERIFY(holder->window() && holder->window()->isVisible());
    }

    void invalidCommandLineOpensTheInputWindow()
    {
        AlarmManager manager(m_config, *m_settings);
        manager.start(QStringLiteral("tea"));
        QVERIFY(manager.alarms().empty());
        QVERIFY(manager.newAlarmWindow()->isVisible());
    }

    void quitFromTheCountdownContextMenu()
    {
        // Exactly what the user does: right-click the countdown window and click
        // "Quit" in the menu. The menu runs its own event loop on top of the
        // window; disposing of the window from inside it is what used to crash.
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, &AlarmManager::idle);
        manager.start(QStringLiteral("1h tea"));
        QPointer<AlarmHolder> holder = manager.alarms().front();
        QWidget *window = holder->window();
        QVERIFY(QTest::qWaitForWindowExposed(window));
        auto *quit = window->findChild<QAction *>(QStringLiteral("quitAction"));
        QVERIFY(quit);

        bool clicked = false;
        QTimer::singleShot(200, this, [&] {
            if (auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget())) {
                QTest::mouseClick(menu, Qt::LeftButton, {}, menu->actionGeometry(quit).center());
                clicked = true;
            }
        });
        const QPoint inside(window->width() / 2, window->height() - 10);
        QContextMenuEvent rightClick(QContextMenuEvent::Mouse, inside, window->mapToGlobal(inside));
        QApplication::sendEvent(window, &rightClick); // returns when the menu closes

        QVERIFY(clicked);
        QVERIFY(manager.alarms().empty());
        QVERIFY(disposed(holder));
        QCOMPARE(idle.count(), 1);
    }

    void quitFromTheToolbarButton()
    {
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, &AlarmManager::idle);
        manager.start(QStringLiteral("1h tea"));
        QPointer<AlarmHolder> holder = manager.alarms().front();
        QVERIFY(QTest::qWaitForWindowExposed(holder->window()));

        auto *exit = holder->window()->findChild<QToolButton *>(QStringLiteral("exitButton"));
        QVERIFY(exit);
        QTest::mouseClick(exit, Qt::LeftButton);

        QVERIFY(disposed(holder));
        QCOMPARE(idle.count(), 1);
    }

    void quitOneOfTwoAlarms()
    {
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, &AlarmManager::idle);
        manager.start(QStringLiteral("1h first"));
        manager.start(QStringLiteral("2h second"));
        QCOMPARE(manager.alarms().size(), std::size_t(2));
        QPointer<AlarmHolder> first = manager.alarms().front();

        first->window()->findChild<QAction *>(QStringLiteral("quitAction"))->trigger();
        QVERIFY(disposed(first));
        QCOMPARE(manager.alarms().size(), std::size_t(1));
        QCOMPARE(manager.alarms().front()->alarm()->message(), QStringLiteral("second"));
        QCOMPARE(idle.count(), 0);
    }

    void newAlarmFromTheCountdownMenu()
    {
        AlarmManager manager(m_config, *m_settings);
        manager.start(QStringLiteral("1h tea"));
        manager.alarms().front()->window()->findChild<QAction *>(QStringLiteral("newAlarmAction"))->trigger();
        QVERIFY(manager.newAlarmWindow()->isVisible());
        QCOMPARE(manager.alarms().size(), std::size_t(1)); // the running alarm stays
    }

    void dismissTheTimeUpWindow()
    {
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, &AlarmManager::idle);
        manager.start(QStringLiteral("1s tea"));

        const QPointer<AlarmHolder> holder = waitForTimeUp(manager);
        QVERIFY(holder);
        auto *window = qobject_cast<TimeUpWindow *>(holder->window());
        QVERIFY(window);
        QTRY_VERIFY(window->isVisible());

        auto *close = window->findChild<QPushButton *>(QStringLiteral("closeButton"));
        QVERIFY(close);
        close->click();

        QVERIFY(disposed(holder));
        QVERIFY(manager.alarms().empty());
        QCOMPARE(idle.count(), 1);
    }

    void newAlarmFromTheTimeUpWindow()
    {
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, &AlarmManager::idle);
        manager.start(QStringLiteral("1s tea"));

        const QPointer<AlarmHolder> holder = waitForTimeUp(manager);
        QVERIFY(holder);
        holder->window()->findChild<QPushButton *>(QStringLiteral("newTimerButton"))->click();

        QVERIFY(disposed(holder));
        QVERIFY(manager.newAlarmWindow()->isVisible());
        QCOMPARE(idle.count(), 0); // the input window keeps the app alive
    }

    void countdownWindowSavesItsPosition()
    {
        {
            AlarmManager manager(m_config, *m_settings);
            manager.start(QStringLiteral("1h tea"));
            QWidget *window = manager.alarms().front()->window();
            window->move(window->pos() + QPoint(40, 30)); // the user moves it
            const QPoint moved = window->pos();
            window->findChild<QAction *>(QStringLiteral("quitAction"))->trigger();
            QTest::qWait(50);
            QCOMPARE(m_settings->value(QStringLiteral("progress-dialog/pos")).toPoint(), moved);
        }
    }

    void treeSwaysInTheTimeUpWindow()
    {
        AlarmManager manager(m_config, *m_settings);
        manager.start(QStringLiteral("1s tea"));
        const QPointer<AlarmHolder> holder = waitForTimeUp(manager);
        QVERIFY(holder);
        auto *tree = holder->window()->findChild<QLabel *>(QStringLiteral("label"));
        QVERIFY(tree);
        const QImage first = tree->pixmap().toImage();
        QTest::qWait(400);
        QVERIFY(tree->pixmap().toImage() != first); // the breeze moved the tree
    }

    void animationsOffKeepsTheTimeUpWindowStill()
    {
        AppConfig still = m_config;
        still.animations = false;
        AlarmManager manager(still, *m_settings);
        manager.start(QStringLiteral("1s tea"));
        const QPointer<AlarmHolder> holder = waitForTimeUp(manager);
        QVERIFY(holder);
        auto *tree = holder->window()->findChild<QLabel *>(QStringLiteral("label"));
        auto *tray = holder->window()->findChild<QSystemTrayIcon *>();
        QVERIFY(tree && tray);
        const QImage treeBefore = tree->pixmap().toImage();
        const qint64 trayBefore = tray->icon().cacheKey();
        QTest::qWait(400);
        QCOMPARE(tree->pixmap().toImage(), treeBefore);
        QCOMPARE(tray->icon().cacheKey(), trayBefore);
    }

    void destroyingTheManagerWithRunningAlarms()
    {
        auto manager = std::make_unique<AlarmManager>(m_config, *m_settings);
        manager->start(QStringLiteral("1h one"));
        manager->start(QStringLiteral("1h two"));
        QPointer<QWidget> window = manager->alarms().front()->window();
        manager.reset(); // like quitting the application
        QVERIFY(window.isNull());
    }
};

QTEST_MAIN(TestAlarmManager)
#include "tst_alarmmanager.moc"
