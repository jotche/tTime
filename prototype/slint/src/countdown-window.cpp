#include "countdown-window.h"

#include "alarm-spec.h"
#include "settings.h"

#include <algorithm>
#include <cmath>

using namespace std::chrono;

namespace {

// Historical key names (the window used to be called the "progress dialog"); shared
// with the Qt tTime, so both remember the same window size and position.
const std::string settingsSizeKey = "progress-dialog/size";
const std::string settingsPosKey = "progress-dialog/pos";
constexpr Size defaultWindowSize{200, 130};
constexpr Point defaultWindowPos{250, 250};
constexpr int minimumWindowWidth = 200;

// The tray icon of the countdown: the bell at 25 % opacity, filling up opaque from the
// bottom as time passes, with a 5px white line at the fill level.
Image progressIcon(const Image &source, double fractionLeft)
{
    Image image = source;
    const double line = image.height * fractionLeft;
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            Rgba &p = image.at(x, y);
            if (std::abs(y + 0.5 - line) <= 2.5 && fractionLeft > 0 && fractionLeft < 1) {
                p = {255, 255, 255, 255};
            } else if (y + 0.5 < line) {
                p.a = static_cast<std::uint8_t>(p.a / 4);
            }
        }
    }
    return image;
}

} // namespace

CountdownWindow::CountdownWindow(Alarm &alarm, const AppConfig &config, AppSettings &settings, const Pictures &pictures)
    : m_alarm(alarm)
    , m_settings(settings)
    , m_pictures(pictures)
    , m_ui(ui::CountdownWindow::create())
    , m_tray(ui::CountdownTray::create())
    , m_glyphs(std::make_shared<slint::VectorModel<ui::Glyph>>())
{
    applyConfig(m_ui, config);
    applyConfig(m_tray, config);
    connectDrag(m_ui);

    m_ui->set_message(slint::SharedString(m_alarm.message()));
    m_ui->set_glyphs(m_glyphs);

    m_ui->on_play_pause([this] { togglePlayPause(); });
    m_ui->on_stop([this] { m_alarm.stop(); });
    m_ui->on_restart([this] { m_alarm.restart(); });
    m_ui->on_minimize([this] { setVisible(false); });
    m_ui->on_quit([this] { onQuitRequested(); });
    m_ui->on_new_alarm([this] { onNewAlarmRequested(); });
    // Alt+F4 & co. minimise to the tray instead of silently killing the alarm.
    m_ui->window().on_close_requested([this] {
        setVisible(false);
        return slint::CloseRequestResponse::KeepWindowShown;
    });

    m_tray->on_toggle_window([this] {
        if (m_visible) {
            setVisible(false);
        } else {
            setVisible(true);
            platform::activate(m_ui->window());
        }
    });
    m_tray->on_minimize([this] { setVisible(false); });
    m_tray->on_restore([this] {
        setVisible(true);
        platform::activate(m_ui->window());
    });
    m_tray->on_new_alarm([this] { onNewAlarmRequested(); });
    m_tray->on_quit([this] { onQuitRequested(); });

    m_alarm.onTimeLeftChanged = [this](seconds left) { onTimeLeftChanged(left); };
    m_alarm.onStateChanged = [this](Alarm::State state) { onStateChanged(state); };
    onTimeLeftChanged(m_alarm.remainingSeconds());
    onStateChanged(m_alarm.state());
    loadGeometry();
}

CountdownWindow::~CountdownWindow()
{
    // Closed by quitting, by the alarm finishing (the "Time's up" window replaces it)
    // or by the application exiting: save in all cases.
    saveGeometry();
    m_alarm.onTimeLeftChanged = nullptr;
    m_alarm.onStateChanged = nullptr;
    m_tray->hide();
    m_ui->hide();
}

void CountdownWindow::show()
{
    m_tray->show();
    setVisible(true);
}

void CountdownWindow::setVisible(bool visible)
{
    m_visible = visible;
    m_tray->set_window_visible(visible);
    if (visible) {
        m_ui->show();
        applyWindowIcon(m_ui, m_windowIcon);
    } else {
        m_ui->hide();
    }
}

void CountdownWindow::togglePlayPause()
{
    if (m_alarm.state() == Alarm::State::Running) {
        m_alarm.pause();
    } else {
        m_alarm.start();
    }
}

void CountdownWindow::onTimeLeftChanged(seconds secondsLeft)
{
    const std::string text = formatDuration(secondsLeft);
    std::vector<ui::Glyph> glyphs;
    for (const char c : text) {
        glyphs.push_back({slint::SharedString(std::string(1, c)), c >= '0' && c <= '9'});
    }
    m_glyphs->set_vector(std::move(glyphs));
    m_ui->set_time_text(slint::SharedString(text));

    const auto total = m_alarm.duration().count();
    updateTrayIcon(total > 0 ? static_cast<double>(secondsLeft.count()) / static_cast<double>(total) : 0.0);

    const std::string tooltip = m_alarm.message().empty() ? text : text + " - " + m_alarm.message();
    m_tray->set_tray_tooltip(slint::SharedString(tooltip));
}

void CountdownWindow::onStateChanged(Alarm::State state)
{
    m_ui->set_running(state == Alarm::State::Running);
}

void CountdownWindow::updateTrayIcon(double fractionLeft)
{
    // Only redraw on noticeable change (or on reset back to full).
    if (m_trayFractionLeft >= 0 && fractionLeft < m_trayFractionLeft && m_trayFractionLeft - fractionLeft < 0.05) {
        return;
    }
    m_trayFractionLeft = fractionLeft;
    const Image image = progressIcon(m_pictures.alarmIcon, fractionLeft);
    const slint::Image icon = toSlint(image);
    m_tray->set_tray_icon(icon);
    m_ui->set_window_icon(icon);
    // The taskbar shows the same fill level (Slint does not update it on Windows).
    m_windowIcon = platform::WindowIcon(image);
    platform::setWindowIcon(m_ui->window(), m_windowIcon);
}

void CountdownWindow::loadGeometry()
{
    const auto savedPos = AppSettings::parsePair(m_settings.value(settingsPosKey, ""), "Point");
    const auto savedSize = AppSettings::parsePair(m_settings.value(settingsSizeKey, ""), "Size");
    Rect saved{savedPos ? savedPos->first : defaultWindowPos.x, savedPos ? savedPos->second : defaultWindowPos.y,
               savedSize ? savedSize->first : defaultWindowSize.width,
               savedSize ? savedSize->second : defaultWindowSize.height};
    if (!saved.isValid()) {
        saved = {defaultWindowPos.x, defaultWindowPos.y, defaultWindowSize.width, defaultWindowSize.height};
    }
    saved.width = std::max(saved.width, minimumWindowWidth);

    // The saved position may be off-screen (monitor unplugged, resolution or taskbar
    // changed): move it by the shortest distance so it is fully visible.
    Rect geometry = saved;
    Rect area{-100000, -100000, 200000, 200000}; // no screen information: no fitting
    if (const std::vector<Rect> screens = platform::availableScreenAreas(); !screens.empty()) {
        const VisiblePlacement visible = fitIntoVisibleArea(saved, screens);
        geometry = visible.geometry;
        area = visible.area;
    }

    // Offset this window if other tTime windows (in this or other processes) are open.
    m_placedWidth = std::max(geometry.width, minimumWindowWidth);
    m_placedPos = cascadePosition(geometry.topLeft(), geometry.size(), area, m_slot.index());
    m_ui->set_window_width(static_cast<float>(m_placedWidth));
    m_ui->window().set_position(
        slint::LogicalPosition({static_cast<float>(m_placedPos.x), static_cast<float>(m_placedPos.y)}));
}

void CountdownWindow::saveGeometry() const
{
    // Only remember geometry the user chose: automatic adjustments made in
    // loadGeometry() (fitting onto the screen, cascade offset) are not saved, so the
    // original position comes back when e.g. the monitor is reconnected.
    const auto &window = m_ui->window();
    const float scale = window.scale_factor();
    const int width = static_cast<int>(std::lround(m_ui->get_window_width()));
    if (width != m_placedWidth) {
        const int height = static_cast<int>(std::lround(window.size().height / scale));
        m_settings.setValue(settingsSizeKey, AppSettings::formatPair("Size", width, height));
    }
    const slint::PhysicalPosition physical = window.position();
    const Point pos{static_cast<int>(std::lround(physical.x / scale)), static_cast<int>(std::lround(physical.y / scale))};
    if (pos != m_placedPos) {
        m_settings.setValue(settingsPosKey, AppSettings::formatPair("Point", pos.x, pos.y));
    }
}
