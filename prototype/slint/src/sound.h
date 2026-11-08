#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>

// Playback length of a PCM WAV file in memory, read from its RIFF header.
[[nodiscard]] std::optional<std::chrono::milliseconds> wavDuration(std::span<const std::uint8_t> data);

// Number of complete loops of `clip` whose total is closest to `target` (at least one).
[[nodiscard]] int loopsForDuration(std::chrono::milliseconds clip, std::chrono::milliseconds target);

// Plays the alarm sound compiled into tTime (submarine1.wav) for about `target`,
// in complete loops. Stops when destroyed.
class AlarmSound
{
public:
    explicit AlarmSound(std::chrono::milliseconds target);
    ~AlarmSound();
    AlarmSound(const AlarmSound &) = delete;
    AlarmSound &operator=(const AlarmSound &) = delete;

    void stop();

private:
    struct Private;
    Private *d;
};
