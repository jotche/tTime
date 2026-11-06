#include "alarm.h"

#include <QSignalSpy>
#include <QTest>

using namespace std::chrono_literals;

class TestAlarm : public QObject
{
    Q_OBJECT

private slots:
    void initialState()
    {
        const Alarm alarm(5s, QStringLiteral("tea"));
        QCOMPARE(alarm.state(), Alarm::State::NotRunning);
        QCOMPARE(alarm.remainingSeconds().count(), 5);
        QCOMPARE(alarm.message(), QStringLiteral("tea"));
    }

    void finishesAfterDuration()
    {
        Alarm alarm(1s, QString());
        QSignalSpy finished(&alarm, &Alarm::finished);

        alarm.start();
        QCOMPARE(alarm.state(), Alarm::State::Running);
        QVERIFY(finished.wait(3000));
        QCOMPARE(finished.count(), 1);
        QCOMPARE(alarm.state(), Alarm::State::Finished);
        QCOMPARE(alarm.remainingSeconds().count(), 0);
    }

    void pauseFreezesRemainingTime()
    {
        Alarm alarm(10s, QString());
        alarm.start();
        QTest::qWait(300);
        alarm.pause();
        const auto remaining = alarm.remaining();
        QVERIFY(remaining < 10s);
        QTest::qWait(300);
        QCOMPARE(alarm.remaining().count(), remaining.count());
        QCOMPARE(alarm.state(), Alarm::State::Paused);
    }

    void stopRewinds()
    {
        Alarm alarm(10s, QString());
        alarm.start();
        QTest::qWait(300);
        alarm.stop();
        QCOMPARE(alarm.state(), Alarm::State::NotRunning);
        QCOMPARE(alarm.remaining().count(), std::chrono::milliseconds{10s}.count());
    }

    void reportsOnlyWhenTheDisplayedSecondChanges()
    {
        Alarm alarm(10s, QString());
        QSignalSpy timeLeft(&alarm, &Alarm::timeLeftChanged);
        alarm.start();
        QTest::qWait(1600); // ~6 internal ticks, but only 1-2 second changes
        QVERIFY2(timeLeft.count() >= 1 && timeLeft.count() <= 3, qPrintable(QString::number(timeLeft.count())));
        for (int i = 1; i < timeLeft.count(); ++i) {
            QVERIFY(timeLeft.at(i).first().value<std::chrono::seconds>()
                    != timeLeft.at(i - 1).first().value<std::chrono::seconds>());
        }
    }

    void restartRewindsAndRuns()
    {
        Alarm alarm(10s, QString());
        alarm.start();
        QTest::qWait(300);
        alarm.restart();
        QCOMPARE(alarm.state(), Alarm::State::Running);
        QVERIFY(alarm.remaining() > 9800ms);
    }
};

QTEST_GUILESS_MAIN(TestAlarm)
#include "tst_alarm.moc"
