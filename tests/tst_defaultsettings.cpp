#include "app-settings.h"
#include "countdown-font.h"
#include "font-weight.h"
#include "testutil.h"
#include "theme.h"

#include <QPoint>
#include <QSettings>
#include <QSize>
#include <QTest>

namespace {

QString shippedIni()
{
    return s(TTIME_SOURCE_DIR "/src/tTime.ini");
}

} // namespace

// The shipped tTime.ini must parse and agree with the built-in defaults, so the
// documented defaults and the behaviour without the file never drift apart.
class TestDefaultSettings : public QObject
{
    Q_OBJECT

private slots:
    void shippedIniMatchesBuiltInDefaults()
    {
        const QSettings ini(shippedIni(), QSettings::IniFormat);
        QCOMPARE(ini.status(), QSettings::NoError);

        QCOMPARE(ini.value(s("appearance/countdown-font")).toString(), s(defaultCountdownFontId));

        int weight = 0;
        QVERIFY(parseFontWeight(ini.value(s("appearance/countdown-weight")).toString(), &weight));
        QCOMPARE(weight, defaultCountdownWeight);

        QCOMPARE(ini.value(s("appearance/message-font")).toString(), s(defaultMessageFontId));
        QVERIFY(parseFontWeight(ini.value(s("appearance/message-weight")).toString(), &weight));
        QCOMPARE(weight, defaultMessageWeight);

        int size = 0;
        QVERIFY(parsePointSize(ini.value(s("appearance/countdown-size")).toString(), &size));
        QCOMPARE(size, defaultCountdownPointSize);
        QVERIFY(parsePointSize(ini.value(s("appearance/message-size")).toString(), &size));
        QCOMPARE(size, defaultMessagePointSize);
        QVERIFY(parsePointSize(ini.value(s("appearance/time-up-message-size")).toString(), &size));
        QCOMPARE(size, defaultTimeUpMessagePointSize);

        QCOMPARE(ini.value(s("appearance/theme")).toString(), s(defaultThemeName));
        bool animations = false;
        QVERIFY(AppSettings::parseSwitch(ini.value(s("appearance/animations")).toString(), &animations));
        QCOMPARE(animations, true);
    }

    void windowDefaultsParse()
    {
        const QSettings ini(shippedIni(), QSettings::IniFormat);
        QCOMPARE(ini.value(s("progress-dialog/size")).toSize(), QSize(200, 130));
        QCOMPARE(ini.value(s("progress-dialog/pos")).toPoint(), QPoint(250, 250));
    }

    void onlyKnownKeys()
    {
        // A typo in the shipped file would silently do nothing.
        const QSettings ini(shippedIni(), QSettings::IniFormat);
        const QStringList known = QStringList()
            << s("appearance/countdown-font") << s("appearance/countdown-weight") << s("appearance/message-font")
            << s("appearance/message-weight") << s("appearance/countdown-size") << s("appearance/message-size")
            << s("appearance/time-up-message-size") << s("appearance/theme") << s("appearance/animations")
            << s("progress-dialog/size") << s("progress-dialog/pos");
        foreach (const QString &key, ini.allKeys()) {
            QVERIFY2(known.contains(key), qPrintable(s("unexpected key: ") + key));
        }
    }
};

QTEST_MAIN(TestDefaultSettings)
#include "tst_defaultsettings.moc"
