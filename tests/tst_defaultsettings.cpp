#include "app-settings.h"
#include "countdown-font.h"
#include "font-weight.h"
#include "theme.h"

#include <QPoint>
#include <QSettings>
#include <QSize>
#include <QTest>

// The shipped tTime.ini must parse and agree with the built-in defaults, so the
// documented defaults and the behaviour without the file never drift apart.
class TestDefaultSettings : public QObject
{
    Q_OBJECT

private slots:
    void shippedIniMatchesBuiltInDefaults()
    {
        const QSettings ini(QStringLiteral(TTIME_DEFAULT_INI), QSettings::IniFormat);
        QCOMPARE(ini.status(), QSettings::NoError);

        QCOMPARE(ini.value(QStringLiteral("appearance/countdown-font")).toString(),
                 QString::fromLatin1(defaultCountdownFontId));

        const auto weight = parseFontWeight(ini.value(QStringLiteral("appearance/countdown-weight")).toString());
        QVERIFY(weight.has_value());
        QCOMPARE(*weight, defaultCountdownWeight);

        QCOMPARE(ini.value(QStringLiteral("appearance/theme")).toString(), QString::fromLatin1(defaultThemeName));
        QCOMPARE(AppSettings::parseSwitch(ini.value(QStringLiteral("appearance/animations")).toString()), true);
    }

    void windowDefaultsParse()
    {
        const QSettings ini(QStringLiteral(TTIME_DEFAULT_INI), QSettings::IniFormat);
        QCOMPARE(ini.value(QStringLiteral("progress-dialog/size")).toSize(), QSize(200, 95));
        QCOMPARE(ini.value(QStringLiteral("progress-dialog/pos")).toPoint(), QPoint(250, 250));
    }

    void onlyKnownKeys()
    {
        // A typo in the shipped file would silently do nothing.
        const QSettings ini(QStringLiteral(TTIME_DEFAULT_INI), QSettings::IniFormat);
        const QStringList known = {
            QStringLiteral("appearance/countdown-font"), QStringLiteral("appearance/countdown-weight"),
            QStringLiteral("appearance/theme"),          QStringLiteral("appearance/animations"),
            QStringLiteral("progress-dialog/size"),      QStringLiteral("progress-dialog/pos"),
        };
        for (const QString &key : ini.allKeys()) {
            QVERIFY2(known.contains(key), qPrintable(QStringLiteral("unexpected key: ") + key));
        }
    }
};

QTEST_GUILESS_MAIN(TestDefaultSettings)
#include "tst_defaultsettings.moc"
