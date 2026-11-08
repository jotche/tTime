#pragma once

#include "alarm-spec.h"
#include "alarm.h"
#include "config.h"
#include "countdown-window.h"
#include "new-alarm-window.h"
#include "time-up-window.h"

#include <functional>
#include <memory>
#include <vector>

class AppSettings;

// One alarm and the window currently presenting it: the countdown window while it
// runs, the "Time's up" window once it has finished.
class AlarmHolder
{
public:
    AlarmHolder(const AlarmSpec &spec, const AppConfig &config, AppSettings &settings, const Pictures &pictures);
    ~AlarmHolder();

    // Shows the countdown window and starts the alarm.
    void start();

    std::function<void()> onClosed; // the user is done with this alarm (quit or dismissed)
    std::function<void()> onNewAlarmRequested;

private:
    void showTimeUpWindow();

    const AppConfig &m_config;
    AppSettings &m_settings;
    const Pictures &m_pictures;
    Alarm m_alarm;
    std::unique_ptr<CountdownWindow> m_countdown;
    std::unique_ptr<TimeUpWindow> m_timeUp;
};

// Creates alarms (from the command line or the input window), keeps them while they
// are in use and reports when the application has nothing left to do.
class AlarmManager
{
public:
    AlarmManager(const AppConfig &config, AppSettings &settings);

    // `initialSpec` is e.g. the command line ("5m green tea"): a valid spec starts an
    // alarm, anything else opens the input window (pre-filled if not empty).
    void start(const std::string &initialSpec);

    // No alarm is left and the input window is closed: the application can quit.
    std::function<void()> onIdle;

private:
    NewAlarmWindow &newAlarmWindow();
    void addAlarm(const AlarmSpec &spec);
    void showNewAlarmWindow();
    void removeAlarm(AlarmHolder *holder);
    void checkIdle();

    const AppConfig &m_config;
    AppSettings &m_settings;
    Pictures m_pictures;
    std::unique_ptr<NewAlarmWindow> m_newAlarmWindow;
    std::vector<std::unique_ptr<AlarmHolder>> m_alarms;
};
