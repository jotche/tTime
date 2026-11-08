#pragma once

#include "app.h" // generated from ui/app.slint (namespace ui)
#include "config.h"
#include "image.h"
#include "platform.h"

#include <slint.h>

// The pictures the C++ side recolors and animates, read once from the compiled-in UI.
struct Pictures
{
    Image alarmIcon; // the app icon in the configured theme (not premultiplied)
    Image tree; // the "Time's up" tree in the configured theme
    Image originalAlarmIcon; // as drawn (blue), for the bell animation
};

[[nodiscard]] Image fromSlint(const slint::Image &image);
[[nodiscard]] slint::Image toSlint(const Image &image);
[[nodiscard]] slint::Color toSlint(Color color);

// Reads the pictures from a component's Assets global and themes them.
template<typename Component>
[[nodiscard]] Pictures loadPictures(const slint::ComponentHandle<Component> &component, const Theme &theme)
{
    const auto &assets = component->template global<ui::Assets>();
    Pictures pictures;
    pictures.originalAlarmIcon = fromSlint(assets.get_alarm_icon());
    pictures.alarmIcon = themedAppIcon(pictures.originalAlarmIcon, theme);
    pictures.tree = recolorForTheme(fromSlint(assets.get_tree()), theme);
    return pictures;
}

// Applies the theme colors and fonts of `config` to a component (globals are per component).
template<typename Component>
void applyConfig(const slint::ComponentHandle<Component> &component, const AppConfig &config)
{
    const auto &theme = component->template global<ui::Theme>();
    theme.set_dark(toSlint(config.theme.dark));
    theme.set_mid(toSlint(config.theme.mid));
    theme.set_light(toSlint(config.theme.light));
    theme.set_highlight(toSlint(config.theme.highlight));
    theme.set_highlight_border(toSlint(config.theme.highlightBorder));

    const auto &fonts = component->template global<ui::Fonts>();
    fonts.set_countdown_family(slint::SharedString(config.countdownFont.family));
    fonts.set_countdown_weight(config.countdownFont.weight);
    fonts.set_countdown_size(pointsToPixels(config.countdownFont.pointSize));
    fonts.set_message_family(slint::SharedString(config.messageFont.family));
    fonts.set_message_weight(config.messageFont.weight);
    fonts.set_message_size(pointsToPixels(config.messageFont.pointSize));
    fonts.set_time_up_message_size(pointsToPixels(config.timeUpMessagePointSize));
    fonts.set_title_size(pointsToPixels(20));
}

// Gives a window its native icon now and, since Slint creates the native window only once
// the event loop handles show(), as soon as that window exists (unless a newer icon has
// been set by then).
template<typename Component>
void applyWindowIcon(const slint::ComponentHandle<Component> &component, const platform::WindowIcon &icon)
{
    platform::setWindowIcon(component->window(), icon);
    struct Retry
    {
        static void after(slint::ComponentWeakHandle<Component> weak, platform::WindowIcon icon, int attempts)
        {
            slint::Timer::single_shot(std::chrono::milliseconds(10), [weak, icon, attempts] {
                const auto ui = weak.lock();
                if (!ui) {
                    return;
                }
                if (platform::hasNativeWindow((*ui)->window())) {
                    platform::setWindowIcon((*ui)->window(), icon, /*onlyIfUnset=*/true);
                } else if (attempts > 0) {
                    after(weak, icon, attempts - 1);
                }
            });
        }
    };
    Retry::after(slint::ComponentWeakHandle<Component>(component), icon, 50);
}

// Centres a window (with natural-width/natural-height properties) on the primary screen,
// like Qt places a new top-level window. Call before showing it.
template<typename Component>
void centerOnScreen(const slint::ComponentHandle<Component> &component)
{
    const Rect area = platform::primaryScreenArea();
    if (!area.isValid()) {
        return; // unknown screen: let the window system place it
    }
    const float x = area.x + (area.width - component->get_natural_width()) / 2;
    const float y = area.y + (area.height - component->get_natural_height()) / 2;
    component->window().set_position(slint::LogicalPosition({x, y}));
}

// Moves a frameless window while its DragArea is dragged. Where the system reports the
// cursor (Windows), the window follows it exactly: start position + cursor distance since
// the press. Elsewhere it moves by the (dx, dy) the DragArea reports.
template<typename Component>
void connectDrag(const slint::ComponentHandle<Component> &component)
{
    struct DragState
    {
        slint::PhysicalPosition window{};
        std::optional<Point> cursor;
    };
    auto state = std::make_shared<DragState>();
    const auto weak = slint::ComponentWeakHandle<Component>(component);

    const auto follow = [weak, state] {
        const auto ui = weak.lock();
        const auto cursor = platform::cursorPosition();
        if (!ui || !state->cursor || !cursor) {
            return false;
        }
        (*ui)->window().set_position(slint::PhysicalPosition({state->window.x + cursor->x - state->cursor->x,
                                                              state->window.y + cursor->y - state->cursor->y}));
        return true;
    };
    component->on_drag_started([weak, state] {
        if (const auto ui = weak.lock()) {
            state->window = (*ui)->window().position();
            state->cursor = platform::cursorPosition();
        }
    });
    component->on_dragged([weak, follow](float dx, float dy) {
        if (follow()) {
            return;
        }
        if (const auto ui = weak.lock()) {
            auto &window = (*ui)->window();
            const float scale = window.scale_factor();
            const slint::PhysicalPosition pos = window.position();
            window.set_position(slint::PhysicalPosition(
                {pos.x + static_cast<int32_t>(dx * scale), pos.y + static_cast<int32_t>(dy * scale)}));
        }
    });
    component->on_drag_ended([follow, state] {
        follow(); // the last mouse move before the release may not have arrived
        state->cursor.reset();
    });
}
