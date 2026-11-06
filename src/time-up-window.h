#pragma once

#include "theme.h"

#include <QElapsedTimer>
#include <QIcon>
#include <QWidget>

#include <memory>
#include <vector>

class QSoundEffect;
class QSystemTrayIcon;
class QTimer;
class Alarm;
class TreeSway;
struct AppConfig;

namespace Ui {
class TimeUpWindow;
}

// "Time is up" window: plays the alarm sound and wiggles the tray icon until dismissed.
class TimeUpWindow : public QWidget
{
    Q_OBJECT

public:
    TimeUpWindow(const Alarm &alarm, const AppConfig &config, QWidget *parent = nullptr);
    ~TimeUpWindow() override;

signals:
    void dismissed(); // Close button, tray "Close", or the window was closed
    void newAlarmRequested(); // "New Timer" button or tray "New Alarm" (also dismisses)

protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void dismiss();
    void requestNewAlarm();
    void createTrayIcon();
    void createAnimationFrames();
    void advanceTrayAnimation();
    void advanceTree();

    std::unique_ptr<Ui::TimeUpWindow> ui;
    Theme m_theme;

    QSoundEffect *m_sound = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QTimer *m_animationTimer = nullptr;

    std::unique_ptr<TreeSway> m_treeSway; // null when animations are off
    QTimer *m_treeTimer = nullptr;
    QElapsedTimer m_treeClock;

    std::vector<QIcon> m_animationFrames; // one ringing cycle, looped
    std::size_t m_animationFrame = 0;
    bool m_dismissed = false;
};
