#include "alarm-holder.h"

#include "alarm.h"
#include "countdown-window.h"
#include "time-up-window.h"
#include "window-utils.h"

AlarmHolder::AlarmHolder(const AlarmSpec &spec, const AppConfig &config, AppSettings &settings, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_settings(settings)
    , m_alarm(new Alarm(spec.duration, spec.message, this))
{
    connect(m_alarm, SIGNAL(finished()), this, SLOT(showTimeUpWindow()));
}

AlarmHolder::~AlarmHolder()
{
    // The window refers to the alarm, so it goes first (the alarm is a child and
    // is deleted after this body). Holders are disposed of with deleteLater()
    // (see AlarmManager), so this never runs inside one of the window's own
    // signal handlers.
    delete m_window.data();
}

void AlarmHolder::start()
{
    showCountdownWindow();
    m_alarm->start();
}

void AlarmHolder::showCountdownWindow()
{
    CountdownWindow *window = new CountdownWindow(m_alarm, m_config, m_settings);
    connect(window, SIGNAL(quitRequested()), this, SIGNAL(closed()));
    connect(window, SIGNAL(newAlarmRequested()), this, SIGNAL(newAlarmRequested()));
    replaceWindow(window);
    window->show();
}

void AlarmHolder::showTimeUpWindow()
{
    TimeUpWindow *window = new TimeUpWindow(*m_alarm, m_config);
    connect(window, SIGNAL(dismissed()), this, SIGNAL(closed()));
    connect(window, SIGNAL(newAlarmRequested()), this, SIGNAL(newAlarmRequested()));
    replaceWindow(window);
    showAndActivate(window);
    emit finished();
}

void AlarmHolder::replaceWindow(QWidget *window)
{
    if (m_window) {
        // We may be inside a signal the old window is connected to, so defer its
        // destruction to the event loop.
        m_window->hide();
        m_window->deleteLater();
    }
    m_window = window;
}
