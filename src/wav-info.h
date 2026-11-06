#pragma once

#include <QString>

#include <chrono>
#include <optional>

class QIODevice;

// Playback length of a PCM WAV stream, read from its RIFF header.
// Returns std::nullopt if the data is not a readable WAV file.
[[nodiscard]] std::optional<std::chrono::milliseconds> wavDuration(QIODevice &device);
[[nodiscard]] std::optional<std::chrono::milliseconds> wavDuration(const QString &fileName);

// Number of times a clip of length `clip` must be played so the total is as
// close as possible to `target`, always playing complete loops (at least one).
[[nodiscard]] int loopsForDuration(std::chrono::milliseconds clip, std::chrono::milliseconds target);
