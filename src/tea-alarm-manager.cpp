#include <QtCore>
#include <QtGui>
#include "tea-alarm-manager.h"
#include "tea-alarm.h"
#include "timer-finished-dialog.h"
#include "alarm-holder.h"

TeaAlarmManager::TeaAlarmManager(QObject *parent, QString alarmDescription)
	: QObject(parent)
{
	createAlarmDialog = new CreateAlarmDialog(0);
	connect(createAlarmDialog, SIGNAL(alarmCreated(TeaAlarm&)), this, SLOT(addAlarm(TeaAlarm&)));

	teaAlarms = new QVector<AlarmHolder*>();

	if (!alarmDescription.isEmpty()) {
		TeaAlarm *alarm = createAlarmDialog->createAlarm(alarmDescription);
		addAlarm(*alarm);
	} else {
		createAlarmDialog->show();
	}
}

TeaAlarmManager::~TeaAlarmManager()
{

}

void TeaAlarmManager::showCreateAlarmDialog()
{
	createAlarmDialog->show();
}

void TeaAlarmManager::addAlarm(TeaAlarm &teaAlarm)
{
	createAlarmDialog->hide();

	AlarmHolder *alarmHolder = new AlarmHolder(&teaAlarm);
	alarmHolder->displayProgressDialog();
	teaAlarms->append(alarmHolder);

	connect(&teaAlarm, SIGNAL(finished(QString)), this, SLOT(onAlarmFinished(QString)));
	connect(&teaAlarm, SIGNAL(closed(QString)), this, SLOT(onAlarmClosed(QString)));
	connect(&teaAlarm, SIGNAL(createNewAlarm()), this, SLOT(showCreateAlarmDialog()));
}

AlarmHolder* TeaAlarmManager::getAlarmHolder(QString alarmId)
{
	AlarmHolder *alarmHolder = 0;
	for (int i = 0; i < teaAlarms->size(); i++) {
		AlarmHolder *holder = teaAlarms->at(i);
		if (alarmId == holder->alarm()->getId()) {
			alarmHolder = holder;
			break;
		}
	}

	return alarmHolder;
}

void TeaAlarmManager::onAlarmFinished(QString alarmId)
{
	AlarmHolder *alarmHolder = getAlarmHolder(alarmId);

	TimerFinishedDialog *dialog = alarmHolder->displayAlarmFinishedDialog();
	connect(dialog, SIGNAL(closed(QString)), this, SLOT(onAlarmFinishedDialogClosed(QString)));
}

void TeaAlarmManager::onAlarmFinishedDialogClosed(QString alarmId)
{
	removeAlarm(alarmId);
}

void TeaAlarmManager::onAlarmClosed(QString alarmId)
{
	removeAlarm(alarmId);
}

void TeaAlarmManager::removeAlarm(QString alarmId)
{
	AlarmHolder *alarmHolder = getAlarmHolder(alarmId);

	for (int i = 0; i < teaAlarms->size(); i++) {
		AlarmHolder *holder = teaAlarms->at(i);
		if (alarmId == holder->alarm()->getId()) {
			teaAlarms->remove(i);
			break;
		}
	}

	delete alarmHolder;

	if (teaAlarms->size() <= 0 && !createAlarmDialog->isVisible()) {
		delete createAlarmDialog;
        qApp->quit();
	}
}
