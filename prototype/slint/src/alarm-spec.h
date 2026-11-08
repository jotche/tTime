#pragma once

#include <chrono>
#include <optional>
#include <string>

// A parsed alarm request, e.g. "1h 20m green tea".
struct AlarmSpec
{
    std::chrono::seconds duration{0};
    std::string message; // UTF-8

    friend bool operator==(const AlarmSpec &, const AlarmSpec &) = default;
};

// Parses an alarm description of the form
//     [<n>h] [<n>m|min] [<n>s|sec] [message]
// Units are case-insensitive, whitespace between parts is optional
// ("1h30m", "1h 30m", "90 sec tea"). Returns std::nullopt when the text does
// not start with a valid, strictly positive duration.
[[nodiscard]] std::optional<AlarmSpec> parseAlarmSpec(const std::string &text);

// Formats a number of seconds as "hh:mm:ss" (hours are not wrapped at 24).
[[nodiscard]] std::string formatDuration(std::chrono::seconds duration);
