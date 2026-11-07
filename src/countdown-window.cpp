#include "countdown-window.h"

#include "alarm-spec.h"
#include "app-config.h"
#include "app-settings.h"
#include "countdown-font.h"
#include "ui_countdown-window.h"
#include "window-placement.h"
#include "window-utils.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QFont>
#include <QFontMetrics>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolButton>

namespace {

// Historical key names (the window used to be called the "progress dialog"); kept so
// existing tTime.ini files keep their saved window size and position.
const char *const settingsSizeKey = "progress-dialog/size";
const char *const settingsPosKey = "progress-dialog/pos";
// Used when tTime.ini has no size; the shipped tTime.ini has the same value.
const QSize defaultWindowSize(200, 130);
const QPoint defaultWindowPos(250, 250);
// The window can be made wider but not narrower than this; its height always
// follows the content.
const int minimumWindowWidth = 200;
// The window actions sit this far from the top and right edges, ignoring the padding.
const int windowActionsInset = 8;
// Smallest gap between the action row and the window actions.
const int windowActionsGap = 8;

} // namespace

CountdownWindow::CountdownWindow(Alarm *alarm, const AppConfig &config, AppSettings &settings, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint)
    , ui(new Ui::CountdownWindow)
    , m_alarm(alarm)
    , m_theme(config.theme)
    , m_settings(settings)
    , m_trayIcon(0)
    , m_trayMenu(0)
    , m_restartAction(0)
    , m_minimizeAction(0)
    , m_restoreAction(0)
    , m_newAlarmAction(0)
    , m_quitAction(0)
    , m_drag(NoDrag)
    , m_dragStartX(0)
    , m_dragStartWidth(0)
    , m_trayFractionLeft(-1)
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

    connect(ui->playPauseButton, SIGNAL(clicked()), this, SLOT(togglePlayPause()));
    connect(ui->stopButton, SIGNAL(clicked()), m_alarm, SLOT(stop()));
    connect(ui->stayOnTopButton, SIGNAL(toggled(bool)), this, SLOT(setStayOnTop(bool)));

    connect(m_alarm, SIGNAL(timeLeftChanged(qint64)), this, SLOT(onTimeLeftChanged(qint64)));
    connect(m_alarm, SIGNAL(stateChanged(Alarm::State)), this, SLOT(onStateChanged(Alarm::State)));

    // Lock every toolbar button at its normal size. The highlight style
    // (hover/pressed/checked) adds a 1px border; without this a highlighted
    // button grew by 2px, and the layout applied that whenever the hovered
    // button's icon changed (e.g. clicking start), so the whole content jumped.
    foreach (QToolButton *button, findChildren<QToolButton *>()) {
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
    connect(m_minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    m_restoreAction = new QAction(tr("&Restore"), this);
    connect(m_restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    m_quitAction = new QAction(ui->exitButton->icon(), tr("&Quit"), this);
    m_quitAction->setObjectName(QLatin1String("quitAction"));
    connect(m_quitAction, SIGNAL(triggered()), this, SLOT(quitAlarm()));

    m_restartAction = new QAction(ui->restartButton->icon(), tr("Restart"), this);
    connect(m_restartAction, SIGNAL(triggered()), m_alarm, SLOT(restart()));

    m_newAlarmAction = new QAction(tr("New Alarm"), this);
    m_newAlarmAction->setObjectName(QLatin1String("newAlarmAction"));
    connect(m_newAlarmAction, SIGNAL(triggered()), this, SIGNAL(newAlarmRequested()));
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
    connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
            SLOT(onTrayIconActivated(QSystemTrayIcon::ActivationReason)));
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
    // Drag moves the window; Ctrl+drag changes its width (the height follows the content).
    if (event->modifiers() & Qt::ControlModifier) {
        m_drag = ResizeDrag;
        m_dragStartX = event->globalPos().x();
        m_dragStartWidth = width();
    } else {
        m_drag = MoveDrag;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
    }
    event->accept();
}

void CountdownWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_drag != NoDrag && (event->buttons() & Qt::LeftButton)) {
        if (m_drag == MoveDrag) {
            move(event->globalPos() - m_dragOffset);
        } else {
            resize(qMax(minimumWindowWidth, m_dragStartWidth + event->globalPos().x() - m_dragStartX), height());
        }
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void CountdownWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_drag = NoDrag;
    }
    QWidget::mouseReleaseEvent(event);
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
    PanelStyle style;
    style.cornerRadius = 0;
    style.sideEdgeAlpha = 200;
    style.sideInsetAlpha = 150;
    style.sideLightAlpha = 80;
    style.lightBandLeft = true;

    QPainter painter(this);
    paintPanel(painter, rect(), m_theme, style);
}

void CountdownWindow::togglePlayPause()
{
    if (m_alarm->state() == Alarm::Running) {
        m_alarm->pause();
    } else {
        m_alarm->start();
    }
}

void CountdownWindow::setStayOnTop(bool onTop)
{
    Qt::WindowFlags flags = windowFlags();
    if (onTop) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
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

void CountdownWindow::onTimeLeftChanged(qint64 secondsLeft)
{
    const QString text = formatDuration(secondsLeft);
    ui->timeLabel->setText(text);

    const qint64 total = m_alarm->duration();
    const double fractionLeft = total > 0 ? static_cast<double>(secondsLeft) / static_cast<double>(total) : 0.0;
    updateTrayIcon(fractionLeft);

    const QString title = m_alarm->message().isEmpty() ? text : text + QLatin1String(" - ") + m_alarm->message();
    m_trayIcon->setToolTip(title);
}

void CountdownWindow::onStateChanged(Alarm::State state)
{
    const bool running = state == Alarm::Running;
    ui->playPauseButton->setIcon(
        QIcon(running ? QLatin1String(":/icons/pastel-pause") : QLatin1String(":/icons/pastel-play")));
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
    // Only remember geometry the user chose. Automatic adjustments made in
    // loadSettings() (fitting onto the screen, cascade offset) are not saved,
    // so the original position comes back when e.g. the monitor is reconnected.
    if (size() != m_placedSize) {
        m_settings.setValue(QLatin1String(settingsSizeKey), size());
    }
    if (pos() != m_placedPos) {
        m_settings.setValue(QLatin1String(settingsPosKey), pos());
    }
}

void CountdownWindow::loadSettings()
{
    const QRect saved(m_settings.value(QLatin1String(settingsPosKey), defaultWindowPos).toPoint(),
                      m_settings.value(QLatin1String(settingsSizeKey), defaultWindowSize).toSize());

    // The saved position may be off-screen (monitor unplugged, resolution or
    // taskbar changed): move it by the shortest distance so it is fully visible.
    QList<QRect> visibleAreas;
    const QDesktopWidget *desktop = QApplication::desktop();
    for (int i = 0; i < desktop->screenCount(); ++i) {
        visibleAreas.append(desktop->availableGeometry(i));
    }
    if (visibleAreas.isEmpty()) {
        visibleAreas.append(QRect(0, 0, 1024, 768)); // no screen info at all; be defensive
    }
    const VisiblePlacement visible
        = fitIntoVisibleArea(saved.isValid() ? saved : QRect(defaultWindowPos, defaultWindowSize), visibleAreas);

    // Offset this window if other tTime windows (in this or other processes) are open.
    resize(visible.geometry.size());
    m_placedSize = size(); // after resize(): the widget may enforce a minimum size
    m_placedPos = cascadePosition(visible.geometry.topLeft(), m_placedSize, visible.area, m_slot.index());
    move(m_placedPos);
}
