#include <QtGui/QApplication>
#include <QWindowsVistaStyle>
#include <QCleanlooksStyle>
#include "src/countdown-widget.h"
#include "src/tea-clock.h"
#include "src/create-alarm-dialog.h"
#include "src/tea-alarm-progress-dialog.h"
#include "src/tea-alarm-manager.h"

//#include "ttime-main-window.h"

using namespace std;

int main(int argc, char *argv[])
{

//test
        //QApplication::setStyle(new QCleanlooksStyle());
        QApplication a(argc, argv);

//	TeaClock w(0, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint /*| Qt::Tool | Qt::WindowStaysOnTopHint*/);
//	w.setStyleSheet("QToolButton {\
//border: 1px none;\
//padding: 0 0 0 1;\
//margin: 0 0 0 0;\
//}\
//\
//QToolButton:hover, QToolButton::pressed, QToolButton::checked {\
//background-color: rgba(141, 178, 206, 150);\
//border: 1px solid rgba(186, 209, 225, 220);\
//padding: 0 0 0 1;\
//margin: 0 0 0 0;\
//}");
//	w.show();

//	QString id = QString::number(QDateTime::currentDateTime().toTime_t()) + "_" + QString::number(QDateTime::currentDateTime().time().msec());
//	TeaAlarm *teaAlarm = new TeaAlarm(0, 1000, id);
//	teaAlarm->setDuration("12s");
//	teaAlarm->setMessage(id);
//
//	TeaAlarmProgressDialog w(
//		0,
//		Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint /*| Qt::Tool | Qt::WindowStaysOnTopHint*/,
//		teaAlarm);
//	w.setStyleSheet("QToolButton {\
//border: 1px none;\
//padding: 0 0 0 1;\
//margin: 0 0 0 0;\
//}\
//\
//QToolButton:hover, QToolButton::pressed, QToolButton::checked {\
//background-color: rgba(141, 178, 206, 150);\
//border: 1px solid rgba(186, 209, 225, 220);\
//padding: 0 0 0 1;\
//margin: 0 0 0 0;\
//}");
//	w.show();
//
//	if (argc > 1) {
//		CreateAlarmDialog *createDialog = new CreateAlarmDialog(0, QString(argv[1]));
//		createDialog->show();
//	}

        QString s = "";
        for (int i = 1; i < argc; i++) {
                if (i > 1) {
                        s += " ";
                }
                s += argv[i];
        }

        TeaAlarmManager *manager = new TeaAlarmManager(0, s);

        return a.exec();
}


/*int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TTimeMainWindow w;
    w.show();

    return a.exec();
}*/
