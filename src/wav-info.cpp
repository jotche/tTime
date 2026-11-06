#include "wav-info.h"

#include <QFile>
#include <QtEndian>

#include <algorithm>
#include <cmath>

using namespace std::chrono;

namespace {

bool readExact(QIODevice &device, char *buffer, qint64 size)
{
    return device.read(buffer, size) == size;
}

} // namespace

std::optional<milliseconds> wavDuration(QIODevice &device)
{
    char riff[12];
    if (!readExact(device, riff, sizeof riff) || qstrncmp(riff, "RIFF", 4) != 0 || qstrncmp(riff + 8, "WAVE", 4) != 0) {
        return std::nullopt;
    }

    quint32 byteRate = 0;
    char header[8];
    while (readExact(device, header, sizeof header)) {
        const auto chunkSize = qFromLittleEndian<quint32>(header + 4);

        if (qstrncmp(header, "fmt ", 4) == 0) {
            char fmt[16];
            if (chunkSize < sizeof fmt || !readExact(device, fmt, sizeof fmt)) {
                return std::nullopt;
            }
            byteRate = qFromLittleEndian<quint32>(fmt + 8);
            // Skip any extension bytes (+ pad byte for odd sizes).
            if (device.skip(static_cast<qint64>(chunkSize - sizeof fmt) + (chunkSize & 1U)) < 0) {
                return std::nullopt;
            }
        } else if (qstrncmp(header, "data", 4) == 0) {
            if (byteRate == 0) {
                return std::nullopt; // "fmt " must come before "data"
            }
            return milliseconds{static_cast<qint64>(chunkSize) * 1000 / byteRate};
        } else if (device.skip(static_cast<qint64>(chunkSize) + (chunkSize & 1U)) < 0) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<milliseconds> wavDuration(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    return wavDuration(file);
}

int loopsForDuration(milliseconds clip, milliseconds target)
{
    if (clip <= 0ms) {
        return 1;
    }
    const double loops = std::round(static_cast<double>(target.count()) / static_cast<double>(clip.count()));
    return std::max(1, static_cast<int>(loops));
}
