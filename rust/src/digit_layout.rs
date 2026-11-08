//! Horizontal layout for text whose digits must not move when they change
//! (a countdown); port of digit-layout.cpp.

/// Every ASCII digit gets a slot as wide as the widest digit and is centred in
/// it; other characters keep their natural width.
#[derive(Debug, Clone, PartialEq, Default)]
pub struct DigitLayout {
    /// Left edge to draw each character at.
    pub glyph_x: Vec<f32>,
    /// Total width, without spacing after the last character.
    pub width: f32,
}

/// `advance` returns a character's natural width (without letter spacing).
/// Letter spacing is applied after each character like QFont does it:
/// percentage of the (slot) width (`spacing_factor`, 1.0 = none) plus a fixed
/// amount in pixels (`absolute_spacing`).
pub fn layout_fixed_digits(
    text: &str,
    advance: impl Fn(char) -> f32,
    spacing_factor: f32,
    absolute_spacing: f32,
) -> DigitLayout {
    let digit_slot = ('0'..='9').map(&advance).fold(0.0_f32, f32::max);

    let mut layout = DigitLayout::default();
    let mut x = 0.0_f32;
    let mut last_spacing = 0.0_f32;
    for c in text.chars() {
        let natural = advance(c);
        let slot = if c.is_ascii_digit() { digit_slot } else { natural };
        layout.glyph_x.push(x + (slot - natural) / 2.0);

        last_spacing = slot * (spacing_factor - 1.0) + absolute_spacing;
        x += slot + last_spacing;
    }
    layout.width = if text.is_empty() { 0.0 } else { x - last_spacing };
    layout
}

#[cfg(test)]
mod tests {
    use super::*;

    // Proportional digits like Jura: '1' narrow, '0' wide, ':' narrow.
    fn proportional(c: char) -> f32 {
        match c {
            '1' => 4.0,
            '7' => 7.0,
            '0' => 10.0,
            ':' => 3.0,
            _ => 8.0,
        }
    }

    #[test]
    fn digits_use_widest_slot() {
        let layout = layout_fixed_digits("10", proportional, 1.0, 0.0);
        assert_eq!(layout.glyph_x, vec![3.0, 10.0]);
        assert_eq!(layout.width, 20.0);
    }

    #[test]
    fn width_independent_of_digits() {
        let a = layout_fixed_digits("01:19:58", proportional, 1.0, 0.0);
        let b = layout_fixed_digits("01:11:11", proportional, 1.0, 0.0);
        let c = layout_fixed_digits("00:00:00", proportional, 1.0, 0.0);
        assert_eq!(a.width, b.width);
        assert_eq!(a.width, c.width);
        assert_eq!(a.glyph_x[2], c.glyph_x[2]);
        assert_eq!(a.glyph_x[5], c.glyph_x[5]);
        assert_eq!(b.glyph_x[5], c.glyph_x[5]);
    }

    #[test]
    fn non_digits_keep_natural_width() {
        let layout = layout_fixed_digits("0:0", proportional, 1.0, 0.0);
        assert_eq!(layout.glyph_x, vec![0.0, 10.0, 13.0]);
        assert_eq!(layout.width, 23.0);
    }

    #[test]
    fn applies_percentage_spacing_between_characters() {
        let layout = layout_fixed_digits("00", proportional, 1.25, 0.0);
        assert_eq!(layout.glyph_x, vec![0.0, 12.5]);
        assert_eq!(layout.width, 22.5);
    }

    #[test]
    fn applies_absolute_spacing() {
        let layout = layout_fixed_digits("0:0", proportional, 1.0, 2.0);
        assert_eq!(layout.glyph_x, vec![0.0, 12.0, 17.0]);
        assert_eq!(layout.width, 27.0);
    }

    #[test]
    fn monospaced_font_is_unchanged() {
        let layout = layout_fixed_digits("12:34", |_| 6.0, 1.0, 0.0);
        assert_eq!(layout.glyph_x, vec![0.0, 6.0, 12.0, 18.0, 24.0]);
        assert_eq!(layout.width, 30.0);
    }

    #[test]
    fn empty_text() {
        let layout = layout_fixed_digits("", proportional, 1.0, 0.0);
        assert!(layout.glyph_x.is_empty());
        assert_eq!(layout.width, 0.0);
    }
}
