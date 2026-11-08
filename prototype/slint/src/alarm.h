#pragma once

#include <slint.h>

#include <chrono>
#include <functional>
#include <string>

// A single countdown alarm (the model; windows present it). Time is measured
// with a monotonic clock, so changing the system time does not affect it.
class Alarm
{
public:
    enum class State { NotRunning, Paused, Running, Finished };

    Alarm(std::chrono::seconds duration, std::string message);
    Alarm(const Alarm &) = delete;
    Alarm &operator=(const Alarm &) = delete;

    [[nodiscard]] const std::string &message() const noexcept { return m_message; }
    [[nodiscard]] State state() const noexcept { return m_state; }
    [[nodiscard]] std::chrono::seconds duration() const noexcept;
    [[nodiscard]] std::chrono::milliseconds remaining() const;
    // Remaining time rounded up, so "00:00:00" is only shown once finished.
    [[nodiscard]] std::chrono::seconds remainingSeconds() const;

    void start(); // starts, or resumes when paused
    void pause();
    void stop(); // stops and rewinds to the full duration
    void restart();

    // Called when the displayed (whole) seconds change, not on every internal tick.
    std::function<void(std::chrono::seconds)> onTimeLeftChanged;
    std::function<void(State)> onStateChanged;
    std::function<void()> onFinished;

private:
    using Clock = std::chrono::steady_clock;

    void tick();
    void setState(State state);
    void reportTimeLeft(std::chrono::seconds secondsLeft);

    std::string m_message;
    std::chrono::milliseconds m_duration;
    std::chrono::milliseconds m_remainingAtResume; // remaining time when last (re)started
    Clock::time_point m_resumedAt;
    slint::Timer m_ticker;
    State m_state = State::NotRunning;
    std::chrono::seconds m_reportedSeconds{-1}; // last value sent with onTimeLeftChanged
};
