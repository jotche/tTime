#include "alarm-spec.h"

#include <QRegExp>

namespace {

QRegExp alarmSpecPattern()
{
    // Longer unit spellings are listed first so that e.g. "5min tea" is not
    // read as "5m" followed by the garbage "in tea".
    return QRegExp(QLatin1String("^\\s*"
                                 "(?:(\\d{1,6})\\s*(?:hours?|hrs?|h)\\s*)?"
                                 "(?:(\\d{1,6})\\s*(?:minutes?|mins?|m)\\s*)?"
                                 "(?:(\\d{1,9})\\s*(?:seconds?|secs?|s))?"
                                 "(?:\\s+(.*))?\\s*$"),
                   Qt::CaseInsensitive, QRegExp::RegExp2);
}

} // namespace

bool parseAlarmSpec(const QString &text, AlarmSpec *spec)
{
    QRegExp pattern = alarmSpecPattern(); // a copy: QRegExp keeps the captures
    if (!pattern.exactMatch(text)) {
        return false;
    }

    AlarmSpec parsed;
    parsed.duration = pattern.cap(1).toLongLong() * 3600 + pattern.cap(2).toLongLong() * 60
        + pattern.cap(3).toLongLong();
    parsed.message = pattern.cap(4).trimmed();

    if (parsed.duration <= 0) {
        return false;
    }
    if (spec) {
        *spec = parsed;
    }
    return true;
}

QString formatDuration(qint64 seconds)
{
    const qint64 total = qMax<qint64>(0, seconds);
    const qint64 h = total / 3600;
    const qint64 m = (total % 3600) / 60;
    const qint64 s = total % 60;
    return QString::fromLatin1("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}
