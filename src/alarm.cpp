#include "alarm.h"

namespace {

// Milliseconds to whole seconds, rounded up (for non-negative values).
qint64 ceilSeconds(qint64 milliseconds)
{
    return milliseconds <= 0 ? 0 : (milliseconds + 999) / 1000;
}

} // namespace

Alarm::Alarm(qint64 durationSeconds, const QString &message, QObject *parent)
    : QObject(parent)
    , m_message(message)
    , m_duration(qMax<qint64>(durationSeconds, 0) * 1000)
    , m_remainingAtResume(m_duration)
    , m_state(NotRunning)
    , m_reportedSeconds(-1)
{
    m_ticker.setInterval(250);
    connect(&m_ticker, SIGNAL(timeout()), this, SLOT(tick()));
}

qint64 Alarm::duration() const
{
    return m_duration / 1000;
}

qint64 Alarm::remaining() const
{
    if (m_state == Running) {
        return qMax<qint64>(m_remainingAtResume - m_sinceResume.elapsed(), 0);
    }
    return m_remainingAtResume;
}

qint64 Alarm::remainingSeconds() const
{
    return ceilSeconds(remaining());
}

void Alarm::start()
{
    if (m_state == Running) {
        return;
    }
    if (m_state == NotRunning || m_state == Finished) {
        m_remainingAtResume = m_duration;
    }
    m_sinceResume.start();
    m_ticker.start();
    setState(Running);
    tick();
}

void Alarm::pause()
{
    if (m_state != Running) {
        return;
    }
    m_remainingAtResume = remaining();
    m_ticker.stop();
    setState(Paused);
    reportTimeLeft(remainingSeconds());
}

void Alarm::stop()
{
    m_ticker.stop();
    m_remainingAtResume = m_duration;
    setState(NotRunning);
    reportTimeLeft(remainingSeconds());
}

void Alarm::restart()
{
    stop();
    start();
}

void Alarm::tick()
{
    const qint64 left = remaining();
    reportTimeLeft(ceilSeconds(left));

    if (m_state == Running && left <= 0) {
        m_ticker.stop();
        m_remainingAtResume = 0;
        setState(Finished);
        emit finished();
    }
}

void Alarm::reportTimeLeft(qint64 secondsLeft)
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
