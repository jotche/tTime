#pragma once

#include "tea-alarm.h"
#include "timer-finished-dialog.h"

class AlarmHolder
{

public:
	AlarmHolder(TeaAlarm *teaAlarm);
	~AlarmHolder(void);

public:
	void displayProgressDialog(void);
	TimerFinishedDialog* displayAlarmFinishedDialog(void);

	TeaAlarm* alarm();

private:
	TeaAlarm *teaAlarm;
	QWidget *alarmDisplay;

};
