#pragma once

#include <QIcon>
#include <QMenu>
#include <QString>
#include <QSystemTrayIcon>
#include <QWidget>

// Small helpers shared by tTime's windows.

// Shows `window` (restoring it if minimized or hidden) and brings it to the front.
inline void showAndActivate(QWidget *window)
{
    window->showNormal();
    window->raise();
    window->activateWindow();
}

// A visible tray icon owned by `owner`, with a context menu and the owner's
// window title as tooltip.
inline QSystemTrayIcon *createTrayIcon(QWidget *owner, const QIcon &icon, QMenu *menu)
{
    QSystemTrayIcon *trayIcon = new QSystemTrayIcon(icon, owner);
    trayIcon->setContextMenu(menu);
    trayIcon->setToolTip(owner->windowTitle());
    trayIcon->show();
    return trayIcon;
}
