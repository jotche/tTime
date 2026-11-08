#pragma once

#include "alarm-spec.h"
#include "platform.h"
#include "ui-support.h"

#include <functional>

// Small input window where the user types e.g. "5m green tea".
class NewAlarmWindow
{
public:
    NewAlarmWindow(const AppConfig &config);

    // Pre-fills the input (e.g. with an unparsable command line) and highlights it as
    // invalid if it cannot be parsed.
    void setText(const std::string &text);
    void show();
    void hide();
    [[nodiscard]] bool isVisible() const noexcept { return m_visible; }
    void setWindowIcon(const Image &icon);

    // The UI component, e.g. for reading the compiled-in pictures.
    [[nodiscard]] const slint::ComponentHandle<ui::NewAlarmWindow> &component() const noexcept { return m_ui; }

    std::function<void(const AlarmSpec &)> onAlarmRequested;
    // The user closed the window (not when it is hidden programmatically).
    std::function<void()> onDismissed;

private:
    void submit(const std::string &text);

    slint::ComponentHandle<ui::NewAlarmWindow> m_ui;
    bool m_visible = false;
    bool m_placed = false;
    platform::WindowIcon m_windowIcon; // themed icon for the title bar and taskbar
};
