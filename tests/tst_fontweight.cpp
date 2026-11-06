#include "font-weight.h"

#include <QTest>

class TestFontWeight : public QObject
{
    Q_OBJECT

private slots:
    void parses_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<int>("weight");
        QTest::newRow("number") << "300" << 300;
        QTest::newRow("in-between number") << "350" << 350;
        QTest::newRow("whitespace") << "  700 " << 700;
        QTest::newRow("light") << "light" << 300;
        QTest::newRow("case") << "Light" << 300;
        QTest::newRow("regular") << "regular" << 400;
        QTest::newRow("normal") << "normal" << 400;
        QTest::newRow("semibold") << "semibold" << 600;
        QTest::newRow("semi-bold") << "semi-bold" << 600;
        QTest::newRow("Semi Bold") << "Semi Bold" << 600;
        QTest::newRow("demibold") << "DemiBold" << 600;
        QTest::newRow("extra_light") << "extra_light" << 200;
        QTest::newRow("black") << "black" << 900;
        QTest::newRow("lowest") << "1" << 1;
        QTest::newRow("highest") << "1000" << 1000;
    }

    void parses()
    {
        QFETCH(QString, input);
        QFETCH(int, weight);
        const auto parsed = parseFontWeight(input);
        QVERIFY2(parsed.has_value(), qPrintable(input));
        QCOMPARE(*parsed, weight);
    }

    void rejects_data()
    {
        QTest::addColumn<QString>("input");
        QTest::newRow("empty") << QString();
        QTest::newRow("zero") << "0";
        QTest::newRow("negative") << "-300";
        QTest::newRow("too heavy") << "1001";
        QTest::newRow("unknown name") << "chunky";
        QTest::newRow("unit") << "300px";
    }

    void rejects()
    {
        QFETCH(QString, input);
        QVERIFY(!parseFontWeight(input).has_value());
    }
};

QTEST_GUILESS_MAIN(TestFontWeight)
#include "tst_fontweight.moc"
