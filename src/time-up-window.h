#pragma once

#include "theme.h"

#include <QElapsedTimer>
#include <QIcon>
#include <QList>
#include <QScopedPointer>
#include <QSystemTrayIcon>
#include <QWidget>

class QSound;
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
    TimeUpWindow(const Alarm &alarm, const AppConfig &config, QWidget *parent = 0);
    ~TimeUpWindow();

signals:
    void dismissed(); // Close button, tray "Close", or the window was closed
    void newAlarmRequested(); // "New Timer" button or tray "New Alarm" (also dismisses)

protected:
    void paintEvent(QPaintEvent *event);
    void closeEvent(QCloseEvent *event);

private slots:
    void dismiss();
    void requestNewAlarm();
    void onTrayIconActivated();
    void advanceTrayAnimation();
    void advanceTree();

private:
    void createTrayIcon();
    void createAnimationFrames();
    void playSound(const QString &sound);

    QScopedPointer<Ui::TimeUpWindow> ui;
    Theme m_theme;

    QSound *m_sound;
    QSystemTrayIcon *m_trayIcon;
    QTimer *m_animationTimer;

    QScopedPointer<TreeSway> m_treeSway; // null when animations are off
    QTimer *m_treeTimer;
    QElapsedTimer m_treeClock;

    QList<QIcon> m_animationFrames; // one ringing cycle, looped
    int m_animationFrame;
    bool m_dismissed;
};
