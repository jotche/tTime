#include "app-settings.h"
#include "testutil.h"

#include <QCoreApplication>
#include <QFile>
#include <QPoint>
#include <QSettings>
#include <QTest>

class TestAppSettings : public QObject
{
    Q_OBJECT

private:
    TemporaryDir m_dir;

    QString path(const char *name) const { return m_dir.filePath(s(name)); }

    static void write(const QString &file, const QString &key, const QVariant &value)
    {
        QSettings ini(file, QSettings::IniFormat);
        ini.setValue(key, value);
    }

    static bool switchValue(const char *text, bool *value)
    {
        return AppSettings::parseSwitch(s(text), value);
    }

private slots:
    void init()
    {
        QVERIFY(m_dir.isValid());
        QFile::remove(path("user.ini"));
        QFile::remove(path("defaults.ini"));
    }

    void userOverridesDefaultsOverridesBuiltIn()
    {
        write(path("defaults.ini"), s("appearance/theme"), s("sage"));
        write(path("defaults.ini"), s("appearance/countdown-weight"), 300);
        write(path("user.ini"), s("appearance/countdown-weight"), 500);

        const AppSettings settings(path("user.ini"), path("defaults.ini"));
        QCOMPARE(settings.value(s("appearance/countdown-weight"), 400).toInt(), 500); // user
        QCOMPARE(settings.value(s("appearance/theme"), s("blue")).toString(), s("sage")); // shipped default
        QCOMPARE(settings.value(s("appearance/countdown-font"), s("oxanium")).toString(),
                 s("oxanium")); // built-in fallback
    }

    void missingFilesUseTheFallback()
    {
        const AppSettings settings(path("user.ini"), path("defaults.ini"));
        QCOMPARE(settings.value(s("appearance/theme"), s("blue")).toString(), s("blue"));
    }

    void writesGoToTheUserFileOnly()
    {
        write(path("defaults.ini"), s("progress-dialog/pos"), QPoint(250, 250));
        {
            AppSettings settings(path("user.ini"), path("defaults.ini"));
            settings.setValue(s("progress-dialog/pos"), QPoint(10, 20));
            QCOMPARE(settings.value(s("progress-dialog/pos")).toPoint(), QPoint(10, 20));
        }
        QCOMPARE(QSettings(path("user.ini"), QSettings::IniFormat).value(s("progress-dialog/pos")).toPoint(),
                 QPoint(10, 20));
        QCOMPARE(QSettings(path("defaults.ini"), QSettings::IniFormat).value(s("progress-dialog/pos")).toPoint(),
                 QPoint(250, 250)); // shipped file untouched
    }

    void parsesSwitches()
    {
        const char *const on[] = {"on", "ON", " true ", "yes", "1"};
        for (int i = 0; i < 5; ++i) {
            bool value = false;
            QVERIFY2(switchValue(on[i], &value), on[i]);
            QCOMPARE(value, true);
        }
        const char *const off[] = {"off", "False", "no", "0"};
        for (int i = 0; i < 4; ++i) {
            bool value = true;
            QVERIFY2(switchValue(off[i], &value), off[i]);
            QCOMPARE(value, false);
        }
        QVERIFY(!AppSettings::parseSwitch(s("maybe")));
        QVERIFY(!AppSettings::parseSwitch(QString()));
    }

    void pathsDoNotDependOnApplicationMetadata()
    {
        // The shipped defaults used to be found only because the organization
        // name happened to be "tTime". Now both paths are fixed.
        QCoreApplication::setOrganizationName(s("Somebody Else"));
        QCoreApplication::setApplicationName(s("other"));

#ifdef Q_OS_MAC
        // Inside the app bundle: Contents/MacOS/tTime -> Contents/Resources/tTime.ini
        const QString shipped = s("/../Resources/tTime.ini");
#else
        const QString shipped = s("/tTime.ini");
#endif
        QCOMPARE(AppSettings::defaultsFilePath(), QCoreApplication::applicationDirPath() + shipped);
        QVERIFY2(AppSettings::userFilePath().endsWith(s("/tTime/tTime.ini")), qPrintable(AppSettings::userFilePath()));
    }
};

QTEST_MAIN(TestAppSettings)
#include "tst_appsettings.moc"
