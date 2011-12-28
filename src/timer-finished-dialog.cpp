#include <QtGui>
#include "timer-finished-dialog.h"
#include "tea-alarm.h"

TimerFinishedDialog::TimerFinishedDialog(QWidget *parent, TeaAlarm *aTeaAlarm)
: QWidget(parent), teaAlarm(aTeaAlarm), trayIconRotationState(0), trayIconRotationDir(1), trayIconImage(0)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_TranslucentBackground);

	connect(this, SIGNAL(createNewAlarm()), teaAlarm, SIGNAL(createNewAlarm()));

	// init message
	ui.timerLabel->setText(teaAlarm->getMessage());

	// init tray icon
	createTrayRotationIcons();

	trayIcon = new QSystemTrayIcon(this);
	updateTrayIcon();
	initTrayIcon();
	trayIcon->show();

	trayIconUpdateTimer = new QTimer(this);
	trayIconUpdateTimer->setInterval(200);
	trayIconUpdateTimer->setSingleShot(false);

	connect(trayIconUpdateTimer, SIGNAL(timeout()), this, SLOT(updateTrayIcon()));
	trayIconUpdateTimer->start();

	// init sound
    alarmSound = new QSound("Resources/sounds/submarine1.wav", this);
	alarmSound->setLoops(-1);
	alarmSound->play();
}

TimerFinishedDialog::~TimerFinishedDialog()
{
	trayIconUpdateTimer->stop();
	delete trayIconUpdateTimer;

	delete alarmSound;

	for(int i = 0; i < rotationIcons->size(); i++) {
		QIcon *icon = (*rotationIcons)[i];
		delete icon;
	}
	rotationIcons->clear();
	delete rotationIcons;
	delete trayIconImage;
}

void TimerFinishedDialog::initTrayIcon()
{
	closeAction = new QAction(tr("&Close"), this);
	closeAction->setIcon(QIcon(QString::fromUtf8(":/icons/alarm")));
	connect(closeAction, SIGNAL(triggered()), this, SLOT(on_closeButton_clicked()));

	createNewAlarmAction = new QAction(tr("&New Alarm"), this);
	//createNewAlarmAction->setIcon(ui.restartButton->icon());
    connect(createNewAlarmAction, SIGNAL(triggered()), this, SLOT(on_newTimerButton_clicked()));

	trayIconMenu = new QMenu(this);
	trayIconMenu->addAction(createNewAlarmAction);
	trayIconMenu->addSeparator();
	trayIconMenu->addAction(closeAction);

	trayIcon->setContextMenu(trayIconMenu);
}

void TimerFinishedDialog::on_closeButton_clicked() {
	alarmSound->stop();
	emit closed(teaAlarm->getId());
}

void TimerFinishedDialog::on_newTimerButton_clicked() {
	emit createNewAlarm();
	on_closeButton_clicked();
}

void TimerFinishedDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

	QColor cstart = QColor(3, 66, 124, 235);
	QColor cinset = QColor(69, 134, 186, 190);
	QColor cmiddle = QColor(164, 212, 238, 150);

	QColor cstartv = QColor(3, 66, 124, 150);
	QColor cinsetv = QColor(69, 134, 186, 100);
	QColor cmiddlev = QColor(164, 212, 238, 80);

	qreal ins = 20;
	qreal p1(ins / height());
	qreal p2(1.0 - p1);

	QLinearGradient hlgradient(0, 0, 0, height());
	hlgradient.setColorAt(0.0, cstart);
	hlgradient.setColorAt(p1, cinset);
	hlgradient.setColorAt(0.5, cmiddle);
	hlgradient.setColorAt(p2, cinset);
	hlgradient.setColorAt(1.0, cstart);


	p1 = ins / width();
	p2 = 1 - p1;

	QLinearGradient vlgradient(0,0, width(), 0);
	vlgradient.setColorAt(0.0, cstartv);
	vlgradient.setColorAt(p1, cinsetv);
	vlgradient.setColorAt(0.5, cmiddlev);
	vlgradient.setColorAt(p2, cinsetv);
	vlgradient.setColorAt(1.0, cstartv);


	int cornerRaduis = 15;
	painter.setPen(QColor(0, 0, 0, 32));
	painter.setBrush(hlgradient);
	painter.drawRoundedRect(0, 0, width(), height(),
		cornerRaduis, cornerRaduis, Qt::AbsoluteSize);

	painter.setPen(QColor(220, 220, 220, 100));
	painter.setBrush(vlgradient);
	painter.drawRoundedRect(0, 0, width(), height(),
		cornerRaduis, cornerRaduis, Qt::AbsoluteSize);

	painter.end();
}

void TimerFinishedDialog::updateTrayIcon()
{
	if (trayIconRotationState == 2 || trayIconRotationState == -2) {
		trayIconRotationDir *= -1;
	}

	trayIconRotationState = trayIconRotationState + trayIconRotationDir;

	QIcon *icon = rotationIcons->at(trayIconRotationState + 2);
	trayIcon->setIcon(*icon);
	setWindowIcon(*icon);
}

void TimerFinishedDialog::createTrayRotationIcons()
{
	QPixmap pm = QPixmap(QString::fromUtf8(":/icons/alarm"));
	qreal w = (qreal) pm.size().width();
	qreal h = (qreal) pm.size().height();
	qreal cx = w / 2;
	qreal cy = h / 2;

	rotationIcons = new QVector<QIcon*>();
	for (int i = -2; i <= 2; i++) {
		QPixmap image(pm.size());
		image.fill(Qt::transparent);

		QPainter imagePainter(&image);
		imagePainter.translate(cx, cy);
		imagePainter.rotate(i * 10);
		imagePainter.translate(-cx, -cy);
		imagePainter.drawPixmap(0, 0, pm);

		imagePainter.end();

		QIcon *icon = new QIcon();
		icon->addPixmap(image, QIcon::Normal, QIcon::Off);

		rotationIcons->append(icon);
	}
}
