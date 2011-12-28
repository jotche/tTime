#include "alarm-holder.h"
#include "tea-alarm.h"
#include "tea-alarm-progress-dialog.h"
#include "timer-finished-dialog.h"

AlarmHolder::AlarmHolder(TeaAlarm *teaAlarm):
teaAlarm(teaAlarm)
{
	alarmDisplay = NULL;
}

AlarmHolder::~AlarmHolder(void)
{
	delete teaAlarm;
    alarmDisplay->close();
    delete alarmDisplay;
}

TeaAlarm* AlarmHolder::alarm(void)
{
	return teaAlarm;
}

void AlarmHolder::displayProgressDialog(void)
{
	QString id = QString::number(QDateTime::currentDateTime().toTime_t())
		+ "_" + QString::number(QDateTime::currentDateTime().time().msec());

	Qt::WFlags flags = Qt::FramelessWindowHint
		| Qt::WindowSystemMenuHint
		/*| Qt::WindowMinMaxButtonsHint*/
		/*| Qt::Tool | Qt::WindowStaysOnTopHint*/;


	TeaAlarmProgressDialog *progressDialog = new TeaAlarmProgressDialog(
		0, flags, teaAlarm);

	progressDialog->setStyleSheet("QToolButton {\
border: 1px none;\
padding: 0 0 0 1;\
margin: 0 0 0 0;\
}\
\
QToolButton:hover, QToolButton::pressed, QToolButton::checked {\
background-color: rgba(141, 178, 206, 150);\
border: 1px solid rgba(186, 209, 225, 220);\
padding: 0 0 0 1;\
margin: 0 0 0 0;\
}");

	if (alarmDisplay != NULL) {
		delete alarmDisplay;
	}

	alarmDisplay = progressDialog;
	alarmDisplay->show();
	teaAlarm->start();
}

TimerFinishedDialog* AlarmHolder::displayAlarmFinishedDialog(void)
{
	TimerFinishedDialog *dialog = new TimerFinishedDialog(0, alarm());
	dialog->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

	if (alarmDisplay != NULL) {
		alarmDisplay->close();
		delete alarmDisplay;
	}

	alarmDisplay = dialog;
	alarmDisplay->show();

	return dialog;
}
