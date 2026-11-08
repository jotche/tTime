#include "alarm-manager.h"

#include <algorithm>

namespace {

// Runs `function` once the current event (e.g. the click that closes a window) has
// been handled, so the objects behind it are not destroyed while still in use.
void later(std::function<void()> function)
{
    slint::invoke_from_event_loop(std::move(function));
}

} // namespace

// ---------------------------------------------------------------------------
// AlarmHolder
// ---------------------------------------------------------------------------

AlarmHolder::AlarmHolder(const AlarmSpec &spec, const AppConfig &config, AppSettings &settings,
                         const Pictures &pictures)
    : m_config(config)
    , m_settings(settings)
    , m_pictures(pictures)
    , m_alarm(spec.duration, spec.message)
{
    m_alarm.onFinished = [this] { showTimeUpWindow(); };
}

AlarmHolder::~AlarmHolder()
{
    m_alarm.onFinished = nullptr;
    m_countdown.reset(); // the windows refer to the alarm, so they go first
    m_timeUp.reset();
}

void AlarmHolder::start()
{
    m_countdown = std::make_unique<CountdownWindow>(m_alarm, m_config, m_settings, m_pictures);
    m_countdown->onQuitRequested = [this] { onClosed(); };
    m_countdown->onNewAlarmRequested = [this] { onNewAlarmRequested(); };
    m_countdown->show();
    m_alarm.start();
}

void AlarmHolder::showTimeUpWindow()
{
    m_timeUp = std::make_unique<TimeUpWindow>(m_alarm, m_config, m_pictures);
    m_timeUp->onDismissed = [this] { onClosed(); };
    m_timeUp->onNewAlarmRequested = [this] { onNewAlarmRequested(); };
    // We are inside the alarm's tick: replace the countdown window afterwards.
    later([this] { m_countdown.reset(); });
    m_timeUp->show();
}

// ---------------------------------------------------------------------------
// AlarmManager
// ---------------------------------------------------------------------------

AlarmManager::AlarmManager(const AppConfig &config, AppSettings &settings)
    : m_config(config)
    , m_settings(settings)
    // Any component can read the compiled-in pictures; a tray icon that is never shown
    // is the lightest one.
    , m_pictures(loadPictures(ui::TimeUpTray::create(), config.theme))
{
}

NewAlarmWindow &AlarmManager::newAlarmWindow()
{
    // Created on first use: most runs start with an alarm on the command line.
    if (!m_newAlarmWindow) {
        m_newAlarmWindow = std::make_unique<NewAlarmWindow>(m_config);
        m_newAlarmWindow->setWindowIcon(m_pictures.alarmIcon);
        m_newAlarmWindow->onAlarmRequested = [this](const AlarmSpec &spec) { addAlarm(spec); };
        // Deferred: the input window is still closing while this runs.
        m_newAlarmWindow->onDismissed = [this] { later([this] { checkIdle(); }); };
    }
    return *m_newAlarmWindow;
}

void AlarmManager::start(const std::string &initialSpec)
{
    if (const auto spec = parseAlarmSpec(initialSpec)) {
        addAlarm(*spec);
    } else {
        newAlarmWindow().setText(initialSpec);
        showNewAlarmWindow();
    }
}

void AlarmManager::showNewAlarmWindow()
{
    newAlarmWindow().show();
}

void AlarmManager::addAlarm(const AlarmSpec &spec)
{
    if (m_newAlarmWindow) {
        m_newAlarmWindow->hide();
    }
    auto holder = std::make_unique<AlarmHolder>(spec, m_config, m_settings, m_pictures);
    AlarmHolder *raw = holder.get();
    holder->onClosed = [this, raw] { removeAlarm(raw); };
    holder->onNewAlarmRequested = [this] { showNewAlarmWindow(); };
    m_alarms.push_back(std::move(holder));
    raw->start();
}

void AlarmManager::removeAlarm(AlarmHolder *holder)
{
    // We are inside a callback of the holder's window (e.g. its "Quit" menu item), so
    // the holder and its windows are destroyed only after that callback has returned.
    later([this, holder] {
        const auto it = std::ranges::find(m_alarms, holder, &std::unique_ptr<AlarmHolder>::get);
        if (it == m_alarms.end()) {
            return; // already removed (e.g. closed twice)
        }
        m_alarms.erase(it);
        checkIdle();
    });
}

void AlarmManager::checkIdle()
{
    if (m_alarms.empty() && !(m_newAlarmWindow && m_newAlarmWindow->isVisible()) && onIdle) {
        onIdle();
    }
}
