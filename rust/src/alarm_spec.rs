//! Parsing of alarm requests such as "1h 20m green tea" (port of alarm-spec.cpp).

use std::sync::LazyLock;
use std::time::Duration;

use regex::Regex;

/// A parsed alarm request, e.g. "1h 20m green tea".
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AlarmSpec {
    pub duration: Duration,
    pub message: String,
}

// Same pattern as the C++ (PCRE without Unicode properties): `\d` and `\s` are
// ASCII-only; case-insensitive; `.` matches newlines. Longer unit spellings are
// listed first so that e.g. "5min tea" is not read as "5m" followed by the
// garbage "in tea".
static PATTERN: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(concat!(
        r"(?is)^(?-u:\s)*",
        r"(?:((?-u:\d){1,6})(?-u:\s)*(?:hours?|hrs?|h)(?-u:\s)*)?",
        r"(?:((?-u:\d){1,6})(?-u:\s)*(?:minutes?|mins?|m)(?-u:\s)*)?",
        r"(?:((?-u:\d){1,9})(?-u:\s)*(?:seconds?|secs?|s))?",
        r"(?:(?-u:\s)+(.*?))?(?-u:\s)*$",
    ))
    .expect("valid alarm spec pattern")
});

/// Parses an alarm description of the form
/// `[<n>h] [<n>m|min] [<n>s|sec] [message]`.
/// Units are case-insensitive, whitespace between parts is optional
/// ("1h30m", "1h 30m", "90 sec tea"). Returns `None` when the text does not
/// start with a valid, strictly positive duration.
pub fn parse_alarm_spec(text: &str) -> Option<AlarmSpec> {
    let caps = PATTERN.captures(text)?;
    let number = |i: usize| -> u64 { caps.get(i).and_then(|m| m.as_str().parse().ok()).unwrap_or(0) };
    let secs = number(1) * 3600 + number(2) * 60 + number(3);
    if secs == 0 {
        return None;
    }
    Some(AlarmSpec {
        duration: Duration::from_secs(secs),
        message: caps.get(4).map(|m| m.as_str().to_owned()).unwrap_or_default(),
    })
}

/// Formats a number of seconds as "hh:mm:ss" (hours are not wrapped at 24).
pub fn format_duration(secs: u64) -> String {
    format!("{:02}:{:02}:{:02}", secs / 3600, (secs % 3600) / 60, secs % 60)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_valid_specs() {
        let cases: &[(&str, u64, &str)] = &[
            ("90s", 90, ""),
            ("5m", 300, ""),
            ("2h", 7200, ""),
            ("1h 2m 3s", 3723, ""),
            ("1h30m", 5400, ""),
            ("5m green tea", 300, "green tea"),
            ("5m go", 300, "go"),
            ("3min tea", 180, "tea"),
            ("1 hour 2 minutes 3 seconds", 3723, ""),
            ("45 sec eggs", 45, "eggs"),
            ("10M Pasta", 600, "Pasta"),
            ("  4m   rooibos  ", 240, "rooibos"),
            ("5m sencha", 300, "sencha"),
        ];
        for &(input, secs, message) in cases {
            let spec = parse_alarm_spec(input).unwrap_or_else(|| panic!("{input}"));
            assert_eq!(spec.duration, Duration::from_secs(secs), "{input}");
            assert_eq!(spec.message, message, "{input}");
        }
    }

    #[test]
    fn rejects_invalid_specs() {
        for input in ["", "   ", "10", "green tea", "0s", "0h 0m 0s tea", "5messages"] {
            assert!(parse_alarm_spec(input).is_none(), "{input}");
        }
    }

    #[test]
    fn digit_limits() {
        assert!(parse_alarm_spec("1234567h").is_none());
        assert_eq!(parse_alarm_spec("999999999s").unwrap().duration, Duration::from_secs(999_999_999));
    }

    #[test]
    fn formats_durations() {
        assert_eq!(format_duration(0), "00:00:00");
        assert_eq!(format_duration(59), "00:00:59");
        assert_eq!(format_duration(3723), "01:02:03");
        assert_eq!(format_duration(100 * 3600), "100:00:00");
    }
}
