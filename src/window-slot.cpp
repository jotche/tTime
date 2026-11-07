#include "window-slot.h"

#include <QSharedMemory>
#include <QString>

namespace {

const int maxSlots = 64;

bool claim(QSharedMemory &slot)
{
    if (slot.create(1)) {
        return true;
    }
    if (slot.error() != QSharedMemory::AlreadyExists) {
        return false;
    }
    // Attaching and detaching again removes a segment that no process uses any more
    // (left behind by a crashed tTime on Unix); a slot in use is not affected.
    if (slot.attach()) {
        slot.detach();
    }
    return slot.create(1);
}

} // namespace

WindowSlot::WindowSlot()
    : m_index(0)
    , m_slot(0)
{
    for (int i = 0; i < maxSlots; ++i) {
        QSharedMemory *slot = new QSharedMemory(QString::fromLatin1("tTime-window-slot-%1").arg(i));
        if (claim(*slot)) {
            m_index = i;
            m_slot = slot;
            return;
        }
        delete slot;
    }
    // Every slot is taken (or shared memory is unusable): fall back to the base position.
    m_index = 0;
}

WindowSlot::~WindowSlot()
{
    delete m_slot;
}
