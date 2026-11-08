//! Font weight setting values (port of font-weight.cpp).

/// Parses a font weight: a number 1-1000, or a name such as "light",
/// "semi-bold" or "Extra Bold" (case-insensitive; "-", " " and "_" ignored).
pub fn parse_font_weight(text: &str) -> Option<i32> {
    let key = text.trim().to_lowercase();
    if let Ok(number) = key.parse::<i32>() {
        return (1..=1000).contains(&number).then_some(number);
    }
    let key: String = key.chars().filter(|c| !matches!(c, '-' | ' ' | '_')).collect();
    const NAMES: &[(&str, i32)] = &[
        ("thin", 100),
        ("hairline", 100),
        ("extralight", 200),
        ("ultralight", 200),
        ("light", 300),
        ("regular", 400),
        ("normal", 400),
        ("book", 400),
        ("medium", 500),
        ("semibold", 600),
        ("demibold", 600),
        ("bold", 700),
        ("extrabold", 800),
        ("ultrabold", 800),
        ("black", 900),
        ("heavy", 900),
    ];
    NAMES.iter().find(|(name, _)| *name == key).map(|&(_, weight)| weight)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses() {
        let cases: &[(&str, i32)] = &[
            ("300", 300),
            ("350", 350),
            ("  700 ", 700),
            ("light", 300),
            ("Light", 300),
            ("regular", 400),
            ("normal", 400),
            ("semibold", 600),
            ("semi-bold", 600),
            ("Semi Bold", 600),
            ("DemiBold", 600),
            ("extra_light", 200),
            ("black", 900),
            ("1", 1),
            ("1000", 1000),
        ];
        for &(input, weight) in cases {
            assert_eq!(parse_font_weight(input), Some(weight), "{input}");
        }
    }

    #[test]
    fn rejects() {
        for input in ["", "0", "-300", "1001", "chunky", "300px"] {
            assert_eq!(parse_font_weight(input), None, "{input}");
        }
    }
}
