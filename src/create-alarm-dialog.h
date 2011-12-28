#ifndef CREATEALARMDIALOG_H
#define CREATEALARMDIALOG_H

#include <QtGui>
#include "ui_create-alarm-dialog.h"
#include "tea-alarm.h"

class CreateAlarmDialog : public QWidget
{
	Q_OBJECT

public:
	CreateAlarmDialog(QWidget *parent = 0);
	CreateAlarmDialog(QWidget *parent, QString defaultValue);
	~CreateAlarmDialog();

	TeaAlarm* createAlarm(QString aDuration);


private slots:
	void on_startAlarmButton_clicked();
	void on_lineEdit_returnPressed();


signals:
	void alarmCreated(TeaAlarm &alarm);


private:
	Ui::CreateAlarmDialogClass ui;

	void createAlarm();

};

#endif // CREATEALARMDIALOG_H
