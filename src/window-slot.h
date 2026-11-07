#pragma once

#include <QtGlobal>

class QSharedMemory;

// Claims the lowest free "cascade slot" shared by all running tTime windows,
// across processes. The slot is held until this object is destroyed. Each slot is
// a named shared memory segment: on Windows the system releases it when its
// process ends (also after a crash); on Unix a segment left behind by a crashed
// process is detected and removed when the slot is claimed next.
class WindowSlot
{
public:
    WindowSlot();
    ~WindowSlot();

    // 0 for the first window, 1 for the second, ...
    int index() const { return m_index; }

private:
    Q_DISABLE_COPY(WindowSlot)

    int m_index;
    QSharedMemory *m_slot;
};
