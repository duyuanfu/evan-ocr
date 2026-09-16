#include "pin_manager.h"

PinManager& PinManager::instance()
{
    static PinManager mgr;
    return mgr;
}

PinWindow* PinManager::createPin(const QPixmap& pixmap, const QRect& initialGeometry)
{
    auto* win = new PinWindow(pixmap, initialGeometry);
    m_pins.append(win);

    connect(win, &QObject::destroyed, this, [this, win]() {
        m_pins.removeAll(win);
    });

    return win;
}

void PinManager::closeAll()
{
    for (auto* win : m_pins) {
        if (win) {
            win->close();
        }
    }
    m_pins.clear();
}
