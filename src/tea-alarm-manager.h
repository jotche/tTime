#ifndef TEAALARMMANAGER_H
#define TEAALARMMANAGER_H

#include <QtCore>
#include "create-alarm-dialog.h"
#include "alarm-holder.h"

class TeaAlarmManager : public QObject
{

Q_OBJECT


public:
	TeaAlarmManager(QObject *parent, QString alarmDescription);
	~TeaAlarmManager();


private slots:
	void addAlarm(TeaAlarm &teaAlarm);
	void onAlarmFinished(QString alarmId);
	void onAlarmFinishedDialogClosed(QString alarmId);
	void onAlarmClosed(QString alarmId);
	void showCreateAlarmDialog();


private:
	CreateAlarmDialog *createAlarmDialog;
	QVector<AlarmHolder*> *teaAlarms;

	AlarmHolder* getAlarmHolder(QString alarmId);
	void removeAlarm(QString alarmId);

};

#endif // TEAALARMMANAGER_H
