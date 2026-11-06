#include "alarm.h"

#include <algorithm>

using namespace std::chrono;
using namespace std::chrono_literals;

Alarm::Alarm(seconds duration, QString message, QObject *parent)
    : QObject(parent)
    , m_message(std::move(message))
    , m_duration(std::max(duration, 0s))
    , m_remainingAtResume(m_duration)
{
    m_ticker.setInterval(250ms);
    connect(&m_ticker, &QTimer::timeout, this, &Alarm::tick);
}

seconds Alarm::duration() const noexcept
{
    return duration_cast<seconds>(m_duration);
}

milliseconds Alarm::remaining() const
{
    if (m_state == State::Running) {
        return std::max(m_remainingAtResume - milliseconds{m_sinceResume.elapsed()}, 0ms);
    }
    return m_remainingAtResume;
}

seconds Alarm::remainingSeconds() const
{
    return ceil<seconds>(remaining());
}

void Alarm::start()
{
    if (m_state == State::Running) {
        return;
    }
    if (m_state == State::NotRunning || m_state == State::Finished) {
        m_remainingAtResume = m_duration;
    }
    m_sinceResume.start();
    m_ticker.start();
    setState(State::Running);
    tick();
}

void Alarm::pause()
{
    if (m_state != State::Running) {
        return;
    }
    m_remainingAtResume = remaining();
    m_ticker.stop();
    setState(State::Paused);
    reportTimeLeft(remainingSeconds());
}

void Alarm::stop()
{
    m_ticker.stop();
    m_remainingAtResume = m_duration;
    setState(State::NotRunning);
    reportTimeLeft(remainingSeconds());
}

void Alarm::restart()
{
    stop();
    start();
}

void Alarm::tick()
{
    const milliseconds left = remaining();
    reportTimeLeft(ceil<seconds>(left));

    if (m_state == State::Running && left <= 0ms) {
        m_ticker.stop();
        m_remainingAtResume = 0ms;
        setState(State::Finished);
        emit finished();
    }
}

void Alarm::reportTimeLeft(seconds secondsLeft)
{
    // The ticker runs 4 times a second for accuracy; listeners (label, tray icon,
    // tooltip) only need to hear about a change of the displayed second.
    if (secondsLeft != m_reportedSeconds) {
        m_reportedSeconds = secondsLeft;
        emit timeLeftChanged(secondsLeft);
    }
}

void Alarm::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}
