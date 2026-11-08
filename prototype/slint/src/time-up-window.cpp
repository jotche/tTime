#include "time-up-window.h"

#include "alarm.h"
#include "bell-animation.h"
#include "platform.h"

#include <cmath>
#include <numbers>

using namespace std::chrono_literals;

namespace {

constexpr auto alarmSoundTargetLength = 7s;
constexpr auto treeFrameInterval = 50ms;
constexpr auto trayFrameInterval = 100ms;

} // namespace

TimeUpWindow::TimeUpWindow(const Alarm &alarm, const AppConfig &config, const Pictures &pictures)
    : m_ui(ui::TimeUpWindow::create())
    , m_tray(ui::TimeUpTray::create())
{
    applyConfig(m_ui, config);
    applyConfig(m_tray, config);
    connectDrag(m_ui);

    m_ui->set_message(slint::SharedString(alarm.message()));
    m_ui->set_tree(toSlint(pictures.tree));
    m_ui->on_dismiss([this] { dismiss(); });
    m_ui->on_new_alarm([this] { requestNewAlarm(); });
    // Alt+F4 behaves like the Close button (stops the sound, releases the alarm).
    m_ui->window().on_close_requested([this] {
        dismiss();
        return slint::CloseRequestResponse::HideWindow;
    });

    if (config.animations) {
        if (TreeSway sway(pictures.tree); sway.isValid()) {
            m_treeSway.emplace(std::move(sway));
            m_treeStart = std::chrono::steady_clock::now();
            m_treeTimer.start(slint::TimerMode::Repeated, treeFrameInterval, [this] { advanceTree(); });
        }
    }

    createAnimationFrames(pictures, config.theme);
    const std::string title = alarm.message().empty() ? "Time is up" : alarm.message();
    m_tray->set_tray_tooltip(slint::SharedString(title));
    m_tray->set_tray_icon(m_animationFrames.front());
    m_tray->set_close_icon(toSlint(pictures.alarmIcon));
    m_ui->set_window_icon(m_animationFrames.front());
    m_tray->on_show_window([this] {
        m_ui->show();
        applyWindowIcon(m_ui, m_frameIcons[m_animationFrame]);
        platform::activate(m_ui->window());
    });
    m_tray->on_new_alarm([this] { requestNewAlarm(); });
    m_tray->on_dismiss([this] { dismiss(); });

    if (config.animations) {
        m_animationTimer.start(slint::TimerMode::Repeated, trayFrameInterval, [this] { advanceTrayAnimation(); });
    }
    m_sound = std::make_unique<AlarmSound>(alarmSoundTargetLength);
}

TimeUpWindow::~TimeUpWindow()
{
    m_treeTimer.stop();
    m_animationTimer.stop();
    m_tray->hide();
    m_ui->hide();
}

void TimeUpWindow::show()
{
    m_tray->show();
    centerOnScreen(m_ui);
    m_ui->show();
    applyWindowIcon(m_ui, m_frameIcons[m_animationFrame]);
    platform::activate(m_ui->window());
}

void TimeUpWindow::createAnimationFrames(const Pictures &pictures, const Theme &theme)
{
    // Ringing bell: the colored background stays still, the bell swings from the top
    // of its handle and the clapper follows a little later and further.
    BellAnimation bell(pictures.originalAlarmIcon);
    if (bell.isValid()) {
        bell.setBackground(recolorIconForTheme(bell.background(), theme));
        constexpr int frames = 12; // one swing cycle (frames x timer interval)
        for (int i = 0; i < frames; ++i) {
            const double phase = 2.0 * std::numbers::pi * i / frames;
            const Image frame = bell.frame(12.0 * std::sin(phase), 16.0 * std::sin(phase - 0.9));
            m_animationFrames.push_back(toSlint(frame));
            m_frameIcons.emplace_back(frame);
        }
        return;
    }
    // An icon without a separable bell: keep it still.
    m_animationFrames.push_back(toSlint(pictures.alarmIcon));
    m_frameIcons.emplace_back(pictures.alarmIcon);
}

void TimeUpWindow::advanceTrayAnimation()
{
    m_animationFrame = (m_animationFrame + 1) % m_animationFrames.size();
    m_tray->set_tray_icon(m_animationFrames[m_animationFrame]);
    m_ui->set_window_icon(m_animationFrames[m_animationFrame]);
    // The taskbar rings too (Slint does not update it on Windows).
    platform::setWindowIcon(m_ui->window(), m_frameIcons[m_animationFrame]);
}

void TimeUpWindow::advanceTree()
{
    // Driven by a clock, not by counting ticks, so late ticks do not slow the breeze.
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_treeStart).count();
    const Image frame = m_treeSway->frame(TreeSway::breezeSway * TreeSway::breeze(seconds),
                                          2 * std::numbers::pi * seconds / TreeSway::leafFlutterPeriod,
                                          TreeSway::leafFlutter);
    m_ui->set_tree(toSlint(frame));
}

void TimeUpWindow::dismiss()
{
    if (m_dismissed) {
        return;
    }
    m_dismissed = true;
    m_sound.reset();
    m_animationTimer.stop();
    m_treeTimer.stop();
    m_tray->hide();
    m_ui->hide();
    if (onDismissed) {
        onDismissed();
    }
}

void TimeUpWindow::requestNewAlarm()
{
    if (onNewAlarmRequested) {
        onNewAlarmRequested();
    }
    dismiss();
}
