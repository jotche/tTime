#pragma once

#include <QString>

class QIODevice;

// Playback length of a PCM WAV stream in milliseconds, read from its RIFF header.
// Returns false if the data is not a readable WAV file; `milliseconds` may be 0.
bool wavDuration(QIODevice &device, qint64 *milliseconds);
bool wavDuration(const QString &fileName, qint64 *milliseconds);

// Number of times a clip of length `clipMs` must be played so the total is as
// close as possible to `targetMs`, always playing complete loops (at least one).
int loopsForDuration(qint64 clipMs, qint64 targetMs);
