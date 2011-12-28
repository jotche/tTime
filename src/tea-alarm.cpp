#include <QtGui/QApplication>
#include "tea-alarm.h"

TeaAlarm::TeaAlarm(QObject *parent, qlonglong aDuration, QString aId)
: QObject(parent),
duration(aDuration),
currentDuration(aDuration),
state(TeaAlarm::NotRunning),
message(QString("")),
id(aId)
{
	alarmTimer = new QTimer(this);
	alarmTimer->setInterval(1000 * duration);
	alarmTimer->setSingleShot(true);

	updateTimer = new QTimer(this);
	updateTimer->setInterval(500);
	updateTimer->setSingleShot(false);

	connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateTimeLeft()));
	connect(alarmTimer, SIGNAL(timeout()), this, SLOT(finish()));
}

TeaAlarm::~TeaAlarm()
{
	delete alarmTimer;
	delete updateTimer;
}

TeaAlarm::State TeaAlarm::getState()
{
	return state;
}

qlonglong TeaAlarm::getDuration()
{
	return duration;
}

void TeaAlarm::setDuration(qlonglong aDuration)
{
	duration = aDuration;
	currentDuration = aDuration;
}

void TeaAlarm::setDuration(QString aDuration)
{
	QRegExp rx("\\s*(?:(\\d+)h)?\\s*(?:(\\d+)(?:m|min))?\\s*(?:(\\d+)(?:s|sec))?\\s*");
	int pos = rx.indexIn(aDuration);
	QStringList list = rx.capturedTexts();

	qlonglong durantionTemp = 0;
	if (list.size() > 1) {
		int h = list[1].toInt();
		durantionTemp += (h * 60 * 60);
	}
	if (list.size() > 2) {
		int m = list[2].toInt();
		durantionTemp += (m * 60);
	}
	if (list.size() > 3) {
		int s = list[3].toInt();
		durantionTemp += (s);
	}

	setDuration(durantionTemp);
}

QString TeaAlarm::getMessage()
{
	return message;
}

void TeaAlarm::setMessage(QString aMessage)
{
	message = aMessage;
}

QString TeaAlarm::getId()
{
	return id;
}

void TeaAlarm::setId(QString aId)
{
	id = aId;
}

void TeaAlarm::start()
{
	state = TeaAlarm::Running;

	startTime = QDateTime::currentDateTime();

	alarmTimer->setInterval(1000 * currentDuration);

	alarmTimer->start();
	updateTimer->start();

	updateTimeLeft();
}

void TeaAlarm::pause()
{
	state = TeaAlarm::Paused;

	int elapsedSeconds = startTime.secsTo(QDateTime::currentDateTime());
	currentDuration = currentDuration - elapsedSeconds;

	alarmTimer->stop();
	updateTimer->stop();

	updateTimeLeft();
}

void TeaAlarm::stop()
{
	state = TeaAlarm::NotRunning;

	currentDuration = duration;

	alarmTimer->stop();
	updateTimer->stop();

	updateTimeLeft();
}

void TeaAlarm::updateTimeLeft()
{
	if (state == TeaAlarm::Running) {
		int elapsedSeconds = startTime.secsTo(QDateTime::currentDateTime());
		int secsLeft = currentDuration - elapsedSeconds;

		emit timeLeftChanged(secsLeft);
	} else {
		emit timeLeftChanged(currentDuration);
	}
}

void TeaAlarm::finish()
{
	emit finished(id);
}

void TeaAlarm::close()
{
	emit closed(id);
}
