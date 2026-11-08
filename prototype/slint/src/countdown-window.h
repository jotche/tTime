#pragma once

#include "alarm.h"
#include "platform.h"
#include "ui-support.h"

#include <functional>
#include <memory>

class AppSettings;

// Frameless countdown window with its own tray icon that fills up as time passes.
// It presents an Alarm and reports what the user wants (quit, new alarm); it
// remembers its size and position in AppSettings.
class CountdownWindow
{
public:
    // `alarm`, `config`, `settings` and `pictures` must outlive this window.
    CountdownWindow(Alarm &alarm, const AppConfig &config, AppSettings &settings, const Pictures &pictures);
    ~CountdownWindow(); // saves the window geometry
    CountdownWindow(const CountdownWindow &) = delete;
    CountdownWindow &operator=(const CountdownWindow &) = delete;

    void show();
    void setVisible(bool visible);

    std::function<void()> onQuitRequested; // "Quit" in the toolbar, context or tray menu
    std::function<void()> onNewAlarmRequested; // "New Alarm" in the context or tray menu

private:
    void togglePlayPause();
    void onTimeLeftChanged(std::chrono::seconds secondsLeft);
    void onStateChanged(Alarm::State state);
    void updateTrayIcon(double fractionLeft);
    void loadGeometry();
    void saveGeometry() const;

    Alarm &m_alarm;
    AppSettings &m_settings;
    const Pictures &m_pictures;
    slint::ComponentHandle<ui::CountdownWindow> m_ui;
    slint::ComponentHandle<ui::CountdownTray> m_tray;
    std::shared_ptr<slint::VectorModel<ui::Glyph>> m_glyphs;
    double m_trayFractionLeft = -1; // last value drawn into the tray icon
    platform::WindowIcon m_windowIcon; // the same picture for the taskbar
    bool m_visible = false;

    // Cascading: several open windows are offset so they don't cover each other.
    platform::WindowSlot m_slot;
    Point m_placedPos; // where the window was put automatically; not a user choice
    int m_placedWidth = 0; // width after automatic fitting; not a user choice
};
