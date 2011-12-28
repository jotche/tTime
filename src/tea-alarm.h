#ifndef TEAALARM_H
#define TEAALARM_H

#include <QtCore>
#include <QObject>

class TeaAlarm : public QObject
{

    Q_OBJECT


public:
    enum State {
        NotRunning,
        Paused,
        Running
    };

	TeaAlarm(QObject *parent, qlonglong duration, QString id);
	~TeaAlarm();

	State getState();

	qlonglong getDuration();
	void setDuration(qlonglong durationSecs);
	void setDuration(QString durationString);

	QString getMessage();
	void setMessage(QString message);

	QString getId();
	void setId(QString id);


public slots:
	void start();
	void pause();
	void stop();
	void updateTimeLeft();
	void finish();
	void close();


signals:
	void timeLeftChanged(int msecLeft);
	void finished(QString timerId);
	void closed(QString timerId);
	void createNewAlarm();


private:
	QString id;

	qlonglong duration;
	qlonglong currentDuration;

	QString message;

	State state;

	QDateTime startTime;
	QTimer *alarmTimer;
	QTimer *updateTimer;
};

#endif // TEAALARM_H
