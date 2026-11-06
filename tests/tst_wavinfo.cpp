#include "wav-info.h"

#include <QBuffer>
#include <QTest>
#include <QtEndian>

using namespace std::chrono_literals;

namespace {

// Builds a minimal PCM WAV: optional extra chunk before "fmt ", then "data".
QByteArray makeWav(quint32 sampleRate, quint16 channels, quint16 bitsPerSample, quint32 dataBytes,
                   bool extraChunk = false)
{
    auto le32 = [](quint32 v) {
        QByteArray b(4, '\0');
        qToLittleEndian(v, b.data());
        return b;
    };
    auto le16 = [](quint16 v) {
        QByteArray b(2, '\0');
        qToLittleEndian(v, b.data());
        return b;
    };

    QByteArray body = "WAVE";
    if (extraChunk) {
        body += "LIST" + le32(3) + QByteArray(3, 'x') + QByteArray(1, '\0'); // odd size + pad
    }
    const auto blockAlign = static_cast<quint16>(channels * bitsPerSample / 8);
    body += "fmt " + le32(16) + le16(1) + le16(channels) + le32(sampleRate) + le32(sampleRate * blockAlign)
        + le16(blockAlign) + le16(bitsPerSample);
    body += "data" + le32(dataBytes) + QByteArray(static_cast<qsizetype>(dataBytes), '\0');
    return "RIFF" + le32(static_cast<quint32>(body.size())) + body;
}

std::optional<std::chrono::milliseconds> durationOf(QByteArray data)
{
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);
    return wavDuration(buffer);
}

} // namespace

class TestWavInfo : public QObject
{
    Q_OBJECT

private slots:
    void readsDuration()
    {
        // 1 s of 8 kHz mono 16-bit = 16000 bytes
        QCOMPARE(durationOf(makeWav(8000, 1, 16, 16000)).value().count(), 1000);
        // 0.5 s of 8 kHz stereo 16-bit
        QCOMPARE(durationOf(makeWav(8000, 2, 16, 16000)).value().count(), 500);
    }

    void skipsUnknownChunks() { QCOMPARE(durationOf(makeWav(8000, 1, 16, 8000, true)).value().count(), 500); }

    void rejectsGarbage()
    {
        QVERIFY(!durationOf("not a wav file at all").has_value());
        QVERIFY(!durationOf(QByteArray()).has_value());
        QVERIFY(!wavDuration(QStringLiteral("does-not-exist.wav")).has_value());
    }

    void readsBundledAlarmSound()
    {
        const auto duration = wavDuration(QStringLiteral(TTIME_ALARM_WAV));
        QVERIFY(duration.has_value());
        QCOMPARE(duration->count(), 2667); // 117620 frames @ 44.1 kHz
        QCOMPARE(loopsForDuration(*duration, 7s), 3);
    }

    void loopCountRoundsToWholeLoops()
    {
        QCOMPARE(loopsForDuration(2667ms, 7s), 3); // 2.62 -> 3 (8.0 s)
        QCOMPARE(loopsForDuration(3000ms, 7s), 2); // 2.33 -> 2 (6.0 s)
        QCOMPARE(loopsForDuration(2000ms, 7s), 4); // 3.5  -> 4 (8.0 s)
        QCOMPARE(loopsForDuration(10s, 7s), 1); // longer than target: play once
        QCOMPARE(loopsForDuration(20s, 7s), 1); // never zero loops
        QCOMPARE(loopsForDuration(0ms, 7s), 1); // degenerate clip
    }
};

QTEST_GUILESS_MAIN(TestWavInfo)
#include "tst_wavinfo.moc"
