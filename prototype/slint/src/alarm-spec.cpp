#include "alarm-spec.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <string_view>

using namespace std::chrono;

// The grammar of the Qt version's regular expression
//
//     ^\s* (?:(\d{1,6})\s*(?:hours?|hrs?|h)\s*)? (?:(\d{1,6})\s*(?:minutes?|mins?|m)\s*)?
//          (?:(\d{1,9})\s*(?:seconds?|secs?|s))? (?:\s+(.*?))? \s*$          (case-insensitive)
//
// parsed by hand with the same backtracking (std::regex engines differ in how they
// backtrack through the optional groups).

namespace {

struct Unit
{
    std::size_t maxDigits;
    long long seconds;
    std::array<std::string_view, 5> spellings; // longer spellings first, like the regex
};

constexpr Unit units[] = {
    {6, 3600, {"hours", "hour", "hrs", "hr", "h"}},
    {6, 60, {"minutes", "minute", "mins", "min", "m"}},
    {9, 1, {"seconds", "second", "secs", "sec", "s"}},
};

bool isSpace(char c)
{
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

std::size_t skipSpace(std::string_view text, std::size_t pos)
{
    while (pos < text.size() && isSpace(text[pos])) {
        ++pos;
    }
    return pos;
}

bool startsWithNoCase(std::string_view text, std::size_t pos, std::string_view word)
{
    if (text.size() - pos < word.size()) {
        return false;
    }
    for (std::size_t i = 0; i < word.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(text[pos + i])) != word[i]) {
            return false;
        }
    }
    return true;
}

// The message part at `pos` (just after the last unit): nothing but whitespace, or
// whitespace followed by the message (trimmed).
std::optional<std::string> parseMessage(std::string_view text, std::size_t pos)
{
    const std::size_t start = skipSpace(text, pos);
    if (start == text.size()) {
        return std::string();
    }
    if (start == pos) {
        return std::nullopt; // text directly after the unit, e.g. "5messages"
    }
    std::size_t end = text.size();
    while (end > start && isSpace(text[end - 1])) {
        --end;
    }
    return std::string(text.substr(start, end - start));
}

// Unit groups from `index` on, starting at `pos`; `end` is where the previous group's
// unit ended (before its trailing whitespace).
std::optional<AlarmSpec> parseFrom(std::string_view text, std::size_t index, std::size_t pos, std::size_t end,
                                   long long total)
{
    if (index == std::size(units)) {
        const auto message = parseMessage(text, end);
        if (!message) {
            return std::nullopt;
        }
        return AlarmSpec{seconds{total}, *message};
    }

    // With this group: digits, optional whitespace, a unit spelling.
    const Unit &unit = units[index];
    std::size_t digitsEnd = pos;
    while (digitsEnd < text.size() && isDigit(text[digitsEnd])) {
        ++digitsEnd;
    }
    const std::size_t digits = digitsEnd - pos;
    if (digits >= 1 && digits <= unit.maxDigits) {
        const long long number = std::stoll(std::string(text.substr(pos, digits)));
        const std::size_t unitStart = skipSpace(text, digitsEnd);
        for (const std::string_view spelling : unit.spellings) {
            if (!startsWithNoCase(text, unitStart, spelling)) {
                continue;
            }
            const std::size_t unitEnd = unitStart + spelling.size();
            // Hours and minutes swallow the whitespace after them; seconds do not.
            const std::size_t next = index + 1 < std::size(units) ? skipSpace(text, unitEnd) : unitEnd;
            if (auto spec = parseFrom(text, index + 1, next, unitEnd, total + number * unit.seconds)) {
                return spec;
            }
        }
    }
    // Without this group.
    return parseFrom(text, index + 1, pos, end, total);
}

} // namespace

std::optional<AlarmSpec> parseAlarmSpec(const std::string &text)
{
    const std::size_t start = skipSpace(text, 0);
    auto spec = parseFrom(text, 0, start, start, 0);
    if (!spec || spec->duration <= 0s) {
        return std::nullopt;
    }
    return spec;
}

std::string formatDuration(seconds duration)
{
    const long long total = std::max<long long>(0, duration.count());
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%02lld:%02lld:%02lld", total / 3600, (total % 3600) / 60, total % 60);
    return buffer;
}
