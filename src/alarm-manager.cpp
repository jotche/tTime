#include "alarm-manager.h"

#include "alarm-holder.h"
#include "new-alarm-window.h"
#include "window-utils.h"

#include <algorithm>

AlarmManager::AlarmManager(const AppConfig &config, AppSettings &settings, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_settings(settings)
    , m_newAlarmWindow(std::make_unique<NewAlarmWindow>(config))
{
    connect(m_newAlarmWindow.get(), &NewAlarmWindow::alarmRequested, this, &AlarmManager::addAlarm);
    // Queued: the input window is still visible while its closeEvent runs.
    connect(m_newAlarmWindow.get(), &NewAlarmWindow::dismissed, this, &AlarmManager::checkIdle, Qt::QueuedConnection);
}

AlarmManager::~AlarmManager() = default; // holders are children and go with the QObject

void AlarmManager::start(const QString &initialSpec)
{
    if (const auto spec = parseAlarmSpec(initialSpec)) {
        addAlarm(*spec);
    } else {
        m_newAlarmWindow->setText(initialSpec);
        showNewAlarmWindow();
    }
}

void AlarmManager::showNewAlarmWindow()
{
    showAndActivate(m_newAlarmWindow.get());
}

void AlarmManager::addAlarm(const AlarmSpec &spec)
{
    m_newAlarmWindow->hide();

    auto *holder = new AlarmHolder(spec, m_config, m_settings, this);
    connect(holder, &AlarmHolder::closed, this, [this, holder] { removeAlarm(holder); });
    connect(holder, &AlarmHolder::newAlarmRequested, this, &AlarmManager::showNewAlarmWindow);
    m_alarms.push_back(holder);
    holder->start();
}

void AlarmManager::removeAlarm(AlarmHolder *holder)
{
    const auto it = std::ranges::find(m_alarms, holder);
    if (it == m_alarms.end()) {
        return; // already removed (e.g. closed twice)
    }
    m_alarms.erase(it);
    // We are inside a signal emitted from the holder's window (e.g. its "Quit"
    // action, the old "segfault when quitting from the context menu"), so the
    // holder and its window are deleted only after that handler has returned.
    holder->disconnect(this);
    holder->deleteLater();
    checkIdle();
}

void AlarmManager::checkIdle()
{
    if (m_alarms.empty() && !m_newAlarmWindow->isVisible()) {
        emit idle();
    }
}
