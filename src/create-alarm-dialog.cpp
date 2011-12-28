#include <QtGui>
#include "create-alarm-dialog.h"
#include "tea-alarm.h"

CreateAlarmDialog::CreateAlarmDialog(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);
}

CreateAlarmDialog::CreateAlarmDialog(QWidget *parent, QString defaultValue)
	: QWidget(parent)
{
	ui.lineEdit->setText(defaultValue);
}

CreateAlarmDialog::~CreateAlarmDialog()
{

}

void CreateAlarmDialog::on_startAlarmButton_clicked()
{
	createAlarm();
}

void CreateAlarmDialog::on_lineEdit_returnPressed()
{
	createAlarm();
}

void CreateAlarmDialog::createAlarm()
{
	QString alarmText = ui.lineEdit->text();
	ui.lineEdit->clear();
	TeaAlarm *teaAlarm = createAlarm(alarmText);
	emit alarmCreated(*teaAlarm);
}

TeaAlarm* CreateAlarmDialog::createAlarm(QString aDuration)
{
	//QRegExp rx("(?:(\\d+)h)?\\s*(?:(\\d+)(?:m|min))?\\s*(?:(\\d+)(?:s|sec))?");
	QRegExp rx("\\s*(?:(\\d+)h)?\\s*(?:(\\d+)(?:m|min))?\\s*(?:(\\d+)(?:s|sec))?(?:\\s+(\\S.+\\S)?\\s*)?");
	int pos = rx.indexIn(aDuration);
	QStringList list = rx.capturedTexts();

	qlonglong duration = 0;
	if (list.size() > 1) {
		int h = list[1].toInt();
		duration += (h * 60 * 60);
	}
	if (list.size() > 2) {
		int m = list[2].toInt();
		duration += (m * 60);
	}
	if (list.size() > 3) {
		int s = list[3].toInt();
		duration += (s);
	}

	QString alarmMessage = "";
	if (list.size() > 4) {
		alarmMessage = list[4];
	}

	QString id = QString::number(QDateTime::currentDateTime().toTime_t())
		+ "_" + QString::number(QDateTime::currentDateTime().time().msec());

	TeaAlarm *teaAlarm = new TeaAlarm(0, duration, id);
	teaAlarm->setMessage(alarmMessage);

	return teaAlarm;
}
