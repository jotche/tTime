#include "countdown-window.h"

#include "alarm-spec.h"
#include "app-config.h"
#include "app-settings.h"
#include "countdown-font.h"
#include "ui_countdown-window.h"
#include "window-placement.h"
#include "window-utils.h"

#include <QAction>
#include <QCloseEvent>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QToolButton>
#include <QWindow>

#include <algorithm>

using namespace std::chrono;

namespace {

// Historical key names (the window used to be called the "progress dialog"); kept so
// existing tTime.ini files keep their saved window size and position.
const auto settingsSizeKey = QStringLiteral("progress-dialog/size");
const auto settingsPosKey = QStringLiteral("progress-dialog/pos");
// Used when tTime.ini has no size; the shipped tTime.ini has the same value.
constexpr QSize defaultWindowSize(200, 130);
// The window can be made wider but not narrower than this; its height always
// follows the content.
constexpr int minimumWindowWidth = 200;
// The window actions sit this far from the top and right edges, ignoring the padding.
constexpr int windowActionsInset = 8;
// Smallest gap between the action row and the window actions.
constexpr int windowActionsGap = 8;

} // namespace

CountdownWindow::CountdownWindow(Alarm *alarm, const AppConfig &config, AppSettings &settings, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint)
    , ui(std::make_unique<Ui::CountdownWindow>())
    , m_alarm(alarm)
    , m_theme(config.theme)
    , m_settings(settings)
{
    Q_ASSERT(m_alarm);

    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(toolButtonStyleSheet(m_theme));
    setWindowTitle(m_alarm->message().isEmpty() ? tr("tTime") : m_alarm->message());

    createActions();
    addAction(m_restartAction);
    addAction(m_minimizeAction);
    addAction(m_restoreAction);
    addAction(m_newAlarmAction);
    addAction(m_quitAction);
    setContextMenuPolicy(Qt::ActionsContextMenu);

    createTrayIcon();

    ui->timeLabel->setFont(config.countdownFont);
    // Digits sit on the baseline and never use the font's descender space, so
    // leave it out of the countdown label: the 4px gap to the message is then
    // the visible gap below the digits.
    ui->timeLabel->setContentsMargins(0, 0, 0, -QFontMetrics(config.countdownFont).descent());
    ui->descriptionLabel->setText(m_alarm->message());
    ui->descriptionLabel->setFont(config.messageFont);

    ui->restartButton->setDefaultAction(m_restartAction);
    ui->minimizeButton->setDefaultAction(m_minimizeAction);
    ui->exitButton->setDefaultAction(m_quitAction);

    connect(ui->playPauseButton, &QToolButton::clicked, this, &CountdownWindow::togglePlayPause);
    connect(ui->stopButton, &QToolButton::clicked, m_alarm, &Alarm::stop);
    connect(ui->stayOnTopButton, &QToolButton::toggled, this, &CountdownWindow::setStayOnTop);

    connect(m_alarm, &Alarm::timeLeftChanged, this, &CountdownWindow::onTimeLeftChanged);
    connect(m_alarm, &Alarm::stateChanged, this, &CountdownWindow::onStateChanged);

    // Lock every toolbar button at its normal size. The highlight style
    // (hover/pressed/checked) adds a 1px border; without this a highlighted
    // button grew by 2px, and the layout applied that whenever the hovered
    // button's icon changed (e.g. clicking start), so the whole content jumped.
    // Alternatives tried (see _docs/code-review.md, M6): a transparent border in
    // the normal state moves the tuned layout by 1-5px, and adding margin: -1px
    // keeps the layout but clips the highlight border.
    for (QToolButton *button : findChildren<QToolButton *>()) {
        button->ensurePolished();
        button->setFixedSize(button->sizeHint());
    }
    // The window actions float over the top-right corner (see placeWindowActions()).
    ui->windowActions->adjustSize();
    ui->windowActions->raise();
    // Reserve room for them at the end of the action row, so a narrow window keeps
    // windowActionsGap between the two. The row ends at the right padding; the
    // window actions end windowActionsInset from the edge.
    ui->actionContainer->addSpacing(ui->windowActions->width() + windowActionsInset
                                    - layout()->contentsMargins().right() + windowActionsGap);
    placeWindowActions();

    setMinimumWidth(minimumWindowWidth);
    fitHeightToContent();

    onTimeLeftChanged(m_alarm->remainingSeconds());
    onStateChanged(m_alarm->state());
    loadSettings();
}

CountdownWindow::~CountdownWindow()
{
    // The window may be closed by quitting, by the alarm finishing (the "Time's
    // up" window replaces it) or by the application exiting: save in all cases.
    saveSettings();
}

void CountdownWindow::createActions()
{
    m_minimizeAction = new QAction(ui->minimizeButton->icon(), tr("Mi&nimize"), this);
    connect(m_minimizeAction, &QAction::triggered, this, &QWidget::hide);

    m_restoreAction = new QAction(tr("&Restore"), this);
    connect(m_restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    m_quitAction = new QAction(ui->exitButton->icon(), tr("&Quit"), this);
    m_quitAction->setObjectName(QStringLiteral("quitAction"));
    connect(m_quitAction, &QAction::triggered, this, &CountdownWindow::quitAlarm);

    m_restartAction = new QAction(ui->restartButton->icon(), tr("Restart"), this);
    connect(m_restartAction, &QAction::triggered, m_alarm, &Alarm::restart);

    m_newAlarmAction = new QAction(tr("New Alarm"), this);
    m_newAlarmAction->setObjectName(QStringLiteral("newAlarmAction"));
    connect(m_newAlarmAction, &QAction::triggered, this, &CountdownWindow::newAlarmRequested);
}

void CountdownWindow::createTrayIcon()
{
    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction(m_minimizeAction);
    m_trayMenu->addAction(m_restoreAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_newAlarmAction);
    m_trayMenu->addAction(m_quitAction);

    m_trayIcon = ::createTrayIcon(this, themedAppIcon(m_theme), m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &CountdownWindow::onTrayIconActivated);
}

void CountdownWindow::setVisible(bool visible)
{
    if (m_minimizeAction) {
        m_minimizeAction->setEnabled(visible);
        m_restoreAction->setEnabled(!visible);
    }
    QWidget::setVisible(visible);
}

void CountdownWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    // Prefer native move/resize (works with frameless windows and high-DPI).
    QWindow *window = windowHandle();
    const bool resize = event->modifiers().testFlag(Qt::ControlModifier);
    const bool handled = window && (resize ? window->startSystemResize(Qt::RightEdge) : window->startSystemMove());

    m_manualDrag = !handled && !resize;
    if (m_manualDrag) {
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    event->accept();
}

void CountdownWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_manualDrag && event->buttons().testFlag(Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void CountdownWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    fitHeightToContent(); // a new width can rewrap the message
    placeWindowActions();
}

void CountdownWindow::fitHeightToContent()
{
    const QLayout *content = layout();
    const int height
        = content->hasHeightForWidth() ? content->totalHeightForWidth(width()) : content->totalSizeHint().height();
    if (minimumHeight() != height || maximumHeight() != height) {
        setFixedHeight(height);
    }
}

void CountdownWindow::placeWindowActions()
{
    // Pinned to the top-right corner, outside the window's padding and layout.
    QWidget *actions = ui->windowActions;
    actions->move(width() - actions->width() - windowActionsInset, windowActionsInset);
}

void CountdownWindow::closeEvent(QCloseEvent *event)
{
    // Alt+F4 & co. minimise to the tray instead of silently killing the alarm.
    hide();
    event->ignore();
}

void CountdownWindow::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    paintPanel(painter, rect(), m_theme,
               PanelStyle{.cornerRadius = 0,
                          .sideEdgeAlpha = 200,
                          .sideInsetAlpha = 150,
                          .sideLightAlpha = 80,
                          .lightBandLeft = true});
}

void CountdownWindow::togglePlayPause()
{
    if (m_alarm->state() == Alarm::State::Running) {
        m_alarm->pause();
    } else {
        m_alarm->start();
    }
}

void CountdownWindow::setStayOnTop(bool onTop)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
    show(); // changing window flags hides the window
}

void CountdownWindow::quitAlarm()
{
    emit quitRequested();
}

void CountdownWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            showAndActivate(this);
        }
    }
}

void CountdownWindow::onTimeLeftChanged(seconds secondsLeft)
{
    const QString text = formatDuration(secondsLeft);
    ui->timeLabel->setText(text);

    const auto total = m_alarm->duration().count();
    const double fractionLeft = total > 0 ? static_cast<double>(secondsLeft.count()) / static_cast<double>(total) : 0.0;
    updateTrayIcon(fractionLeft);

    const QString title
        = m_alarm->message().isEmpty() ? text : QString(text + QStringLiteral(" - ") + m_alarm->message());
    m_trayIcon->setToolTip(title);
}

void CountdownWindow::onStateChanged(Alarm::State state)
{
    const bool running = state == Alarm::State::Running;
    ui->playPauseButton->setIcon(
        QIcon(running ? QStringLiteral(":/icons/pastel-pause") : QStringLiteral(":/icons/pastel-play")));
    ui->playPauseButton->setText(running ? tr("Pause") : tr("Start")); // read by screen readers
    ui->playPauseButton->setToolTip(running ? tr("Pause") : tr("Start"));
}

void CountdownWindow::updateTrayIcon(double fractionLeft)
{
    // Only redraw on noticeable change (or on reset back to full).
    if (m_trayFractionLeft >= 0 && fractionLeft < m_trayFractionLeft && m_trayFractionLeft - fractionLeft < 0.05) {
        return;
    }
    m_trayFractionLeft = fractionLeft;

    const QPixmap source = themedAppPixmap(m_theme);
    QPixmap image(source.size());
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setOpacity(0.25);
    painter.drawPixmap(0, 0, source);

    // Elapsed part is drawn opaque, filling up from the bottom.
    const qreal y = image.height() * fractionLeft;
    QPainterPath clip;
    clip.addRect(0, y, image.width(), image.height() - y);
    painter.setClipPath(clip);
    painter.setOpacity(1);
    painter.drawPixmap(0, 0, source);
    painter.setClipping(false);

    QPen pen(Qt::white);
    pen.setWidth(5);
    painter.setPen(pen);
    painter.drawLine(QPointF(0, y), QPointF(image.width(), y));
    painter.end();

    const QIcon icon(image);
    m_trayIcon->setIcon(icon);
    setWindowIcon(icon);
}

void CountdownWindow::saveSettings() const
{
    AppSettings &settings = m_settings;
    // Only remember geometry the user chose. Automatic adjustments made in
    // loadSettings() (fitting onto the screen, cascade offset) are not saved,
    // so the original position comes back when e.g. the monitor is reconnected.
    if (size() != m_placedSize) {
        settings.setValue(settingsSizeKey, size());
    }
    if (pos() != m_placedPos) {
        settings.setValue(settingsPosKey, pos());
    }
}

void CountdownWindow::loadSettings()
{
    const AppSettings &settings = m_settings;
    const QRect saved(settings.value(settingsPosKey, QPoint(250, 250)).toPoint(),
                      settings.value(settingsSizeKey, defaultWindowSize).toSize());

    // The saved position may be off-screen (monitor unplugged, resolution or
    // taskbar changed): move it by the shortest distance so it is fully visible.
    QList<QRect> visibleAreas;
    for (const QScreen *screen : QGuiApplication::screens()) {
        visibleAreas.append(screen->availableGeometry());
    }
    if (visibleAreas.isEmpty()) {
        visibleAreas.append(QRect(0, 0, 1024, 768)); // no screen info at all; be defensive
    }
    const VisiblePlacement visible
        = fitIntoVisibleArea(saved.isValid() ? saved : QRect(QPoint(250, 250), defaultWindowSize), visibleAreas);

    // Offset this window if other tTime windows (in this or other processes) are open.
    resize(visible.geometry.size());
    m_placedSize = size(); // after resize(): the widget may enforce a minimum size
    m_placedPos = cascadePosition(visible.geometry.topLeft(), m_placedSize, visible.area, m_slot.index());
    move(m_placedPos);
}
