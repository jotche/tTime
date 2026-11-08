//! A single countdown alarm (port of alarm.cpp), without timers or signals:
//! the GUI calls [`Alarm::tick`] about 4 times a second and polls the state.
//! Time is measured with the monotonic [`Instant`], so changing the system time
//! does not affect it.

use std::time::{Duration, Instant};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AlarmState {
    NotRunning,
    Paused,
    Running,
    Finished,
}

#[derive(Debug, Clone)]
pub struct Alarm {
    message: String,
    duration: Duration,
    remaining_at_resume: Duration, // remaining time when last (re)started
    resumed_at: Instant,
    state: AlarmState,
    finish_unreported: bool, // finished inside start_at(); reported by the next tick
}

/// Whole milliseconds, like `QElapsedTimer::elapsed()`.
fn whole_millis(d: Duration) -> Duration {
    Duration::from_millis(d.as_millis() as u64)
}

impl Alarm {
    pub fn new(duration: Duration, message: String) -> Self {
        let duration = whole_millis(duration);
        Self {
            message,
            duration,
            remaining_at_resume: duration,
            resumed_at: Instant::now(),
            state: AlarmState::NotRunning,
            finish_unreported: false,
        }
    }

    pub fn message(&self) -> &str {
        &self.message
    }

    pub fn state(&self) -> AlarmState {
        self.state
    }

    /// The full duration in whole seconds.
    pub fn duration(&self) -> Duration {
        Duration::from_secs(self.duration.as_secs())
    }

    pub fn remaining(&self) -> Duration {
        self.remaining_at(Instant::now())
    }

    pub fn remaining_at(&self, now: Instant) -> Duration {
        if self.state == AlarmState::Running {
            let elapsed = whole_millis(now.saturating_duration_since(self.resumed_at));
            return self.remaining_at_resume.saturating_sub(elapsed);
        }
        self.remaining_at_resume
    }

    /// Remaining time rounded up, so "00:00:00" is only shown once finished.
    pub fn remaining_seconds(&self) -> u64 {
        self.remaining_seconds_at(Instant::now())
    }

    pub fn remaining_seconds_at(&self, now: Instant) -> u64 {
        self.remaining_at(now).as_millis().div_ceil(1000) as u64
    }

    /// Starts, or resumes when paused.
    pub fn start(&mut self) {
        self.start_at(Instant::now());
    }

    pub fn start_at(&mut self, now: Instant) {
        if self.state == AlarmState::Running {
            return;
        }
        if matches!(self.state, AlarmState::NotRunning | AlarmState::Finished) {
            self.remaining_at_resume = self.duration;
        }
        self.resumed_at = now;
        self.finish_unreported = false;
        self.state = AlarmState::Running;
        // The C++ ticks once right away; a zero-length alarm finishes here and
        // the next tick() reports it.
        if self.finish_if_due(now) {
            self.finish_unreported = true;
        }
    }

    pub fn pause(&mut self) {
        self.pause_at(Instant::now());
    }

    pub fn pause_at(&mut self, now: Instant) {
        if self.state != AlarmState::Running {
            return;
        }
        self.remaining_at_resume = self.remaining_at(now);
        self.state = AlarmState::Paused;
    }

    /// Stops and rewinds to the full duration.
    pub fn stop(&mut self) {
        self.remaining_at_resume = self.duration;
        self.finish_unreported = false;
        self.state = AlarmState::NotRunning;
    }

    pub fn restart(&mut self) {
        self.restart_at(Instant::now());
    }

    pub fn restart_at(&mut self, now: Instant) {
        self.stop();
        self.start_at(now);
    }

    /// Call about 4 times a second. Returns true exactly once, when the alarm
    /// has just finished.
    pub fn tick(&mut self) -> bool {
        self.tick_at(Instant::now())
    }

    pub fn tick_at(&mut self, now: Instant) -> bool {
        if std::mem::take(&mut self.finish_unreported) {
            return true;
        }
        self.finish_if_due(now)
    }

    fn finish_if_due(&mut self, now: Instant) -> bool {
        if self.state == AlarmState::Running && self.remaining_at(now).is_zero() {
            self.remaining_at_resume = Duration::ZERO;
            self.state = AlarmState::Finished;
            return true;
        }
        false
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn ms(n: u64) -> Duration {
        Duration::from_millis(n)
    }

    #[test]
    fn initial_state() {
        let alarm = Alarm::new(Duration::from_secs(5), "tea".into());
        assert_eq!(alarm.state(), AlarmState::NotRunning);
        assert_eq!(alarm.remaining_seconds(), 5);
        assert_eq!(alarm.message(), "tea");
        assert_eq!(alarm.duration(), Duration::from_secs(5));
    }

    #[test]
    fn finishes_after_duration() {
        let t0 = Instant::now();
        let mut alarm = Alarm::new(Duration::from_secs(1), String::new());
        alarm.start_at(t0);
        assert_eq!(alarm.state(), AlarmState::Running);
        let mut finished = 0;
        for i in 0..12 {
            if alarm.tick_at(t0 + ms(250 * i)) {
                finished += 1;
            }
        }
        assert_eq!(finished, 1);
        assert_eq!(alarm.state(), AlarmState::Finished);
        assert_eq!(alarm.remaining_seconds_at(t0 + ms(3000)), 0);
    }

    #[test]
    fn remaining_seconds_round_up() {
        let t0 = Instant::now();
        let mut alarm = Alarm::new(Duration::from_secs(10), String::new());
        alarm.start_at(t0);
        assert_eq!(alarm.remaining_seconds_at(t0 + ms(1)), 10);
        assert_eq!(alarm.remaining_seconds_at(t0 + ms(1000)), 9);
        assert_eq!(alarm.remaining_seconds_at(t0 + ms(9999)), 1);
        assert!(!alarm.tick_at(t0 + ms(9999)));
        assert!(alarm.tick_at(t0 + ms(10_000)));
        assert!(!alarm.tick_at(t0 + ms(10_250)));
    }

    #[test]
    fn pause_freezes_remaining_time() {
        let t0 = Instant::now();
        let mut alarm = Alarm::new(Duration::from_secs(10), String::new());
        alarm.start_at(t0);
        alarm.pause_at(t0 + ms(300));
        let remaining = alarm.remaining_at(t0 + ms(300));
        assert_eq!(remaining, ms(9700));
        assert_eq!(alarm.remaining_at(t0 + ms(600)), remaining);
        assert_eq!(alarm.state(), AlarmState::Paused);
        // resuming continues from where it was paused
        alarm.start_at(t0 + ms(5000));
        assert_eq!(alarm.remaining_at(t0 + ms(5700)), ms(9000));
    }

    #[test]
    fn stop_rewinds() {
        let t0 = Instant::now();
        let mut alarm = Alarm::new(Duration::from_secs(10), String::new());
        alarm.start_at(t0);
        let _ = alarm.tick_at(t0 + ms(300));
        alarm.stop();
        assert_eq!(alarm.state(), AlarmState::NotRunning);
        assert_eq!(alarm.remaining(), ms(10_000));
    }

    #[test]
    fn restart_rewinds_and_runs() {
        let t0 = Instant::now();
        let mut alarm = Alarm::new(Duration::from_secs(10), String::new());
        alarm.start_at(t0);
        alarm.restart_at(t0 + ms(300));
        assert_eq!(alarm.state(), AlarmState::Running);
        assert!(alarm.remaining_at(t0 + ms(300)) > ms(9800));
    }

    #[test]
    fn start_after_finish_runs_again() {
        let t0 = Instant::now();
        let mut alarm = Alarm::new(Duration::from_secs(1), String::new());
        alarm.start_at(t0);
        assert!(alarm.tick_at(t0 + ms(1000)));
        alarm.start_at(t0 + ms(2000));
        assert_eq!(alarm.state(), AlarmState::Running);
        assert_eq!(alarm.remaining_at(t0 + ms(2000)), ms(1000));
    }

    #[test]
    fn zero_duration_finishes_on_start_and_reports_once() {
        let t0 = Instant::now();
        let mut alarm = Alarm::new(Duration::ZERO, String::new());
        alarm.start_at(t0);
        assert_eq!(alarm.state(), AlarmState::Finished);
        assert!(alarm.tick_at(t0));
        assert!(!alarm.tick_at(t0 + ms(250)));
    }
}
