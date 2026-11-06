#pragma once

#include <QLockFile>

#include <memory>

// Claims the lowest free "cascade slot" shared by all running tTime windows,
// across processes. The slot is held until this object is destroyed; if a
// process crashes, its slot is reclaimed automatically (stale lock detection).
class WindowSlot
{
public:
    WindowSlot();
    ~WindowSlot() = default;

    WindowSlot(const WindowSlot &) = delete;
    WindowSlot &operator=(const WindowSlot &) = delete;

    // 0 for the first window, 1 for the second, ...
    [[nodiscard]] int index() const noexcept { return m_index; }

private:
    int m_index = 0;
    std::unique_ptr<QLockFile> m_lock;
};
