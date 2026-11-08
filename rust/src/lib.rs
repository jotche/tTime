//! GUI-free core of tTime: alarm model, parsing, settings, themes and image
//! processing. The gpui front end lives in `main.rs` / `gui`.

pub mod alarm;
pub mod alarm_spec;
pub mod bell_animation;
pub mod config;
pub mod digit_layout;
pub mod font_weight;
pub mod image_layers;
pub mod settings;
pub mod theme;
pub mod tree_sway;
pub mod wav_info;
pub mod window_placement;
pub mod window_slot;
