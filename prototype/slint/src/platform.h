#pragma once

#include "image.h"
#include "window-placement.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace slint {
class Window;
}

// The few things Slint does not provide, per operating system.
namespace platform {

// Command-line arguments (without the program name) as UTF-8.
[[nodiscard]] std::vector<std::string> arguments(int argc, char *argv[]);

[[nodiscard]] std::filesystem::path executableDirectory();
// %APPDATA% on Windows, $XDG_CONFIG_HOME or ~/.config elsewhere.
[[nodiscard]] std::filesystem::path userConfigDirectory();

// Usable area (without taskbars/docks) of every screen, in logical pixels. Empty if unknown.
[[nodiscard]] std::vector<Rect> availableScreenAreas();
// Usable area of the primary screen, in logical pixels (invalid if unknown).
[[nodiscard]] Rect primaryScreenArea();

// A native window icon (title bar, taskbar, Alt+Tab) made from an image. On Windows it
// holds a small and a large HICON scaled down from the image; elsewhere it is empty and
// the Slint `icon` property is used instead.
class WindowIcon
{
public:
    WindowIcon() = default;
    explicit WindowIcon(const Image &image, float scale = 1.0F);

    [[nodiscard]] bool isNull() const noexcept { return !m_big; }

private:
    friend void setWindowIcon(const slint::Window &window, const WindowIcon &icon, bool onlyIfUnset);
    std::shared_ptr<void> m_big;
    std::shared_ptr<void> m_small;
};

// Gives a shown window this icon (Slint does not update the taskbar icon on Windows). The
// icon is kept alive while the window shows it. With `onlyIfUnset`, a window that already
// has an icon from here keeps it.
void setWindowIcon(const slint::Window &window, const WindowIcon &icon, bool onlyIfUnset = false);
// Whether the native window exists yet (always true where there is nothing to wait for).
[[nodiscard]] bool hasNativeWindow(const slint::Window &window);

// The mouse cursor on the screen in physical pixels, where the system can tell.
[[nodiscard]] std::optional<Point> cursorPosition();

// Brings a shown window to the front and gives it the focus.
void activate(const slint::Window &window);

// Shows text to the user (a message box on Windows, where a GUI program has no console).
void showMessage(const std::string &title, const std::string &text, bool error = false);

// Claims the lowest free "cascade slot" shared by all running tTime windows, across
// processes. The slot is held until this object is destroyed; the operating system
// releases it if the process ends (also by crashing).
class WindowSlot
{
public:
    WindowSlot();
    ~WindowSlot();
    WindowSlot(const WindowSlot &) = delete;
    WindowSlot &operator=(const WindowSlot &) = delete;

    // 0 for the first window, 1 for the second, ...
    [[nodiscard]] int index() const noexcept { return m_index; }

private:
    int m_index = 0;
#ifdef _WIN32
    void *m_handle = nullptr;
#else
    int m_fd = -1;
#endif
};

} // namespace platform
