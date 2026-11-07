#include "alarm-spec.h"
#include "testutil.h"

#include <QTest>

class TestAlarmSpec : public QObject
{
    Q_OBJECT

private slots:
    void parsesValidSpecs_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<qint64>("duration");
        QTest::addColumn<QString>("message");

        QTest::newRow("seconds") << s("90s") << qint64(90) << QString();
        QTest::newRow("minutes") << s("5m") << qint64(300) << QString();
        QTest::newRow("hours") << s("2h") << qint64(7200) << QString();
        QTest::newRow("all units") << s("1h 2m 3s") << qint64(3723) << QString();
        QTest::newRow("no spaces") << s("1h30m") << qint64(5400) << QString();
        QTest::newRow("message") << s("5m green tea") << qint64(300) << s("green tea");
        QTest::newRow("short message") << s("5m go") << qint64(300) << s("go");
        QTest::newRow("min spelling + message") << s("3min tea") << qint64(180) << s("tea");
        QTest::newRow("long spellings") << s("1 hour 2 minutes 3 seconds") << qint64(3723) << QString();
        QTest::newRow("sec spelling") << s("45 sec eggs") << qint64(45) << s("eggs");
        QTest::newRow("case insensitive") << s("10M Pasta") << qint64(600) << s("Pasta");
        QTest::newRow("surrounding whitespace") << s("  4m   rooibos  ") << qint64(240) << s("rooibos");
        QTest::newRow("message starting with s") << s("5m sencha") << qint64(300) << s("sencha");
    }

    void parsesValidSpecs()
    {
        QFETCH(QString, input);
        QFETCH(qint64, duration);
        QFETCH(QString, message);

        AlarmSpec spec;
        QVERIFY2(parseAlarmSpec(input, &spec), qPrintable(input));
        QCOMPARE(spec.duration, duration);
        QCOMPARE(spec.message, message);
    }

    void rejectsInvalidSpecs_data()
    {
        QTest::addColumn<QString>("input");
        QTest::newRow("empty") << QString();
        QTest::newRow("blank") << s("   ");
        QTest::newRow("no unit") << s("10");
        QTest::newRow("message only") << s("green tea");
        QTest::newRow("zero") << s("0s");
        QTest::newRow("zero all") << s("0h 0m 0s tea");
        QTest::newRow("unit glued to text") << s("5messages");
    }

    void rejectsInvalidSpecs()
    {
        QFETCH(QString, input);
        QVERIFY(!parseAlarmSpec(input));
    }

    void formatsDurations()
    {
        QCOMPARE(formatDuration(0), s("00:00:00"));
        QCOMPARE(formatDuration(59), s("00:00:59"));
        QCOMPARE(formatDuration(3723), s("01:02:03"));
        QCOMPARE(formatDuration(100 * 3600), s("100:00:00"));
        QCOMPARE(formatDuration(-5), s("00:00:00"));
    }
};

QTEST_APPLESS_MAIN(TestAlarmSpec)
#include "tst_alarmspec.moc"
