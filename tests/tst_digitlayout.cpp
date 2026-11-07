#include "digit-layout.h"
#include "testutil.h"

#include <QTest>

namespace {

// Proportional digits like Jura: '1' narrow, '0' wide, ':' narrow.
qreal proportional(QChar c)
{
    switch (c.unicode()) {
    case '1':
        return 4;
    case '7':
        return 7;
    case '0':
        return 10;
    case ':':
        return 3;
    default:
        return 8;
    }
}

qreal mono(QChar)
{
    return 6;
}

QList<qreal> positions(qreal a, qreal b, qreal c = -1, qreal d = -1, qreal e = -1)
{
    QList<qreal> list;
    list << a << b;
    if (c >= 0) {
        list << c;
    }
    if (d >= 0) {
        list << d;
    }
    if (e >= 0) {
        list << e;
    }
    return list;
}

} // namespace

class TestDigitLayout : public QObject
{
    Q_OBJECT

private slots:
    void digitsUseWidestSlot()
    {
        const DigitLayout layout = layoutFixedDigits(s("10"), proportional);
        // slot = 10 (widest digit '0'); '1' (4 wide) is centred in the first slot
        QCOMPARE(layout.glyphX, positions(3, 10));
        QCOMPARE(layout.width, 20.0);
    }

    void widthIndependentOfDigits()
    {
        // The whole point: changing digits never changes positions or width.
        const DigitLayout a = layoutFixedDigits(s("01:19:58"), proportional);
        const DigitLayout b = layoutFixedDigits(s("01:11:11"), proportional);
        const DigitLayout c = layoutFixedDigits(s("00:00:00"), proportional);
        QCOMPARE(a.width, b.width);
        QCOMPARE(a.width, c.width);
        // the colons sit at the same place in all of them
        QCOMPARE(a.glyphX[2], c.glyphX[2]);
        QCOMPARE(a.glyphX[5], c.glyphX[5]);
        QCOMPARE(b.glyphX[5], c.glyphX[5]);
    }

    void nonDigitsKeepNaturalWidth()
    {
        const DigitLayout layout = layoutFixedDigits(s("0:0"), proportional);
        QCOMPARE(layout.glyphX, positions(0, 10, 13));
        QCOMPARE(layout.width, 23.0);
    }

    void appliesPercentageSpacingBetweenCharacters()
    {
        // 125 % spacing adds 25 % of each slot after it, but not after the last one
        const DigitLayout layout = layoutFixedDigits(s("00"), proportional, 1.25);
        QCOMPARE(layout.glyphX, positions(0, 12.5));
        QCOMPARE(layout.width, 22.5);
    }

    void appliesAbsoluteSpacing()
    {
        const DigitLayout layout = layoutFixedDigits(s("0:0"), proportional, 1.0, 2.0);
        QCOMPARE(layout.glyphX, positions(0, 12, 17));
        QCOMPARE(layout.width, 27.0);
    }

    void monospacedFontIsUnchanged()
    {
        const DigitLayout layout = layoutFixedDigits(s("12:34"), mono);
        QCOMPARE(layout.glyphX, positions(0, 6, 12, 18, 24));
        QCOMPARE(layout.width, 30.0);
    }

    void emptyText()
    {
        const DigitLayout layout = layoutFixedDigits(QString(), proportional);
        QVERIFY(layout.glyphX.isEmpty());
        QCOMPARE(layout.width, 0.0);
    }
};

QTEST_APPLESS_MAIN(TestDigitLayout)
#include "tst_digitlayout.moc"
