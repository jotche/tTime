#include "app-config.h"
#include "testutil.h"
#include "wav-info.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QFile>
#include <QTest>
#include <QtEndian>

namespace {

QByteArray le32(quint32 v)
{
    QByteArray b(4, '\0');
    qToLittleEndian(v, reinterpret_cast<uchar *>(b.data()));
    return b;
}

QByteArray le16(quint16 v)
{
    QByteArray b(2, '\0');
    qToLittleEndian(v, reinterpret_cast<uchar *>(b.data()));
    return b;
}

// Builds a minimal PCM WAV: optional extra chunk before "fmt ", then "data".
QByteArray makeWav(quint32 sampleRate, quint16 channels, quint16 bitsPerSample, quint32 dataBytes,
                   bool extraChunk = false)
{
    QByteArray body = "WAVE";
    if (extraChunk) {
        body += "LIST" + le32(3) + QByteArray(3, 'x') + QByteArray(1, '\0'); // odd size + pad
    }
    const quint16 blockAlign = static_cast<quint16>(channels * bitsPerSample / 8);
    body += "fmt " + le32(16) + le16(1) + le16(channels) + le32(sampleRate) + le32(sampleRate * blockAlign)
        + le16(blockAlign) + le16(bitsPerSample);
    body += "data" + le32(dataBytes) + QByteArray(static_cast<int>(dataBytes), '\0');
    return "RIFF" + le32(static_cast<quint32>(body.size())) + body;
}

// The duration in ms, or -1 if the data is not a readable WAV.
qint64 durationOf(QByteArray data)
{
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);
    qint64 ms = -1;
    return wavDuration(buffer, &ms) ? ms : -1;
}

} // namespace

class TestWavInfo : public QObject
{
    Q_OBJECT

private slots:
    void readsDuration()
    {
        // 1 s of 8 kHz mono 16-bit = 16000 bytes
        QCOMPARE(durationOf(makeWav(8000, 1, 16, 16000)), qint64(1000));
        // 0.5 s of 8 kHz stereo 16-bit
        QCOMPARE(durationOf(makeWav(8000, 2, 16, 16000)), qint64(500));
    }

    void skipsUnknownChunks() { QCOMPARE(durationOf(makeWav(8000, 1, 16, 8000, true)), qint64(500)); }

    void rejectsGarbage()
    {
        QCOMPARE(durationOf("not a wav file at all"), qint64(-1));
        QCOMPARE(durationOf(QByteArray()), qint64(-1));
        QVERIFY(!wavDuration(s("does-not-exist.wav"), 0));
    }

    void readsBundledAlarmSound()
    {
        qint64 duration = 0;
        QVERIFY(wavDuration(s(TTIME_SOURCE_DIR "/src/Resources/sounds/submarine1.wav"), &duration));
        QCOMPARE(duration, qint64(2667)); // 117620 frames @ 44.1 kHz
        QCOMPARE(loopsForDuration(duration, 7000), 3);
    }

    void alarmSoundIsInTheResourcesFolder()
    {
        // QSound plays files only: the sound ships in Resources next to the exe
        // (installed by "make install"), not in the compiled-in resources.
        const QString expected
            = QCoreApplication::applicationDirPath() + s("/Resources/sounds/submarine1.wav");
        QCOMPARE(externalResourcePath(s("sounds/submarine1.wav")), expected);
        QCOMPARE(AppConfig::defaults().alarmSound, expected);
        QVERIFY(!QFile::exists(s(":/sounds/submarine")));
    }

    void loopCountRoundsToWholeLoops()
    {
        QCOMPARE(loopsForDuration(2667, 7000), 3); // 2.62 -> 3 (8.0 s)
        QCOMPARE(loopsForDuration(3000, 7000), 2); // 2.33 -> 2 (6.0 s)
        QCOMPARE(loopsForDuration(2000, 7000), 4); // 3.5  -> 4 (8.0 s)
        QCOMPARE(loopsForDuration(10000, 7000), 1); // longer than target: play once
        QCOMPARE(loopsForDuration(20000, 7000), 1); // never zero loops
        QCOMPARE(loopsForDuration(0, 7000), 1); // degenerate clip
    }
};

QTEST_MAIN(TestWavInfo)
#include "tst_wavinfo.moc"
