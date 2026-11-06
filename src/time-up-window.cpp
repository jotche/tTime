#include "time-up-window.h"

#include "alarm.h"
#include "app-config.h"
#include "bell-animation.h"
#include "tree-sway.h"
#include "ui_time-up-window.h"
#include "wav-info.h"
#include "window-utils.h"

#include <QAction>
#include <QCloseEvent>
#include <QLoggingCategory>
#include <QMenu>
#include <QPainter>
#include <QSoundEffect>
#include <QSystemTrayIcon>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(lcSound, "ttime.sound", QtWarningMsg)

namespace {

constexpr auto alarmSoundTargetLength = 7s;

constexpr auto treeFrameInterval = 50ms;

// Loops needed to ring for about alarmSoundTargetLength, in whole loops.
int alarmSoundLoopCount(const QUrl &sound)
{
    const QString path = sound.scheme() == QLatin1String("qrc") ? QLatin1Char(':') + sound.path() : sound.toLocalFile();
    const auto clip = wavDuration(path);
    if (!clip) {
        qCWarning(lcSound) << "Cannot read the length of the alarm sound; playing it once" << sound;
        return 1;
    }
    return loopsForDuration(*clip, alarmSoundTargetLength);
}

} // namespace

TimeUpWindow::TimeUpWindow(const Alarm &alarm, const AppConfig &config, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , ui(std::make_unique<Ui::TimeUpWindow>())
    , m_theme(config.theme)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->timerLabel->setText(alarm.message());
    // Same font as the message in the countdown window, at its own size.
    QFont messageFont = config.messageFont;
    messageFont.setPointSize(config.timeUpMessagePointSize);
    ui->timerLabel->setFont(messageFont);
    // The tree artwork has a translucent fill drawn for the blue theme.
    const QImage picture = recolorForTheme(QImage(QStringLiteral(":/images/tree02")), m_theme);
    ui->label->setPixmap(QPixmap::fromImage(picture));
    if (config.animations) {
        auto sway = std::make_unique<TreeSway>(picture);
        if (sway->isValid()) {
            m_treeSway = std::move(sway);
            m_treeTimer = new QTimer(this);
            m_treeTimer->setInterval(treeFrameInterval);
            connect(m_treeTimer, &QTimer::timeout, this, &TimeUpWindow::advanceTree);
            m_treeClock.start();
            m_treeTimer->start();
        }
    }
    setWindowTitle(alarm.message().isEmpty() ? tr("Time is up") : alarm.message());

    connect(ui->closeButton, &QPushButton::clicked, this, &TimeUpWindow::dismiss);
    connect(ui->newTimerButton, &QPushButton::clicked, this, &TimeUpWindow::requestNewAlarm);

    createAnimationFrames();
    createTrayIcon();

    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(100ms);
    connect(m_animationTimer, &QTimer::timeout, this, &TimeUpWindow::advanceTrayAnimation);
    if (config.animations) {
        m_animationTimer->start(); // otherwise the tray shows the resting bell
    }

    if (config.alarmSound.isEmpty()) {
        return; // silent (e.g. in tests)
    }
    m_sound = new QSoundEffect(this);
    m_sound->setSource(config.alarmSound);
    m_sound->setLoopCount(alarmSoundLoopCount(config.alarmSound));
    // Diagnostics: QT_LOGGING_RULES="ttime.sound.debug=true" shows playback progress.
    connect(m_sound, &QSoundEffect::statusChanged, this, [this] {
        if (m_sound->status() == QSoundEffect::Error) {
            qCWarning(lcSound) << "Cannot play alarm sound" << m_sound->source();
        } else {
            qCDebug(lcSound) << "status" << m_sound->status();
        }
    });
    connect(m_sound, &QSoundEffect::playingChanged, this, [this] {
        qCDebug(lcSound) << "playing" << m_sound->isPlaying() << "loops left" << m_sound->loopsRemaining();
    });
    m_sound->play();
}

TimeUpWindow::~TimeUpWindow() = default;

void TimeUpWindow::createTrayIcon()
{
    auto *newAlarmAction = new QAction(tr("&New Alarm"), this);
    connect(newAlarmAction, &QAction::triggered, this, &TimeUpWindow::requestNewAlarm);

    auto *closeAction = new QAction(themedAppIcon(m_theme), tr("&Close"), this);
    connect(closeAction, &QAction::triggered, this, &TimeUpWindow::dismiss);

    auto *menu = new QMenu(this);
    menu->addAction(newAlarmAction);
    menu->addSeparator();
    menu->addAction(closeAction);

    m_trayIcon = ::createTrayIcon(this, m_animationFrames.front(), menu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this] { showAndActivate(this); });
}

void TimeUpWindow::createAnimationFrames()
{
    const QPixmap source = themedAppPixmap(m_theme);

    // Ringing bell: the colored background stays still, the bell swings from the
    // top of its handle and the clapper follows a little later and further.
    const BellAnimation bell = themedBellAnimation(m_theme);
    if (bell.isValid()) {
        constexpr int frames = 12; // one swing cycle (frames x timer interval)
        for (int i = 0; i < frames; ++i) {
            const double phase = 2.0 * std::numbers::pi * i / frames;
            const double bellAngle = 12.0 * std::sin(phase);
            const double clapperAngle = 16.0 * std::sin(phase - 0.9);
            m_animationFrames.emplace_back(QPixmap::fromImage(bell.frame(bellAngle, clapperAngle)));
        }
        return;
    }

    // Fallback for an icon without a separable bell: rock the whole icon.
    const QPointF center(source.width() / 2.0, source.height() / 2.0);
    for (const double angle : {0.0, 10.0, 20.0, 10.0, 0.0, -10.0, -20.0, -10.0}) {
        QPixmap image(source.size());
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.translate(center);
        painter.rotate(angle);
        painter.translate(-center);
        painter.drawPixmap(0, 0, source);
        painter.end();
        m_animationFrames.emplace_back(image);
    }
}

void TimeUpWindow::advanceTrayAnimation()
{
    m_animationFrame = (m_animationFrame + 1) % m_animationFrames.size();
    const QIcon &icon = m_animationFrames[m_animationFrame];
    m_trayIcon->setIcon(icon);
    setWindowIcon(icon);
}

void TimeUpWindow::advanceTree()
{
    // Driven by a clock, not by counting ticks, so late ticks do not slow the breeze.
    const double seconds = static_cast<double>(m_treeClock.elapsed()) / 1000.0;
    const QImage frame
        = m_treeSway->frame(TreeSway::breezeSway * TreeSway::breeze(seconds),
                            2 * std::numbers::pi * seconds / TreeSway::leafFlutterPeriod, TreeSway::leafFlutter);
    ui->label->setPixmap(QPixmap::fromImage(frame));
}

void TimeUpWindow::dismiss()
{
    if (m_dismissed) {
        return;
    }
    m_dismissed = true;
    if (m_sound) {
        m_sound->stop();
    }
    m_animationTimer->stop();
    if (m_treeTimer) {
        m_treeTimer->stop();
    }
    m_trayIcon->hide();
    hide();
    emit dismissed();
}

void TimeUpWindow::requestNewAlarm()
{
    emit newAlarmRequested();
    dismiss();
}

void TimeUpWindow::closeEvent(QCloseEvent *event)
{
    // Alt+F4 behaves like the Close button (stops the sound, releases the alarm).
    event->accept();
    dismiss();
}

void TimeUpWindow::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    paintPanel(painter, rect(), m_theme,
               PanelStyle{.cornerRadius = 15,
                          .sideEdgeAlpha = 150,
                          .sideInsetAlpha = 100,
                          .sideLightAlpha = 80,
                          .lightBandLeft = false});
}
