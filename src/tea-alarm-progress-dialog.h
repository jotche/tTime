#ifndef TEAALARMPROGRESSDIALOG_H
#define TEAALARMPROGRESSDIALOG_H

#include <QtGui>
#include "ui_tea-alarm-progress-dialog.h"
#include "tea-alarm.h"

class TeaAlarmProgressDialog : public QWidget
{

	Q_OBJECT


public:
	TeaAlarmProgressDialog(
		QWidget *parent,
		Qt::WFlags flags,
		TeaAlarm *aTeaAlarm);

	~TeaAlarmProgressDialog();

	void setVisible(bool visible);


private slots:
	void on_playPauseButton_clicked();
	void on_stopButton_clicked();
	void on_stayOnTopButton_clicked();
	void restart();
	void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

	void onTimeLeftChanged(int timeLeft);
	void onTeaAlarmFinished();
	void quitTimer();


protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void startTimer();
    void pauseTimer();
    void stopTimer();

private:
	Ui::TeaAlarmProgressDialogClass ui;

	TeaAlarm *teaAlarm;

	QMenu *trayIconMenu;
	QSystemTrayIcon *trayIcon;

	QPoint dragPosition;
	bool resizeEvent;

	QAction *restartAction;

	QAction *minimizeAction;
	QAction *restoreAction;
	QAction *quitAction;
	QAction *createNewAlarmAction;

	qreal percentPassed;

	void createActions();
	void createTrayIcon();
	void loadSettings();
	void saveSettings();
	void updateTrayIcon(qreal percentPassed);
};

#endif // TEAALARMPROGRESSDIALOG_H
