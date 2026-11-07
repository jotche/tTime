#pragma once

#include "alarm-spec.h"

#include <QObject>
#include <QPointer>
#include <QWidget>

class AppSettings;
class Alarm;
struct AppConfig;

// One alarm and the window currently presenting it: the countdown window while
// it runs, the "Time's up" window once it has finished. The holder owns both and
// is the only thing the manager talks to.
class AlarmHolder : public QObject
{
    Q_OBJECT

public:
    // `config` and `settings` are not owned and must outlive the holder.
    AlarmHolder(const AlarmSpec &spec, const AppConfig &config, AppSettings &settings, QObject *parent = 0);
    ~AlarmHolder();

    // Shows the countdown window and starts the alarm.
    void start();

    Alarm *alarm() const { return m_alarm; }
    QWidget *window() const { return m_window; }

signals:
    void finished(); // time is up; the "Time's up" window is showing
    void closed(); // the user is done with this alarm (quit or dismissed)
    void newAlarmRequested(); // the user asked for another alarm

private slots:
    void showTimeUpWindow();

private:
    void showCountdownWindow();
    void replaceWindow(QWidget *window);

    const AppConfig &m_config;
    AppSettings &m_settings;
    Alarm *m_alarm; // child of this holder
    QPointer<QWidget> m_window;
};
