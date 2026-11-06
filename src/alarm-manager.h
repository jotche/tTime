#pragma once

#include "alarm-spec.h"

#include <QObject>
#include <QString>

#include <memory>
#include <vector>

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
    AlarmManager(const AppConfig &config, AppSettings &settings, QObject *parent = nullptr);
    ~AlarmManager() override;

    AlarmManager(const AlarmManager &) = delete;
    AlarmManager &operator=(const AlarmManager &) = delete;

    // `initialSpec` is e.g. the command line ("5m green tea"): a valid spec starts
    // an alarm, anything else opens the input window (pre-filled if not empty).
    void start(const QString &initialSpec);

    [[nodiscard]] const std::vector<AlarmHolder *> &alarms() const noexcept { return m_alarms; }
    [[nodiscard]] NewAlarmWindow *newAlarmWindow() const noexcept { return m_newAlarmWindow.get(); }

signals:
    // No alarm is left and the input window is closed: the application can quit.
    void idle();

private:
    void addAlarm(const AlarmSpec &spec);
    void showNewAlarmWindow();
    void removeAlarm(AlarmHolder *holder);
    void checkIdle();

    const AppConfig &m_config;
    AppSettings &m_settings;
    std::unique_ptr<NewAlarmWindow> m_newAlarmWindow;
    std::vector<AlarmHolder *> m_alarms; // children of this manager
};
