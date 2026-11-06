#include "alarm-spec.h"

#include <QTest>

using namespace std::chrono_literals;

Q_DECLARE_METATYPE(std::chrono::seconds)

class TestAlarmSpec : public QObject
{
    Q_OBJECT

private slots:
    void parsesValidSpecs_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<std::chrono::seconds>("duration");
        QTest::addColumn<QString>("message");

        QTest::newRow("seconds") << "90s" << std::chrono::seconds{90} << QString();
        QTest::newRow("minutes") << "5m" << std::chrono::seconds{5min} << QString();
        QTest::newRow("hours") << "2h" << std::chrono::seconds{2h} << QString();
        QTest::newRow("all units") << "1h 2m 3s" << std::chrono::seconds{3723} << QString();
        QTest::newRow("no spaces") << "1h30m" << std::chrono::seconds{90min} << QString();
        QTest::newRow("message") << "5m green tea" << std::chrono::seconds{5min} << "green tea";
        QTest::newRow("short message") << "5m go" << std::chrono::seconds{5min} << "go";
        QTest::newRow("min spelling + message") << "3min tea" << std::chrono::seconds{3min} << "tea";
        QTest::newRow("long spellings") << "1 hour 2 minutes 3 seconds" << std::chrono::seconds{3723} << QString();
        QTest::newRow("sec spelling") << "45 sec eggs" << std::chrono::seconds{45} << "eggs";
        QTest::newRow("case insensitive") << "10M Pasta" << std::chrono::seconds{10min} << "Pasta";
        QTest::newRow("surrounding whitespace") << "  4m   rooibos  " << std::chrono::seconds{4min} << "rooibos";
        QTest::newRow("message starting with s") << "5m sencha" << std::chrono::seconds{5min} << "sencha";
    }

    void parsesValidSpecs()
    {
        QFETCH(QString, input);
        QFETCH(std::chrono::seconds, duration);
        QFETCH(QString, message);

        const auto spec = parseAlarmSpec(input);
        QVERIFY2(spec.has_value(), qPrintable(input));
        QCOMPARE(spec->duration.count(), duration.count());
        QCOMPARE(spec->message, message);
    }

    void rejectsInvalidSpecs_data()
    {
        QTest::addColumn<QString>("input");
        QTest::newRow("empty") << QString();
        QTest::newRow("blank") << "   ";
        QTest::newRow("no unit") << "10";
        QTest::newRow("message only") << "green tea";
        QTest::newRow("zero") << "0s";
        QTest::newRow("zero all") << "0h 0m 0s tea";
        QTest::newRow("unit glued to text") << "5messages";
    }

    void rejectsInvalidSpecs()
    {
        QFETCH(QString, input);
        QVERIFY(!parseAlarmSpec(input).has_value());
    }

    void formatsDurations()
    {
        QCOMPARE(formatDuration(0s), QStringLiteral("00:00:00"));
        QCOMPARE(formatDuration(59s), QStringLiteral("00:00:59"));
        QCOMPARE(formatDuration(3723s), QStringLiteral("01:02:03"));
        QCOMPARE(formatDuration(std::chrono::seconds{100h}), QStringLiteral("100:00:00"));
        QCOMPARE(formatDuration(-5s), QStringLiteral("00:00:00"));
    }
};

QTEST_GUILESS_MAIN(TestAlarmSpec)
#include "tst_alarmspec.moc"
