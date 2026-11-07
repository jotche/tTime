#include "alarm-holder.h"
#include "alarm-manager.h"
#include "alarm.h"
#include "app-config.h"
#include "app-settings.h"
#include "countdown-font.h"
#include "new-alarm-window.h"
#include "testutil.h"
#include "time-up-window.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QSound>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QSystemTrayIcon>
#include <QTest>
#include <QTimer>
#include <QToolButton>

namespace {

// True once the guarded object is gone.
struct IsNull
{
    explicit IsNull(const QPointer<AlarmHolder> &holder)
        : holder(holder)
    {
    }
    bool operator()() const { return holder.isNull(); }
    const QPointer<AlarmHolder> &holder;
};

// Clicks `action` in the context menu that is open when its timer fires.
class MenuClicker : public QObject
{
    Q_OBJECT

public:
    explicit MenuClicker(QAction *action)
        : clicked(false)
        , m_action(action)
    {
    }

    bool clicked;

public slots:
    void click()
    {
        if (QMenu *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget())) {
            QTest::mouseClick(menu, Qt::LeftButton, 0, menu->actionGeometry(m_action).center());
            clicked = true;
        }
    }

private:
    QAction *m_action;
};

QImage pixmapOf(QLabel *label)
{
    return label->pixmap() ? label->pixmap()->toImage() : QImage();
}

} // namespace

// The life of alarms from start to disposal, through the real manager, holders
// and windows - in particular the paths where a window asks to be disposed of
// from inside its own signal handler (the old "segfault when quitting from the
// context menu").
class TestAlarmManager : public QObject
{
    Q_OBJECT

private:
    TemporaryDir m_dir;
    QScopedPointer<AppSettings> m_settings;
    AppConfig m_config;

    // Waits until the holder is gone, the way the event loop would dispose of it.
    static bool disposed(const QPointer<AlarmHolder> &holder) { return waitFor(IsNull(holder), 2000); }

    static QPointer<AlarmHolder> waitForTimeUp(AlarmManager &manager)
    {
        QPointer<AlarmHolder> holder = manager.alarms().first();
        QSignalSpy finished(holder.data(), SIGNAL(finished()));
        if (!waitForSignal(finished, 4000)) {
            return QPointer<AlarmHolder>();
        }
        return holder;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        loadBundledFonts();
        m_config = AppConfig::defaults();
        m_config.alarmSound = QString(); // no sound in tests
    }

    void init()
    {
        m_settings.reset(new AppSettings(m_dir.filePath(s("user.ini")), m_dir.filePath(s("defaults.ini"))));
    }

    void cleanup()
    {
        runEventLoop(50); // let deferred deletions run before the next test
        m_settings.reset();
    }

    void validCommandLineStartsAnAlarm()
    {
        AlarmManager manager(m_config, *m_settings);
        manager.start(s("1h tea"));
        QCOMPARE(manager.alarms().size(), 1);
        QVERIFY(!manager.newAlarmWindow()->isVisible());
        AlarmHolder *holder = manager.alarms().first();
        QCOMPARE(holder->alarm()->state(), Alarm::Running);
        QCOMPARE(holder->alarm()->message(), s("tea"));
        QVERIFY(holder->window() && holder->window()->isVisible());
    }

    void invalidCommandLineOpensTheInputWindow()
    {
        AlarmManager manager(m_config, *m_settings);
        manager.start(s("tea"));
        QVERIFY(manager.alarms().isEmpty());
        QVERIFY(manager.newAlarmWindow()->isVisible());
    }

    void quitFromTheCountdownContextMenu()
    {
        // Exactly what the user does: right-click the countdown window and click
        // "Quit" in the menu. The menu runs its own event loop on top of the
        // window; disposing of the window from inside it is what used to crash.
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, SIGNAL(idle()));
        manager.start(s("1h tea"));
        QPointer<AlarmHolder> holder = manager.alarms().first();
        QWidget *window = holder->window();
        QTest::qWaitForWindowShown(window);
        QAction *quit = window->findChild<QAction *>(s("quitAction"));
        QVERIFY(quit);

        MenuClicker clicker(quit);
        QTimer::singleShot(200, &clicker, SLOT(click()));
        const QPoint inside(window->width() / 2, window->height() - 10);
        QContextMenuEvent rightClick(QContextMenuEvent::Mouse, inside, window->mapToGlobal(inside));
        QApplication::sendEvent(window, &rightClick); // returns when the menu closes

        QVERIFY(clicker.clicked);
        QVERIFY(manager.alarms().isEmpty());
        QVERIFY(disposed(holder));
        QCOMPARE(idle.count(), 1);
    }

    void quitFromTheToolbarButton()
    {
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, SIGNAL(idle()));
        manager.start(s("1h tea"));
        QPointer<AlarmHolder> holder = manager.alarms().first();
        QTest::qWaitForWindowShown(holder->window());

        QToolButton *exit = holder->window()->findChild<QToolButton *>(s("exitButton"));
        QVERIFY(exit);
        QTest::mouseClick(exit, Qt::LeftButton);

        QVERIFY(disposed(holder));
        QCOMPARE(idle.count(), 1);
    }

    void quitOneOfTwoAlarms()
    {
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, SIGNAL(idle()));
        manager.start(s("1h first"));
        manager.start(s("2h second"));
        QCOMPARE(manager.alarms().size(), 2);
        QPointer<AlarmHolder> first = manager.alarms().first();

        first->window()->findChild<QAction *>(s("quitAction"))->trigger();
        QVERIFY(disposed(first));
        QCOMPARE(manager.alarms().size(), 1);
        QCOMPARE(manager.alarms().first()->alarm()->message(), s("second"));
        QCOMPARE(idle.count(), 0);
    }

    void newAlarmFromTheCountdownMenu()
    {
        AlarmManager manager(m_config, *m_settings);
        manager.start(s("1h tea"));
        manager.alarms().first()->window()->findChild<QAction *>(s("newAlarmAction"))->trigger();
        QVERIFY(manager.newAlarmWindow()->isVisible());
        QCOMPARE(manager.alarms().size(), 1); // the running alarm stays
    }

    void dismissTheTimeUpWindow()
    {
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, SIGNAL(idle()));
        manager.start(s("1s tea"));

        const QPointer<AlarmHolder> holder = waitForTimeUp(manager);
        QVERIFY(holder);
        TimeUpWindow *window = qobject_cast<TimeUpWindow *>(holder->window());
        QVERIFY(window);
        QVERIFY(window->isVisible());

        QPushButton *close = window->findChild<QPushButton *>(s("closeButton"));
        QVERIFY(close);
        close->click();

        QVERIFY(disposed(holder));
        QVERIFY(manager.alarms().isEmpty());
        QCOMPARE(idle.count(), 1);
    }

    void newAlarmFromTheTimeUpWindow()
    {
        AlarmManager manager(m_config, *m_settings);
        QSignalSpy idle(&manager, SIGNAL(idle()));
        manager.start(s("1s tea"));

        const QPointer<AlarmHolder> holder = waitForTimeUp(manager);
        QVERIFY(holder);
        holder->window()->findChild<QPushButton *>(s("newTimerButton"))->click();

        QVERIFY(disposed(holder));
        QVERIFY(manager.newAlarmWindow()->isVisible());
        QCOMPARE(idle.count(), 0); // the input window keeps the app alive
    }

    void countdownWindowSavesItsPosition()
    {
        AlarmManager manager(m_config, *m_settings);
        manager.start(s("1h tea"));
        QWidget *window = manager.alarms().first()->window();
        window->move(window->pos() + QPoint(40, 30)); // the user moves it
        const QPoint moved = window->pos();
        window->findChild<QAction *>(s("quitAction"))->trigger();
        runEventLoop(50); // the window saves its position when it is deleted
        QCOMPARE(m_settings->value(s("progress-dialog/pos")).toPoint(), moved);
    }

    void treeSwaysInTheTimeUpWindow()
    {
        AlarmManager manager(m_config, *m_settings);
        manager.start(s("1s tea"));
        const QPointer<AlarmHolder> holder = waitForTimeUp(manager);
        QVERIFY(holder);
        QLabel *tree = holder->window()->findChild<QLabel *>(s("label"));
        QVERIFY(tree);
        const QImage first = pixmapOf(tree);
        QTest::qWait(400);
        QVERIFY(pixmapOf(tree) != first); // the breeze moved the tree
    }

    void animationsOffKeepsTheTimeUpWindowStill()
    {
        AppConfig still = m_config;
        still.animations = false;
        AlarmManager manager(still, *m_settings);
        manager.start(s("1s tea"));
        const QPointer<AlarmHolder> holder = waitForTimeUp(manager);
        QVERIFY(holder);
        QLabel *tree = holder->window()->findChild<QLabel *>(s("label"));
        QSystemTrayIcon *tray = holder->window()->findChild<QSystemTrayIcon *>();
        QVERIFY(tree && tray);
        const QImage treeBefore = pixmapOf(tree);
        const qint64 trayBefore = tray->icon().cacheKey();
        QTest::qWait(400);
        QCOMPARE(pixmapOf(tree), treeBefore);
        QCOMPARE(tray->icon().cacheKey(), trayBefore);
    }

    void missingAlarmSoundIsReported()
    {
        // The sound is a file in the Resources folder next to tTime.exe; without it
        // the alarm is silent and says why.
        AppConfig config = m_config;
        config.alarmSound = m_dir.filePath(s("Resources/sounds/missing.wav"));
        const Alarm alarm(1, s("tea"));
        QTest::ignoreMessage(QtWarningMsg, qPrintable(s("Alarm sound not found: ")
                                                      + QDir::toNativeSeparators(config.alarmSound)));
        TimeUpWindow window(alarm, config);
        QVERIFY(!window.findChild<QSound *>());
    }

    void alarmSoundPlaysFromAFile()
    {
        AppConfig config = m_config;
        config.alarmSound = s(TTIME_SOURCE_DIR "/src/Resources/sounds/submarine1.wav");
        const Alarm alarm(1, s("tea"));
        TimeUpWindow window(alarm, config);
        QSound *sound = window.findChild<QSound *>();
        QVERIFY(sound);
        QCOMPARE(QDir::fromNativeSeparators(sound->fileName()), config.alarmSound);
        QCOMPARE(sound->loops(), 3); // about 7 s of the 2.7 s sound
        window.close(); // stops the sound
    }

    void destroyingTheManagerWithRunningAlarms()
    {
        QScopedPointer<AlarmManager> manager(new AlarmManager(m_config, *m_settings));
        manager->start(s("1h one"));
        manager->start(s("1h two"));
        QPointer<QWidget> window = manager->alarms().first()->window();
        manager.reset(); // like quitting the application
        QVERIFY(window.isNull());
    }
};

QTEST_MAIN(TestAlarmManager)
#include "tst_alarmmanager.moc"
