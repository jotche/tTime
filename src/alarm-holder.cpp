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
    connect(m_alarm, &Alarm::finished, this, &AlarmHolder::showTimeUpWindow);
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
    auto *window = new CountdownWindow(m_alarm, m_config, m_settings);
    connect(window, &CountdownWindow::quitRequested, this, &AlarmHolder::closed);
    connect(window, &CountdownWindow::newAlarmRequested, this, &AlarmHolder::newAlarmRequested);
    replaceWindow(window);
    window->show();
}

void AlarmHolder::showTimeUpWindow()
{
    auto *window = new TimeUpWindow(*m_alarm, m_config);
    connect(window, &TimeUpWindow::dismissed, this, &AlarmHolder::closed);
    connect(window, &TimeUpWindow::newAlarmRequested, this, &AlarmHolder::newAlarmRequested);
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
