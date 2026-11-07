#include "alarm.h"
#include "testutil.h"

#include <QSignalSpy>
#include <QTest>

class TestAlarm : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // QSignalSpy records the arguments of timeLeftChanged(qint64).
        qRegisterMetaType<qint64>("qint64");
    }

    void initialState()
    {
        const Alarm alarm(5, s("tea"));
        QCOMPARE(alarm.state(), Alarm::NotRunning);
        QCOMPARE(alarm.remainingSeconds(), qint64(5));
        QCOMPARE(alarm.message(), s("tea"));
    }

    void finishesAfterDuration()
    {
        Alarm alarm(1, QString());
        QSignalSpy finished(&alarm, SIGNAL(finished()));

        alarm.start();
        QCOMPARE(alarm.state(), Alarm::Running);
        QVERIFY(waitForSignal(finished, 3000));
        QCOMPARE(finished.count(), 1);
        QCOMPARE(alarm.state(), Alarm::Finished);
        QCOMPARE(alarm.remainingSeconds(), qint64(0));
    }

    void pauseFreezesRemainingTime()
    {
        Alarm alarm(10, QString());
        alarm.start();
        QTest::qWait(300);
        alarm.pause();
        const qint64 remaining = alarm.remaining();
        QVERIFY(remaining < 10000);
        QTest::qWait(300);
        QCOMPARE(alarm.remaining(), remaining);
        QCOMPARE(alarm.state(), Alarm::Paused);
    }

    void stopRewinds()
    {
        Alarm alarm(10, QString());
        alarm.start();
        QTest::qWait(300);
        alarm.stop();
        QCOMPARE(alarm.state(), Alarm::NotRunning);
        QCOMPARE(alarm.remaining(), qint64(10000));
    }

    void reportsOnlyWhenTheDisplayedSecondChanges()
    {
        Alarm alarm(10, QString());
        QSignalSpy timeLeft(&alarm, SIGNAL(timeLeftChanged(qint64)));
        alarm.start();
        QTest::qWait(1600); // ~6 internal ticks, but only 1-2 second changes
        QVERIFY2(timeLeft.count() >= 1 && timeLeft.count() <= 3, qPrintable(QString::number(timeLeft.count())));
        for (int i = 1; i < timeLeft.count(); ++i) {
            QVERIFY(timeLeft.at(i).first().toLongLong() != timeLeft.at(i - 1).first().toLongLong());
        }
    }

    void restartRewindsAndRuns()
    {
        Alarm alarm(10, QString());
        alarm.start();
        QTest::qWait(300);
        alarm.restart();
        QCOMPARE(alarm.state(), Alarm::Running);
        QVERIFY(alarm.remaining() > 9800);
    }
};

QTEST_MAIN(TestAlarm)
#include "tst_alarm.moc"
