#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>

// A single countdown alarm (the model; windows present it). Time is measured
// with a monotonic clock, so changing the system time does not affect it.
class Alarm : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)

public:
    enum State { NotRunning, Paused, Running, Finished };

    // `durationSeconds` is the length of the countdown in seconds.
    Alarm(qint64 durationSeconds, const QString &message, QObject *parent = 0);

    const QString &message() const { return m_message; }
    State state() const { return m_state; }
    qint64 duration() const; // seconds
    qint64 remaining() const; // milliseconds
    // Remaining time in seconds, rounded up, so "00:00:00" is only shown once finished.
    qint64 remainingSeconds() const;

public slots:
    void start(); // starts, or resumes when paused
    void pause();
    void stop(); // stops and rewinds to the full duration
    void restart();

signals:
    // Emitted when the displayed (whole) seconds change, not on every internal tick.
    void timeLeftChanged(qint64 secondsLeft);
    void stateChanged(Alarm::State state);
    void finished();

private slots:
    void tick();

private:
    void setState(State state);
    void reportTimeLeft(qint64 secondsLeft);

    QString m_message;
    qint64 m_duration; // milliseconds
    qint64 m_remainingAtResume; // milliseconds left when last (re)started
    QElapsedTimer m_sinceResume;
    QTimer m_ticker;
    State m_state;
    qint64 m_reportedSeconds; // last value sent with timeLeftChanged
};
