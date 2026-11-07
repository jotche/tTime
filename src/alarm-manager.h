#pragma once

#include "alarm-spec.h"

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QString>

class AlarmHolder;
class AppSettings;
class NewAlarmWindow;
struct AppConfig;

// Creates alarms (from the command line or the input window), keeps them while
// they are in use and reports when the application has nothing left to do.
class AlarmManager : public QObject
{
    Q_OBJECT

public:
    // `config` and `settings` are not owned and must outlive the manager.
    AlarmManager(const AppConfig &config, AppSettings &settings, QObject *parent = 0);
    ~AlarmManager();

    // `initialSpec` is e.g. the command line ("5m green tea"): a valid spec starts
    // an alarm, anything else opens the input window (pre-filled if not empty).
    void start(const QString &initialSpec);

    const QList<AlarmHolder *> &alarms() const { return m_alarms; }
    NewAlarmWindow *newAlarmWindow() const { return m_newAlarmWindow.data(); }

signals:
    // No alarm is left and the input window is closed: the application can quit.
    void idle();

private slots:
    void addAlarm(const AlarmSpec &spec);
    void showNewAlarmWindow();
    void onHolderClosed();
    void checkIdle();

private:
    Q_DISABLE_COPY(AlarmManager)

    void removeAlarm(AlarmHolder *holder);

    const AppConfig &m_config;
    AppSettings &m_settings;
    QScopedPointer<NewAlarmWindow> m_newAlarmWindow;
    QList<AlarmHolder *> m_alarms; // children of this manager
};
