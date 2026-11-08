//! Playback length of a PCM WAV stream, read from its RIFF header (port of wav-info.cpp).

use std::time::Duration;

/// The bundled alarm sound (src/Resources/sounds/submarine1.wav).
pub const ALARM_WAV: &[u8] = include_bytes!("../../src/Resources/sounds/submarine1.wav");

fn le32(bytes: &[u8]) -> u32 {
    u32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]])
}

/// Playback length of a WAV file's bytes; `None` if it is not a readable WAV file.
pub fn wav_duration(bytes: &[u8]) -> Option<Duration> {
    let mut pos = 0usize;
    let read = |n: usize, pos: &mut usize| -> Option<&[u8]> {
        let chunk = bytes.get(*pos..pos.checked_add(n)?)?;
        *pos += n;
        Some(chunk)
    };

    let riff = read(12, &mut pos)?;
    if &riff[0..4] != b"RIFF" || &riff[8..12] != b"WAVE" {
        return None;
    }

    let mut byte_rate = 0u32;
    while let Some(header) = read(8, &mut pos) {
        let id = &header[0..4];
        let chunk_size = le32(&header[4..8]);
        let pad = (chunk_size & 1) as usize;

        if id == b"fmt " {
            if chunk_size < 16 {
                return None;
            }
            let fmt = read(16, &mut pos)?;
            byte_rate = le32(&fmt[8..12]);
            // Skip any extension bytes (+ pad byte for odd sizes).
            pos = pos.saturating_add(chunk_size as usize - 16 + pad);
        } else if id == b"data" {
            if byte_rate == 0 {
                return None; // "fmt " must come before "data"
            }
            return Some(Duration::from_millis(u64::from(chunk_size) * 1000 / u64::from(byte_rate)));
        } else {
            pos = pos.saturating_add(chunk_size as usize + pad);
        }
    }
    None
}

/// Number of times a clip of length `clip` must be played so the total is as
/// close as possible to `target`, always playing complete loops (at least one).
pub fn loops_for_duration(clip: Duration, target: Duration) -> u32 {
    let clip_ms = clip.as_millis();
    if clip_ms == 0 {
        return 1;
    }
    let loops = (target.as_millis() as f64 / clip_ms as f64).round();
    (loops as u32).max(1)
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A minimal PCM WAV: optional extra chunk before "fmt ", then "data".
    fn make_wav(sample_rate: u32, channels: u16, bits: u16, data_bytes: u32, extra_chunk: bool) -> Vec<u8> {
        let mut body = b"WAVE".to_vec();
        if extra_chunk {
            body.extend_from_slice(b"LIST");
            body.extend_from_slice(&3u32.to_le_bytes());
            body.extend_from_slice(b"xxx\0"); // odd size + pad
        }
        let block_align = channels * bits / 8;
        body.extend_from_slice(b"fmt ");
        body.extend_from_slice(&16u32.to_le_bytes());
        body.extend_from_slice(&1u16.to_le_bytes());
        body.extend_from_slice(&channels.to_le_bytes());
        body.extend_from_slice(&sample_rate.to_le_bytes());
        body.extend_from_slice(&(sample_rate * u32::from(block_align)).to_le_bytes());
        body.extend_from_slice(&block_align.to_le_bytes());
        body.extend_from_slice(&bits.to_le_bytes());
        body.extend_from_slice(b"data");
        body.extend_from_slice(&data_bytes.to_le_bytes());
        body.resize(body.len() + data_bytes as usize, 0);
        let mut wav = b"RIFF".to_vec();
        wav.extend_from_slice(&(body.len() as u32).to_le_bytes());
        wav.extend(body);
        wav
    }

    fn ms(n: u64) -> Duration {
        Duration::from_millis(n)
    }

    #[test]
    fn reads_duration() {
        assert_eq!(wav_duration(&make_wav(8000, 1, 16, 16000, false)), Some(ms(1000)));
        assert_eq!(wav_duration(&make_wav(8000, 2, 16, 16000, false)), Some(ms(500)));
    }

    #[test]
    fn skips_unknown_chunks() {
        assert_eq!(wav_duration(&make_wav(8000, 1, 16, 8000, true)), Some(ms(500)));
    }

    #[test]
    fn rejects_garbage() {
        assert_eq!(wav_duration(b"not a wav file at all"), None);
        assert_eq!(wav_duration(b""), None);
        // "data" before "fmt "
        assert_eq!(wav_duration(b"RIFF\x0c\0\0\0WAVEdata\0\0\0\0"), None);
        // truncated after the header
        assert_eq!(wav_duration(b"RIFF\x04\0\0\0WAVE"), None);
    }

    #[test]
    fn reads_bundled_alarm_sound() {
        let duration = wav_duration(ALARM_WAV).unwrap();
        assert_eq!(duration, ms(2667)); // 117620 frames @ 44.1 kHz
        assert_eq!(loops_for_duration(duration, Duration::from_secs(7)), 3);
    }

    #[test]
    fn loop_count_rounds_to_whole_loops() {
        let target = Duration::from_secs(7);
        assert_eq!(loops_for_duration(ms(2667), target), 3);
        assert_eq!(loops_for_duration(ms(3000), target), 2);
        assert_eq!(loops_for_duration(ms(2000), target), 4);
        assert_eq!(loops_for_duration(Duration::from_secs(10), target), 1);
        assert_eq!(loops_for_duration(Duration::from_secs(20), target), 1);
        assert_eq!(loops_for_duration(ms(0), target), 1);
    }
}
