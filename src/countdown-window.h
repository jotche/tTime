#pragma once

#include "alarm.h"
#include "theme.h"
#include "window-slot.h"

#include <QPoint>
#include <QScopedPointer>
#include <QSize>
#include <QSystemTrayIcon>
#include <QWidget>

class AppSettings;
struct AppConfig;
class QAction;
class QMenu;

namespace Ui {
class CountdownWindow;
}

// Frameless countdown window with its own tray icon that fills up as time passes.
// It presents an Alarm and reports what the user wants (quit, new alarm) through
// its own signals; it remembers its size and position in AppSettings.
class CountdownWindow : public QWidget
{
    Q_OBJECT

public:
    // `alarm`, `config` and `settings` are not owned and must outlive this window.
    CountdownWindow(Alarm *alarm, const AppConfig &config, AppSettings &settings, QWidget *parent = 0);
    ~CountdownWindow(); // saves the window geometry

    void setVisible(bool visible);

signals:
    void quitRequested(); // "Quit" in the toolbar, context or tray menu
    void newAlarmRequested(); // "New Alarm" in the context or tray menu

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);

private slots:
    void togglePlayPause();
    void setStayOnTop(bool onTop);
    void quitAlarm();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onTimeLeftChanged(qint64 secondsLeft);
    void onStateChanged(Alarm::State state);

private:
    enum Drag { NoDrag, MoveDrag, ResizeDrag };

    void placeWindowActions();
    void fitHeightToContent();
    void createActions();
    void createTrayIcon();
    void updateTrayIcon(double fractionLeft);
    void loadSettings();
    void saveSettings() const;

    QScopedPointer<Ui::CountdownWindow> ui;
    Alarm *m_alarm;
    Theme m_theme;
    AppSettings &m_settings;

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_restartAction;
    QAction *m_minimizeAction;
    QAction *m_restoreAction;
    QAction *m_newAlarmAction;
    QAction *m_quitAction;

    // Moving (drag) and resizing (Ctrl+drag, width only) with the mouse.
    Drag m_drag;
    QPoint m_dragOffset; // MoveDrag: cursor position relative to the window
    int m_dragStartX; // ResizeDrag: global cursor x and window width when it started
    int m_dragStartWidth;
    double m_trayFractionLeft; // last value drawn into the tray icon

    // Cascading: several open windows are offset so they don't cover each other.
    WindowSlot m_slot;
    QPoint m_placedPos; // where the window was put automatically; not a user choice
    QSize m_placedSize; // size after automatic fitting; not a user choice
};
