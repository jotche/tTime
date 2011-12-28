#ifndef TEACLOCK_H
#define TEACLOCK_H

#include <QtGui/QWidget>
#include <QSystemTrayIcon>
#include <QDateTime>
#include "ui_tea-clock.h"

class TeaClock : public QWidget
{
	Q_OBJECT

public:
	TeaClock(QWidget *parent = 0, Qt::WFlags flags = 0);
	~TeaClock();

	void setVisible(bool visible);


protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void startTimer();
    void pauseTimer();
    void stopTimer();


private slots:
	void on_playPauseButton_clicked();
	void on_stopButton_clicked();
	void on_stayOnTopButton_clicked();
	void restart();
	void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

	//void on_minimizeButton_clicked();
	//void on_exitButton_clicked();


private:
	void createActions();
	void createTrayIcon();

	Ui::TeaClockClass ui;

	QMenu *trayIconMenu;
	QSystemTrayIcon *trayIcon;

	QPoint dragPosition;
	bool resizeEvent;
	qlonglong requestedDuration;
	qlonglong duration;
	QDateTime startTime;
	QDateTime endTime;
	bool running;

	QAction *restartAction;

	QAction *minimizeAction;
	QAction *restoreAction;
	QAction *quitAction;
};

#endif // TEACLOCK_H
