#pragma once

#include "alarm.h"
#include "theme.h"
#include "window-slot.h"

#include <QPoint>
#include <QSize>
#include <QSystemTrayIcon>
#include <QWidget>

#include <chrono>
#include <memory>

class AppSettings;
struct AppConfig;
class QAction;
class QMenu;

namespace Ui {
class CountdownWindow;
}

// Frameless countdown window with its own tray icon that fills up as time passes.
// It presents a Alarm and reports what the user wants (quit, new alarm) through
// its own signals; it remembers its size and position in AppSettings.
class CountdownWindow : public QWidget
{
    Q_OBJECT

public:
    // `alarm`, `config` and `settings` are not owned and must outlive this window.
    CountdownWindow(Alarm *alarm, const AppConfig &config, AppSettings &settings, QWidget *parent = nullptr);
    ~CountdownWindow() override; // saves the window geometry

    void setVisible(bool visible) override;

signals:
    void quitRequested(); // "Quit" in the toolbar, context or tray menu
    void newAlarmRequested(); // "New Alarm" in the context or tray menu

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void createActions();
    void createTrayIcon();
    void togglePlayPause();
    void setStayOnTop(bool onTop);
    void quitAlarm();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onTimeLeftChanged(std::chrono::seconds secondsLeft);
    void onStateChanged(Alarm::State state);
    void updateTrayIcon(double fractionLeft);
    void loadSettings();
    void saveSettings() const;

    std::unique_ptr<Ui::CountdownWindow> ui;
    Alarm *m_alarm;
    Theme m_theme;
    AppSettings &m_settings;

    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_restartAction = nullptr;
    QAction *m_minimizeAction = nullptr;
    QAction *m_restoreAction = nullptr;
    QAction *m_newAlarmAction = nullptr;
    QAction *m_quitAction = nullptr;

    QPoint m_dragOffset; // fallback manual dragging
    bool m_manualDrag = false;
    double m_trayFractionLeft = -1; // last value drawn into the tray icon

    // Cascading: several open windows are offset so they don't cover each other.
    WindowSlot m_slot;
    QPoint m_placedPos; // where the window was put automatically; not a user choice
    QSize m_placedSize; // size after automatic fitting; not a user choice
};
