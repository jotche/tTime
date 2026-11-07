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
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QPainter>
#include <QSound>
#include <QTimer>

#include <cmath>

namespace {

const double pi = 3.14159265358979323846;

const qint64 alarmSoundTargetLengthMs = 7000;
const int treeFrameIntervalMs = 50;

// Loops needed to ring for about alarmSoundTargetLengthMs, in whole loops.
int alarmSoundLoopCount(const QString &file)
{
    qint64 clipMs = 0;
    if (!wavDuration(file, &clipMs)) {
        qWarning("Cannot read the length of the alarm sound %s; playing it once", qPrintable(file));
        return 1;
    }
    return loopsForDuration(clipMs, alarmSoundTargetLengthMs);
}

} // namespace

TimeUpWindow::TimeUpWindow(const Alarm &alarm, const AppConfig &config, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , ui(new Ui::TimeUpWindow)
    , m_theme(config.theme)
    , m_sound(0)
    , m_trayIcon(0)
    , m_animationTimer(0)
    , m_treeTimer(0)
    , m_animationFrame(0)
    , m_dismissed(false)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->timerLabel->setText(alarm.message());
    // Same font as the message in the countdown window, at its own size.
    QFont messageFont = config.messageFont;
    messageFont.setPointSize(config.timeUpMessagePointSize);
    ui->timerLabel->setFont(messageFont);
    // The tree artwork has a translucent fill drawn for the blue theme.
    const QImage picture = recolorForTheme(QImage(QLatin1String(":/images/tree02")), m_theme);
    ui->label->setPixmap(QPixmap::fromImage(picture));
    if (config.animations) {
        QScopedPointer<TreeSway> sway(new TreeSway(picture));
        if (sway->isValid()) {
            m_treeSway.reset(sway.take());
            m_treeTimer = new QTimer(this);
            m_treeTimer->setInterval(treeFrameIntervalMs);
            connect(m_treeTimer, SIGNAL(timeout()), this, SLOT(advanceTree()));
            m_treeClock.start();
            m_treeTimer->start();
        }
    }
    setWindowTitle(alarm.message().isEmpty() ? tr("Time is up") : alarm.message());

    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(dismiss()));
    connect(ui->newTimerButton, SIGNAL(clicked()), this, SLOT(requestNewAlarm()));

    createAnimationFrames();
    createTrayIcon();

    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(100);
    connect(m_animationTimer, SIGNAL(timeout()), this, SLOT(advanceTrayAnimation()));
    if (config.animations) {
        m_animationTimer->start(); // otherwise the tray shows the resting bell
    }

    if (!config.alarmSound.isEmpty()) { // empty: silent (e.g. in tests)
        playSound(config.alarmSound);
    }
}

TimeUpWindow::~TimeUpWindow()
{
}

void TimeUpWindow::playSound(const QString &file)
{
    if (!QSound::isAvailable()) {
        qWarning("Cannot play the alarm sound: no sound support");
        return;
    }
    // QSound plays files only, so the sound is shipped in the Resources folder next
    // to tTime.exe (see externalResourcePath()); without it the alarm is silent.
    if (!QFile::exists(file)) {
        qWarning("Alarm sound not found: %s", qPrintable(QDir::toNativeSeparators(file)));
        return;
    }
    m_sound = new QSound(file, this);
    m_sound->setLoops(alarmSoundLoopCount(file));
    m_sound->play();
}

void TimeUpWindow::createTrayIcon()
{
    QAction *newAlarmAction = new QAction(tr("&New Alarm"), this);
    connect(newAlarmAction, SIGNAL(triggered()), this, SLOT(requestNewAlarm()));

    QAction *closeAction = new QAction(themedAppIcon(m_theme), tr("&Close"), this);
    connect(closeAction, SIGNAL(triggered()), this, SLOT(dismiss()));

    QMenu *menu = new QMenu(this);
    menu->addAction(newAlarmAction);
    menu->addSeparator();
    menu->addAction(closeAction);

    m_trayIcon = ::createTrayIcon(this, m_animationFrames.first(), menu);
    connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onTrayIconActivated()));
}

void TimeUpWindow::onTrayIconActivated()
{
    showAndActivate(this);
}

void TimeUpWindow::createAnimationFrames()
{
    const QPixmap source = themedAppPixmap(m_theme);

    // Ringing bell: the colored background stays still, the bell swings from the
    // top of its handle and the clapper follows a little later and further.
    const BellAnimation bell = themedBellAnimation(m_theme);
    if (bell.isValid()) {
        const int frames = 12; // one swing cycle (frames x timer interval)
        for (int i = 0; i < frames; ++i) {
            const double phase = 2.0 * pi * i / frames;
            const double bellAngle = 12.0 * std::sin(phase);
            const double clapperAngle = 16.0 * std::sin(phase - 0.9);
            m_animationFrames.append(QIcon(QPixmap::fromImage(bell.frame(bellAngle, clapperAngle))));
        }
        return;
    }

    // Fallback for an icon without a separable bell: rock the whole icon.
    const QPointF center(source.width() / 2.0, source.height() / 2.0);
    const double angles[] = {0.0, 10.0, 20.0, 10.0, 0.0, -10.0, -20.0, -10.0};
    for (int i = 0; i < 8; ++i) {
        QPixmap image(source.size());
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.translate(center);
        painter.rotate(angles[i]);
        painter.translate(-center);
        painter.drawPixmap(0, 0, source);
        painter.end();
        m_animationFrames.append(QIcon(image));
    }
}

void TimeUpWindow::advanceTrayAnimation()
{
    m_animationFrame = (m_animationFrame + 1) % m_animationFrames.size();
    const QIcon &icon = m_animationFrames.at(m_animationFrame);
    m_trayIcon->setIcon(icon);
    setWindowIcon(icon);
}

void TimeUpWindow::advanceTree()
{
    // Driven by a clock, not by counting ticks, so late ticks do not slow the breeze.
    const double seconds = static_cast<double>(m_treeClock.elapsed()) / 1000.0;
    const QImage frame = m_treeSway->frame(TreeSway::breezeSway * TreeSway::breeze(seconds),
                                           2 * pi * seconds / TreeSway::leafFlutterPeriod, TreeSway::leafFlutter);
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
    PanelStyle style;
    style.cornerRadius = 15;
    style.sideEdgeAlpha = 150;
    style.sideInsetAlpha = 100;
    style.sideLightAlpha = 80;
    style.lightBandLeft = false;

    QPainter painter(this);
    paintPanel(painter, rect(), m_theme, style);
}
