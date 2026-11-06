#include "app-settings.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

class TestAppSettings : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString path(const char *name) const { return m_dir.filePath(QString::fromLatin1(name)); }

    static void write(const QString &file, const QString &key, const QVariant &value)
    {
        QSettings ini(file, QSettings::IniFormat);
        ini.setValue(key, value);
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
        write(path("defaults.ini"), QStringLiteral("appearance/theme"), QStringLiteral("sage"));
        write(path("defaults.ini"), QStringLiteral("appearance/countdown-weight"), 300);
        write(path("user.ini"), QStringLiteral("appearance/countdown-weight"), 500);

        const AppSettings settings(path("user.ini"), path("defaults.ini"));
        QCOMPARE(settings.value(QStringLiteral("appearance/countdown-weight"), 400).toInt(), 500); // user
        QCOMPARE(settings.value(QStringLiteral("appearance/theme"), QStringLiteral("blue")).toString(),
                 QStringLiteral("sage")); // shipped default
        QCOMPARE(settings.value(QStringLiteral("appearance/countdown-font"), QStringLiteral("oxanium")).toString(),
                 QStringLiteral("oxanium")); // built-in fallback
    }

    void missingFilesUseTheFallback()
    {
        const AppSettings settings(path("user.ini"), path("defaults.ini"));
        QCOMPARE(settings.value(QStringLiteral("appearance/theme"), QStringLiteral("blue")).toString(),
                 QStringLiteral("blue"));
    }

    void writesGoToTheUserFileOnly()
    {
        write(path("defaults.ini"), QStringLiteral("progress-dialog/pos"), QPoint(250, 250));
        {
            AppSettings settings(path("user.ini"), path("defaults.ini"));
            settings.setValue(QStringLiteral("progress-dialog/pos"), QPoint(10, 20));
            QCOMPARE(settings.value(QStringLiteral("progress-dialog/pos")).toPoint(), QPoint(10, 20));
        }
        QCOMPARE(
            QSettings(path("user.ini"), QSettings::IniFormat).value(QStringLiteral("progress-dialog/pos")).toPoint(),
            QPoint(10, 20));
        QCOMPARE(QSettings(path("defaults.ini"), QSettings::IniFormat)
                     .value(QStringLiteral("progress-dialog/pos"))
                     .toPoint(),
                 QPoint(250, 250)); // shipped file untouched
    }

    void parsesSwitches()
    {
        for (const auto *on : {"on", "ON", " true ", "yes", "1"}) {
            QCOMPARE(AppSettings::parseSwitch(QString::fromLatin1(on)), true);
        }
        for (const auto *off : {"off", "False", "no", "0"}) {
            QCOMPARE(AppSettings::parseSwitch(QString::fromLatin1(off)), false);
        }
        QVERIFY(!AppSettings::parseSwitch(QStringLiteral("maybe")).has_value());
        QVERIFY(!AppSettings::parseSwitch(QString()).has_value());
    }

    void pathsDoNotDependOnApplicationMetadata()
    {
        // The shipped defaults used to be found only because the organization
        // name happened to be "tTime". Now both paths are fixed.
        QCoreApplication::setOrganizationName(QStringLiteral("Somebody Else"));
        QCoreApplication::setApplicationName(QStringLiteral("other"));

        QCOMPARE(AppSettings::defaultsFilePath(),
                 QCoreApplication::applicationDirPath() + QStringLiteral("/tTime.ini"));
        QVERIFY2(AppSettings::userFilePath().endsWith(QStringLiteral("/tTime/tTime.ini")),
                 qPrintable(AppSettings::userFilePath()));
    }
};

QTEST_GUILESS_MAIN(TestAppSettings)
#include "tst_appsettings.moc"
