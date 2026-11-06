#include "alarm-spec.h"

#include <QRegularExpression>

using namespace std::chrono;

namespace {

const QRegularExpression &alarmSpecPattern()
{
    // Longer unit spellings are listed first so that e.g. "5min tea" is not
    // read as "5m" followed by the garbage "in tea".
    static const QRegularExpression pattern(QStringLiteral(R"(^\s*)"
                                                           R"((?:(\d{1,6})\s*(?:hours?|hrs?|h)\s*)?)"
                                                           R"((?:(\d{1,6})\s*(?:minutes?|mins?|m)\s*)?)"
                                                           R"((?:(\d{1,9})\s*(?:seconds?|secs?|s))?)"
                                                           R"((?:\s+(.*?))?\s*$)"),
                                            QRegularExpression::CaseInsensitiveOption
                                                | QRegularExpression::DotMatchesEverythingOption);
    return pattern;
}

} // namespace

std::optional<AlarmSpec> parseAlarmSpec(const QString &text)
{
    const QRegularExpressionMatch match = alarmSpecPattern().match(text);
    if (!match.hasMatch()) {
        return std::nullopt;
    }

    const auto h = hours{match.capturedView(1).toLongLong()};
    const auto m = minutes{match.capturedView(2).toLongLong()};
    const auto s = seconds{match.capturedView(3).toLongLong()};

    AlarmSpec spec;
    spec.duration = h + m + s;
    spec.message = match.captured(4);

    if (spec.duration <= 0s) {
        return std::nullopt;
    }
    return spec;
}

QString formatDuration(seconds duration)
{
    const auto total = std::max<qint64>(0, duration.count());
    const qint64 h = total / 3600;
    const qint64 m = (total % 3600) / 60;
    const qint64 s = total % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}
