#include "font-weight.h"
#include "testutil.h"

#include <QTest>

class TestFontWeight : public QObject
{
    Q_OBJECT

private slots:
    void parses_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<int>("weight");
        QTest::newRow("number") << s("300") << 300;
        QTest::newRow("in-between number") << s("350") << 350;
        QTest::newRow("whitespace") << s("  700 ") << 700;
        QTest::newRow("light") << s("light") << 300;
        QTest::newRow("case") << s("Light") << 300;
        QTest::newRow("regular") << s("regular") << 400;
        QTest::newRow("normal") << s("normal") << 400;
        QTest::newRow("semibold") << s("semibold") << 600;
        QTest::newRow("semi-bold") << s("semi-bold") << 600;
        QTest::newRow("Semi Bold") << s("Semi Bold") << 600;
        QTest::newRow("demibold") << s("DemiBold") << 600;
        QTest::newRow("extra_light") << s("extra_light") << 200;
        QTest::newRow("black") << s("black") << 900;
        QTest::newRow("lowest") << s("1") << 1;
        QTest::newRow("highest") << s("1000") << 1000;
    }

    void parses()
    {
        QFETCH(QString, input);
        QFETCH(int, weight);
        int parsed = 0;
        QVERIFY2(parseFontWeight(input, &parsed), qPrintable(input));
        QCOMPARE(parsed, weight);
    }

    void rejects_data()
    {
        QTest::addColumn<QString>("input");
        QTest::newRow("empty") << QString();
        QTest::newRow("zero") << s("0");
        QTest::newRow("negative") << s("-300");
        QTest::newRow("too heavy") << s("1001");
        QTest::newRow("unknown name") << s("chunky");
        QTest::newRow("unit") << s("300px");
    }

    void rejects()
    {
        QFETCH(QString, input);
        QVERIFY(!parseFontWeight(input));
    }
};

QTEST_APPLESS_MAIN(TestFontWeight)
#include "tst_fontweight.moc"
