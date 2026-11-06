#include "window-slot.h"

#include <QDir>
#include <QStandardPaths>

namespace {

constexpr int maxSlots = 64;

QString slotDirectory()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (base.isEmpty()) {
        base = QDir::tempPath();
    }
    const QString dir = base + QStringLiteral("/tTime-window-slots");
    QDir().mkpath(dir);
    return dir;
}

} // namespace

WindowSlot::WindowSlot()
{
    const QString dir = slotDirectory();
    for (int i = 0; i < maxSlots; ++i) {
        auto lock = std::make_unique<QLockFile>(dir + QStringLiteral("/slot-%1.lock").arg(i));
        // Never expire a lock by age: windows can stay open for hours. Locks of
        // processes that no longer run are still detected as stale.
        lock->setStaleLockTime(std::chrono::milliseconds::zero());
        if (lock->tryLock()) {
            m_index = i;
            m_lock = std::move(lock);
            return;
        }
    }
    // Every slot is taken (or the temp dir is unusable): fall back to the base position.
    m_index = 0;
}
