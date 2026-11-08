//! Claims the lowest free "cascade slot" shared by all running tTime windows,
//! across processes (port of window-slot.cpp). The slot is held until the
//! [`WindowSlot`] is dropped; if a process dies, the OS releases its lock.

use std::fs::{self, File, OpenOptions};
use std::path::PathBuf;

const MAX_SLOTS: i32 = 64;

#[derive(Debug)]
pub struct WindowSlot {
    index: i32,
    _lock: Option<File>,
}

fn slot_directory() -> PathBuf {
    std::env::temp_dir().join("tTime-window-slots")
}

impl WindowSlot {
    /// Claims the lowest free slot 0..63; falls back to 0 (without holding a
    /// lock) if every slot is taken or the temp directory is unusable.
    pub fn acquire() -> WindowSlot {
        let dir = slot_directory();
        let _ = fs::create_dir_all(&dir);
        for i in 0..MAX_SLOTS {
            let path = dir.join(format!("slot-{i}.lock"));
            let Ok(file) = OpenOptions::new().read(true).write(true).create(true).truncate(false).open(&path) else {
                continue;
            };
            if file.try_lock().is_ok() {
                return WindowSlot { index: i, _lock: Some(file) };
            }
        }
        WindowSlot { index: 0, _lock: None }
    }

    /// 0 for the first window, 1 for the second, ...
    pub fn index(&self) -> i32 {
        self.index
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn slots_are_distinct_and_released() {
        let a = WindowSlot::acquire();
        let b = WindowSlot::acquire();
        assert_ne!(a.index(), b.index());
        assert!((0..MAX_SLOTS).contains(&a.index()) && (0..MAX_SLOTS).contains(&b.index()));
        let freed = b.index();
        drop(b);
        // The freed slot (or a lower one) is available again.
        let c = WindowSlot::acquire();
        assert!(c.index() <= freed, "{} > {freed}", c.index());
        assert_ne!(c.index(), a.index());
    }
}
