#include "alarm-manager.h"

#include "alarm-holder.h"
#include "new-alarm-window.h"
#include "window-utils.h"

AlarmManager::AlarmManager(const AppConfig &config, AppSettings &settings, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_settings(settings)
    , m_newAlarmWindow(new NewAlarmWindow(config))
{
    connect(m_newAlarmWindow.data(), SIGNAL(alarmRequested(AlarmSpec)), this, SLOT(addAlarm(AlarmSpec)));
    // Queued: the input window is still visible while its closeEvent runs.
    connect(m_newAlarmWindow.data(), SIGNAL(dismissed()), this, SLOT(checkIdle()), Qt::QueuedConnection);
}

AlarmManager::~AlarmManager()
{
    // Holders are children and go with the QObject.
}

void AlarmManager::start(const QString &initialSpec)
{
    AlarmSpec spec;
    if (parseAlarmSpec(initialSpec, &spec)) {
        addAlarm(spec);
    } else {
        m_newAlarmWindow->setText(initialSpec);
        showNewAlarmWindow();
    }
}

void AlarmManager::showNewAlarmWindow()
{
    showAndActivate(m_newAlarmWindow.data());
}

void AlarmManager::addAlarm(const AlarmSpec &spec)
{
    m_newAlarmWindow->hide();

    AlarmHolder *holder = new AlarmHolder(spec, m_config, m_settings, this);
    connect(holder, SIGNAL(closed()), this, SLOT(onHolderClosed()));
    connect(holder, SIGNAL(newAlarmRequested()), this, SLOT(showNewAlarmWindow()));
    m_alarms.append(holder);
    holder->start();
}

void AlarmManager::onHolderClosed()
{
    removeAlarm(qobject_cast<AlarmHolder *>(sender()));
}

void AlarmManager::removeAlarm(AlarmHolder *holder)
{
    if (!holder || !m_alarms.removeOne(holder)) {
        return; // already removed (e.g. closed twice)
    }
    // We are inside a signal emitted from the holder's window (e.g. its "Quit"
    // action, the old "segfault when quitting from the context menu"), so the
    // holder and its window are deleted only after that handler has returned.
    holder->disconnect(this);
    holder->deleteLater();
    checkIdle();
}

void AlarmManager::checkIdle()
{
    if (m_alarms.isEmpty() && !m_newAlarmWindow->isVisible()) {
        emit idle();
    }
}
