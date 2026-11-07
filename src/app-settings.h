#pragma once

#include <QScopedPointer>
#include <QString>
#include <QVariant>

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

    // The two files of the running application (see above).
    static QString userFilePath();
    static QString defaultsFilePath();

    QVariant value(const QString &key, const QVariant &fallback = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

    // Parses an on/off setting: on, off, true, false, yes, no, 1, 0 (any case).
    // Returns false if `text` is none of them; `value` may be 0.
    static bool parseSwitch(const QString &text, bool *value = 0);

private:
    Q_DISABLE_COPY(AppSettings)

    QScopedPointer<QSettings> m_user;
    QScopedPointer<QSettings> m_defaults;
};
