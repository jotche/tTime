//! The alarm sound: the bundled WAV, looped for about seven seconds.
//!
//! Playback runs on its own thread (opening the audio device can take a
//! moment); dropping the [`AlarmSound`] or calling [`AlarmSound::stop`] stops it.

use std::io::Cursor;
use std::sync::mpsc::{self, RecvTimeoutError, Sender};
use std::time::Duration;

use ttime::wav_info::{loops_for_duration, wav_duration};

const ALARM_SOUND: &[u8] = include_bytes!("../../../src/Resources/sounds/submarine1.wav");
const TARGET_LENGTH: Duration = Duration::from_secs(7);

pub struct AlarmSound {
    stop: Option<Sender<()>>,
}

impl AlarmSound {
    pub fn play() -> Self {
        let loops = match wav_duration(ALARM_SOUND) {
            Some(clip) => loops_for_duration(clip, TARGET_LENGTH),
            None => {
                log::warn!("Cannot read the length of the alarm sound; playing it once");
                1
            }
        };
        let (stop, stopped) = mpsc::channel::<()>();
        let spawned = std::thread::Builder::new().name("alarm-sound".into()).spawn(move || {
            let mut sink = match rodio::DeviceSinkBuilder::open_default_sink() {
                Ok(sink) => sink,
                Err(error) => {
                    log::warn!("Cannot play alarm sound: {error}");
                    return;
                }
            };
            sink.log_on_drop(false);
            let player = rodio::Player::connect_new(sink.mixer());
            for _ in 0..loops {
                match rodio::Decoder::new(Cursor::new(ALARM_SOUND)) {
                    Ok(source) => player.append(source),
                    Err(error) => {
                        log::warn!("Cannot play alarm sound: {error}");
                        return;
                    }
                }
            }
            loop {
                match stopped.recv_timeout(Duration::from_millis(50)) {
                    Ok(()) | Err(RecvTimeoutError::Disconnected) => {
                        player.stop();
                        break;
                    }
                    Err(RecvTimeoutError::Timeout) if player.empty() => break,
                    Err(RecvTimeoutError::Timeout) => {}
                }
            }
        });
        if let Err(error) = spawned {
            log::warn!("Cannot play alarm sound: {error}");
        }
        Self { stop: Some(stop) }
    }

    pub fn stop(&mut self) {
        if let Some(stop) = self.stop.take() {
            let _ = stop.send(());
        }
    }
}

impl Drop for AlarmSound {
    fn drop(&mut self) {
        self.stop();
    }
}
