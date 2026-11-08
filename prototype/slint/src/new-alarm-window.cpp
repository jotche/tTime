#include "new-alarm-window.h"

#include "platform.h"

NewAlarmWindow::NewAlarmWindow(const AppConfig &config)
    : m_ui(ui::NewAlarmWindow::create())
{
    applyConfig(m_ui, config);
    m_ui->on_submit([this](const slint::SharedString &text) { submit(std::string(text)); });
    m_ui->on_edited([this] { m_ui->set_invalid(false); });
    m_ui->window().on_close_requested([this] {
        m_visible = false;
        if (onDismissed) {
            onDismissed();
        }
        return slint::CloseRequestResponse::HideWindow;
    });
}

void NewAlarmWindow::setText(const std::string &text)
{
    m_ui->set_text(slint::SharedString(text));
    m_ui->set_invalid(!text.empty() && !parseAlarmSpec(text));
}

void NewAlarmWindow::show()
{
    if (!m_placed) {
        centerOnScreen(m_ui); // first time only; afterwards it reopens where the user left it
        m_placed = true;
    }
    m_visible = true;
    m_ui->show();
    applyWindowIcon(m_ui, m_windowIcon);
    platform::activate(m_ui->window());
}

void NewAlarmWindow::hide()
{
    m_visible = false;
    m_ui->hide();
}

void NewAlarmWindow::setWindowIcon(const Image &icon)
{
    m_ui->set_window_icon(toSlint(icon));
    m_windowIcon = platform::WindowIcon(icon);
    platform::setWindowIcon(m_ui->window(), m_windowIcon);
}

void NewAlarmWindow::submit(const std::string &text)
{
    const auto spec = parseAlarmSpec(text);
    if (!spec) {
        m_ui->set_invalid(true);
        return;
    }
    m_ui->set_text("");
    m_ui->set_invalid(false);
    if (onAlarmRequested) {
        onAlarmRequested(*spec);
    }
}
