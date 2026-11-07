#include "wav-info.h"

#include <QFile>
#include <QtEndian>

#include <cmath>

namespace {

bool readExact(QIODevice &device, char *buffer, qint64 size)
{
    return device.read(buffer, size) == size;
}

// Skips `size` bytes (QIODevice::skip() is not available in Qt 4).
bool skip(QIODevice &device, qint64 size)
{
    if (!device.isSequential()) {
        return device.seek(device.pos() + size);
    }
    char buffer[4096];
    while (size > 0) {
        const qint64 chunk = qMin<qint64>(size, sizeof buffer);
        if (!readExact(device, buffer, chunk)) {
            return false;
        }
        size -= chunk;
    }
    return true;
}

quint32 littleEndian32(const char *data)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(data));
}

} // namespace

bool wavDuration(QIODevice &device, qint64 *milliseconds)
{
    char riff[12];
    if (!readExact(device, riff, sizeof riff) || qstrncmp(riff, "RIFF", 4) != 0 || qstrncmp(riff + 8, "WAVE", 4) != 0) {
        return false;
    }

    quint32 byteRate = 0;
    char header[8];
    while (readExact(device, header, sizeof header)) {
        const quint32 chunkSize = littleEndian32(header + 4);

        if (qstrncmp(header, "fmt ", 4) == 0) {
            char fmt[16];
            if (chunkSize < sizeof fmt || !readExact(device, fmt, sizeof fmt)) {
                return false;
            }
            byteRate = littleEndian32(fmt + 8);
            // Skip any extension bytes (+ pad byte for odd sizes).
            if (!skip(device, static_cast<qint64>(chunkSize - sizeof fmt) + (chunkSize & 1U))) {
                return false;
            }
        } else if (qstrncmp(header, "data", 4) == 0) {
            if (byteRate == 0) {
                return false; // "fmt " must come before "data"
            }
            if (milliseconds) {
                *milliseconds = static_cast<qint64>(chunkSize) * 1000 / byteRate;
            }
            return true;
        } else if (!skip(device, static_cast<qint64>(chunkSize) + (chunkSize & 1U))) {
            return false;
        }
    }
    return false;
}

bool wavDuration(const QString &fileName, qint64 *milliseconds)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    return wavDuration(file, milliseconds);
}

int loopsForDuration(qint64 clipMs, qint64 targetMs)
{
    if (clipMs <= 0) {
        return 1;
    }
    const double loops = std::floor(static_cast<double>(targetMs) / static_cast<double>(clipMs) + 0.5);
    return qMax(1, static_cast<int>(loops));
}
