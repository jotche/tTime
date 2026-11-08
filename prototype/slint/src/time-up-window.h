#pragma once

#include "platform.h"
#include "sound.h"
#include "tree-sway.h"
#include "ui-support.h"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class Alarm;

// "Time is up" window: plays the alarm sound, sways the tree and rings the tray icon
// until dismissed.
class TimeUpWindow
{
public:
    TimeUpWindow(const Alarm &alarm, const AppConfig &config, const Pictures &pictures);
    ~TimeUpWindow();
    TimeUpWindow(const TimeUpWindow &) = delete;
    TimeUpWindow &operator=(const TimeUpWindow &) = delete;

    void show();

    std::function<void()> onDismissed; // Close button, tray "Close", or the window was closed
    std::function<void()> onNewAlarmRequested; // "New Alarm" button or tray menu (also dismisses)

private:
    void dismiss();
    void requestNewAlarm();
    void createAnimationFrames(const Pictures &pictures, const Theme &theme);
    void advanceTrayAnimation();
    void advanceTree();

    slint::ComponentHandle<ui::TimeUpWindow> m_ui;
    slint::ComponentHandle<ui::TimeUpTray> m_tray;
    std::unique_ptr<AlarmSound> m_sound;

    std::optional<TreeSway> m_treeSway; // empty when animations are off
    slint::Timer m_treeTimer;
    std::chrono::steady_clock::time_point m_treeStart;

    std::vector<slint::Image> m_animationFrames; // one ringing cycle, looped
    std::vector<platform::WindowIcon> m_frameIcons; // the same frames for the taskbar
    std::size_t m_animationFrame = 0;
    slint::Timer m_animationTimer;
    bool m_dismissed = false;
};
