#pragma once

#include <QString>
#include <QVariant>

#include <memory>
#include <optional>

class QSettings;

// tTime's settings, read from two INI files:
//
//   1. the user's file      %APPDATA%\tTime\tTime.ini   (read/write)
//   2. the shipped defaults <exe dir>\tTime.ini          (read-only)
//
// value() returns the first file that has the key, else the given fallback (the
// built-in default). setValue() only ever writes the user's file. Both paths are
// explicit, so the lookup does not depend on QCoreApplication's organization or
// application name.
class AppSettings
{
public:
    AppSettings(const QString &userFile, const QString &defaultsFile);
    ~AppSettings();

    AppSettings(AppSettings &&) noexcept;
    AppSettings &operator=(AppSettings &&) noexcept;
    AppSettings(const AppSettings &) = delete;
    AppSettings &operator=(const AppSettings &) = delete;

    // The settings of the running application (the two files above).
    [[nodiscard]] static AppSettings forApplication();
    [[nodiscard]] static QString userFilePath();
    [[nodiscard]] static QString defaultsFilePath();

    [[nodiscard]] QVariant value(const QString &key, const QVariant &fallback = {}) const;

    // Parses an on/off setting: on, off, true, false, yes, no, 1, 0 (any case).
    [[nodiscard]] static std::optional<bool> parseSwitch(const QString &text);
    void setValue(const QString &key, const QVariant &value);

private:
    std::unique_ptr<QSettings> m_user;
    std::unique_ptr<QSettings> m_defaults;
};
