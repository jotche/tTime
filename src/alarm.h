#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>

#include <chrono>

// A single countdown alarm (the model; windows present it). Time is measured
// with a monotonic clock, so changing the system time does not affect it.
class Alarm : public QObject
{
    Q_OBJECT

public:
    enum class State { NotRunning, Paused, Running, Finished };
    Q_ENUM(State)

    Alarm(std::chrono::seconds duration, QString message, QObject *parent = nullptr);

    [[nodiscard]] const QString &message() const noexcept { return m_message; }
    [[nodiscard]] State state() const noexcept { return m_state; }
    [[nodiscard]] std::chrono::seconds duration() const noexcept;
    [[nodiscard]] std::chrono::milliseconds remaining() const;
    // Remaining time rounded up, so "00:00:00" is only shown once finished.
    [[nodiscard]] std::chrono::seconds remainingSeconds() const;

public slots:
    void start(); // starts, or resumes when paused
    void pause();
    void stop(); // stops and rewinds to the full duration
    void restart();

signals:
    // Emitted when the displayed (whole) seconds change, not on every internal tick.
    void timeLeftChanged(std::chrono::seconds secondsLeft);
    void stateChanged(Alarm::State state);
    void finished();

private:
    void tick();
    void setState(State state);
    void reportTimeLeft(std::chrono::seconds secondsLeft);

    QString m_message;
    std::chrono::milliseconds m_duration;
    std::chrono::milliseconds m_remainingAtResume; // remaining time when last (re)started
    QElapsedTimer m_sinceResume;
    QTimer m_ticker;
    State m_state = State::NotRunning;
    std::chrono::seconds m_reportedSeconds{-1}; // last value sent with timeLeftChanged
};
