#include "app-settings.h"

#include <QCoreApplication>
#include <QSettings>

namespace {

// Fixed on purpose (not QCoreApplication::organizationName()), so the user's
// settings stay where they are even if the application metadata changes.
const auto settingsOrganization = QStringLiteral("tTime");
const auto settingsApplication = QStringLiteral("tTime");

} // namespace

AppSettings::AppSettings(const QString &userFile, const QString &defaultsFile)
    : m_user(std::make_unique<QSettings>(userFile, QSettings::IniFormat))
    , m_defaults(std::make_unique<QSettings>(defaultsFile, QSettings::IniFormat))
{
    m_user->setFallbacksEnabled(false);
    m_defaults->setFallbacksEnabled(false);
}

AppSettings::~AppSettings() = default;
AppSettings::AppSettings(AppSettings &&) noexcept = default;
AppSettings &AppSettings::operator=(AppSettings &&) noexcept = default;

AppSettings AppSettings::forApplication()
{
    return {userFilePath(), defaultsFilePath()};
}

QString AppSettings::userFilePath()
{
    // %APPDATA%\tTime\tTime.ini on Windows (the location tTime has always used).
    return QSettings(QSettings::IniFormat, QSettings::UserScope, settingsOrganization, settingsApplication).fileName();
}

QString AppSettings::defaultsFilePath()
{
#ifdef Q_OS_MACOS
    // tTime.app/Contents/MacOS/tTime -> tTime.app/Contents/Resources/tTime.ini
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources/tTime.ini");
#else
    return QCoreApplication::applicationDirPath() + QStringLiteral("/tTime.ini");
#endif
}

QVariant AppSettings::value(const QString &key, const QVariant &fallback) const
{
    if (m_user->contains(key)) {
        return m_user->value(key);
    }
    if (m_defaults->contains(key)) {
        return m_defaults->value(key);
    }
    return fallback;
}

std::optional<bool> AppSettings::parseSwitch(const QString &text)
{
    const QString key = text.trimmed().toLower();
    if (key == QLatin1String("on") || key == QLatin1String("true") || key == QLatin1String("yes")
        || key == QLatin1String("1")) {
        return true;
    }
    if (key == QLatin1String("off") || key == QLatin1String("false") || key == QLatin1String("no")
        || key == QLatin1String("0")) {
        return false;
    }
    return std::nullopt;
}

void AppSettings::setValue(const QString &key, const QVariant &value)
{
    m_user->setValue(key, value);
}
