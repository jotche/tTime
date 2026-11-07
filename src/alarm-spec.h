#pragma once

#include <QString>

// A parsed alarm request, e.g. "1h 20m green tea".
struct AlarmSpec
{
    AlarmSpec()
        : duration(0)
    {
    }

    qint64 duration; // seconds
    QString message;

    bool operator==(const AlarmSpec &other) const
    {
        return duration == other.duration && message == other.message;
    }
    bool operator!=(const AlarmSpec &other) const { return !(*this == other); }
};

// Parses an alarm description of the form
//     [<n>h] [<n>m|min] [<n>s|sec] [message]
// Units are case-insensitive, whitespace between parts is optional
// ("1h30m", "1h 30m", "90 sec tea"). Returns false (and leaves `spec` alone) when
// the text does not start with a valid, strictly positive duration. `spec` may be 0.
bool parseAlarmSpec(const QString &text, AlarmSpec *spec = 0);

// Formats a number of seconds as "hh:mm:ss" (hours are not wrapped at 24).
QString formatDuration(qint64 seconds);
