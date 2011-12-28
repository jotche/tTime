#ifndef TIMERFINISHEDDIALOG_H
#define TIMERFINISHEDDIALOG_H

#include <QtGui>
#include "tea-alarm.h"
#include "ui_timer-finished-dialog.h"

class TimerFinishedDialog : public QWidget
{
	Q_OBJECT

public:
	TimerFinishedDialog(QWidget *parent, TeaAlarm *teaAlarm);
	~TimerFinishedDialog();


protected:
	void paintEvent(QPaintEvent *event);


private slots:
	void on_closeButton_clicked();
	void on_newTimerButton_clicked();
	void updateTrayIcon();


signals:
	void closed(QString alarmId);
	void createNewAlarm();


private:
	Ui::TimerFinishedDialogClass ui;

	TeaAlarm *teaAlarm;
	QSound *alarmSound;

	QMenu *trayIconMenu;
	QAction *closeAction;
	QAction *createNewAlarmAction;

	QSystemTrayIcon *trayIcon;
	QIcon *trayIconImage;
	QTimer *trayIconUpdateTimer;
	QVector<QIcon*> *rotationIcons;
	int trayIconRotationState;
	int trayIconRotationDir;

	void createTrayRotationIcons();
	void initTrayIcon();
};

#endif // TIMERFINISHEDDIALOG_H
