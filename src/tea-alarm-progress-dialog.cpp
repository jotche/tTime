#include <QtGui>
#include <QSize>
#include "tea-alarm-progress-dialog.h"
#include "timer-finished-dialog.h"

TeaAlarmProgressDialog::TeaAlarmProgressDialog(QWidget *parent, Qt::WFlags flags, TeaAlarm *aTeaAlarm)
	: QWidget(parent, flags),
	teaAlarm(aTeaAlarm)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_TranslucentBackground);

	resizeEvent = false;
	percentPassed = 1;

	createActions();
	addAction(restartAction);
	addAction(minimizeAction);
	addAction(restoreAction);
	addAction(createNewAlarmAction);
	addAction(quitAction);

	createTrayIcon();

    setContextMenuPolicy(Qt::ActionsContextMenu);
    /*setToolTip(tr("Drag the clock with the left mouse button.\n"
                  "Use the right mouse button to open a context menu."));*/


	connect(teaAlarm, SIGNAL(timeLeftChanged(int)), this, SLOT(onTimeLeftChanged(int)));
	connect(teaAlarm, SIGNAL(finished(QString)), this, SLOT(onTeaAlarmFinished()));
	teaAlarm->updateTimeLeft();

        QFontDatabase::addApplicationFont(":/fonts/sansation_regular");
        QFontDatabase::addApplicationFont(":/fonts/sansation_light");
        QFontDatabase::addApplicationFont(":/fonts/telegrama");
        QFontDatabase::addApplicationFont(":/fonts/satellite");
        QFontDatabase::addApplicationFont(":/fonts/GeosansLight");
        QFont timeFont = QFont("sansation", 18, QFont::DemiBold, false);
        timeFont.setLetterSpacing(QFont::PercentageSpacing, 125);


	ui.timeLabel->setFont(timeFont);

	ui.descriptionLabel->setText(teaAlarm->getMessage());
        QFont descriptionFont = QFont("GeosansLight", 14, QFont::Normal, false);
	ui.descriptionLabel->setFont(descriptionFont);


	//QIcon trayIconImage;

	//QPixmap pm = QPixmap(QString::fromUtf8(":/icons/alarm"));

	//QPixmap image(pm.size());
	//image.fill(Qt::transparent);
 //   QPainter imagePainter(&image);
 //   //imagePainter.setRenderHint(QPainter::Antialiasing, true);
    ////imagePainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    ////imagePainter.setBackgroundMode(Qt::TransparentMode);
    ////imagePainter.setBackground(Qt::transparent);
    //imagePainter.setOpacity(0.25);
 //   //imagePainter.eraseRect(0, 0, 100, 100);
    //imagePainter.drawPixmap(0, 0, pm);
    ////imagePainter.setBrush(QBrush(Qt::red, Qt::SolidPattern));
    ////imagePainter.drawRect(2, 2, 20, 20);
    ////imagePainter.drawRect(3, 3, 75, 75);

	//QPainterPath ellipse;
	//ellipse.moveTo(image.size().width() / 2, image.size().height() / 2);
	//ellipse.arcTo(-200, -200, 400, 400, 180, 270);
	//ellipse.closeSubpath();

	//QPainterPath brect;
	//brect.addRect(0, image.size().height() / 3, image.size().width(), 2 * image.size().height() / 3);

	////QPainterPath clip = ellipse.intersected(brect);

	//imagePainter.setClipPath(brect);
	//imagePainter.setOpacity(1);
	//imagePainter.drawPixmap(0, 0, pm);
	//
 //   imagePainter.end();

    //trayIconImage.addPixmap(image, QIcon::Normal, QIcon::Off);

	//trayIconImage.addPixmap(QPixmap(QString::fromUtf8(":/icons/alarm")), QIcon::Normal, QIcon::Off);

	//updateTrayIcon(0);
	//setWindowIcon(trayIconImage);

	//trayIcon->setIcon(trayIconImage);
	trayIcon->show();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
             this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

    ui.restartButton->setDefaultAction(restartAction);

	ui.minimizeButton->setDefaultAction(minimizeAction);
	ui.exitButton->setDefaultAction(quitAction);

	loadSettings();
}

TeaAlarmProgressDialog::~TeaAlarmProgressDialog()
{
	delete trayIcon;
}

void TeaAlarmProgressDialog::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		if (event->modifiers() & Qt::ControlModifier) {
			resizeEvent = true;
			dragPosition = frameGeometry().bottomRight() - event->globalPos();
			event->accept();
		} else {
			resizeEvent = false;
			dragPosition = event->globalPos() - frameGeometry().topLeft();
			event->accept();
		}
	}
}

void TeaAlarmProgressDialog::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::LeftButton) {

		if (resizeEvent) {
			resize(event->x() + dragPosition.x(), event->y() + dragPosition.y());
		} else {
			move(event->globalPos() - dragPosition);
		}

        sizeIncrement();
        event->accept();

    }
}

void TeaAlarmProgressDialog::setVisible(bool visible)
{
	 minimizeAction->setEnabled(visible);
	 restoreAction->setEnabled(!visible);
	 QWidget::setVisible(visible);
}


void TeaAlarmProgressDialog::createActions()
{
	minimizeAction = new QAction(tr("Mi&nimize"), this);
	minimizeAction->setIcon(ui.minimizeButton->icon());
	connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

	restoreAction = new QAction(tr("&Restore"), this);
	connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

	quitAction = new QAction(tr("&Quit"), this);
	quitAction->setIcon(ui.exitButton->icon());
	connect(quitAction, SIGNAL(triggered()), this, SLOT(quitTimer()));

	restartAction = new QAction(tr("Restart"), this);
	restartAction->setIcon(ui.restartButton->icon());
	connect(restartAction, SIGNAL(triggered()), this, SLOT(restart()));

	createNewAlarmAction = new QAction(tr("New Alarm"), this);
	//createNewAlarmAction->setIcon(ui.restartButton->icon());
	connect(createNewAlarmAction, SIGNAL(triggered()), teaAlarm, SIGNAL(createNewAlarm()));
}


void TeaAlarmProgressDialog::createTrayIcon()
{
	trayIconMenu = new QMenu(this);
	trayIconMenu->addAction(minimizeAction);
	trayIconMenu->addAction(restoreAction);
	trayIconMenu->addSeparator();
	trayIconMenu->addAction(createNewAlarmAction);
	trayIconMenu->addAction(quitAction);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setContextMenu(trayIconMenu);

	trayIcon->setIcon(QIcon(QString::fromUtf8(":/icons/alarm")));
}

void TeaAlarmProgressDialog::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason) {
		//case QSystemTrayIcon::Trigger:
		case QSystemTrayIcon::DoubleClick:
			if (isVisible()) {
				hide();
			} else {
				showNormal();
			}
			break;
		default:
			;
	}
}

void TeaAlarmProgressDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

	QColor cstart = QColor(3, 66, 124, 235);
	QColor cinset = QColor(69, 134, 186, 190);
	QColor cmiddle = QColor(164, 212, 238, 150);

        QColor cstartv = QColor(3, 66, 124, 200);
        QColor cinsetv = QColor(69, 134, 186, 150);
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

	QLinearGradient vlgradient(0, 0, width(), 0);
	vlgradient.setColorAt(0.0, cstartv);
	vlgradient.setColorAt(p1, cinsetv);
        vlgradient.setColorAt(0.35, cmiddlev);
        vlgradient.setColorAt(0.55, cinsetv);
	vlgradient.setColorAt(1.0, cstartv);


    int roundRadius = 0;
    painter.setPen(QColor(0, 0, 0, 32));
    painter.setBrush(hlgradient);
    painter.drawRoundedRect(0, 0, width(), height(),
        roundRadius, roundRadius, Qt::AbsoluteSize);

	painter.setPen(QColor(220, 220, 220, 100));
	painter.setBrush(vlgradient);
	painter.drawRoundedRect(0, 0, width(), height(),
		roundRadius, roundRadius, Qt::AbsoluteSize);

	painter.end();
}

void TeaAlarmProgressDialog::on_playPauseButton_clicked() {
	bool isRunning = teaAlarm->getState() == TeaAlarm::Running;
	if (isRunning) {
		pauseTimer();
	} else {
		startTimer();
	}
}

void TeaAlarmProgressDialog::on_stopButton_clicked() {
	stopTimer();
}

void TeaAlarmProgressDialog::restart() {
	stopTimer();
	startTimer();
}

void TeaAlarmProgressDialog::on_stayOnTopButton_clicked() {
	Qt::WindowFlags flags = windowFlags();
	flags ^= Qt::WindowStaysOnTopHint;
	setWindowFlags(flags);
	show();
}

void TeaAlarmProgressDialog::startTimer()
{
	teaAlarm->start();

    QIcon icon(QPixmap(QString::fromUtf8(":/icons/pastel-pause")));
    ui.playPauseButton->setIcon(icon);
}

void TeaAlarmProgressDialog::pauseTimer() {
	teaAlarm->pause();

    QIcon icon(QPixmap(QString::fromUtf8(":/icons/pastel-play")));
    ui.playPauseButton->setIcon(icon);
}

void TeaAlarmProgressDialog::stopTimer() {
	teaAlarm->stop();

	QIcon icon = QIcon(QPixmap(QString::fromUtf8(":/icons/pastel-play")));
	ui.playPauseButton->setIcon(icon);

	percentPassed = 1;
}

void TeaAlarmProgressDialog::onTimeLeftChanged(int timeLeft) {
	int hours = timeLeft / (60 * 60);
	int minutes = (timeLeft % (60 * 60) ) / 60;
	int seconds = (timeLeft % (60 * 60) ) % 60;

	QString s;
	s = QString("%1:%2:%3")
		.arg((int) hours, 2, 10, (QChar) '0')
		.arg((int) minutes, 2, 10, (QChar) '0')
		.arg((int) seconds, 2, 10, (QChar) '0');
	ui.timeLabel->setText(s);

	if (trayIcon != 0) {
		qreal _percentPassed = ((qreal) timeLeft) / teaAlarm->getDuration();
		updateTrayIcon(_percentPassed);
	}

	update();
}

void TeaAlarmProgressDialog::updateTrayIcon(qreal _percentPassed) {
	/*QIcon newTrayIconImage(QString::fromUtf8(":/icons/alarm"));
	trayIcon->setIcon(newTrayIconImage);
	setWindowIcon(newTrayIconImage);*/

	qreal delta = percentPassed - _percentPassed;

	if (percentPassed == 1 || delta >= 0.05) {
		percentPassed = _percentPassed;

		QPixmap pm = QPixmap(QString::fromUtf8(":/icons/alarm"));

		QPixmap image(pm.size());
		image.fill(Qt::transparent);

		QPainter imagePainter(&image);
		imagePainter.setOpacity(0.25);
		imagePainter.drawPixmap(0, 0, pm);

		QPainterPath clip;
		qreal y = ((qreal) image.size().height()) * percentPassed;
		qreal height = ((qreal) image.size().height()) * (((qreal) 1) - percentPassed);
		clip.addRect(0, y, image.size().width(), height);

		imagePainter.setClipPath(clip);
		imagePainter.setOpacity(1);
		imagePainter.drawPixmap(0, 0, pm);

		imagePainter.setClipping(false);

		QPen pen(Qt::white);
		pen.setWidth(5);
		imagePainter.setPen(pen);
		imagePainter.drawLine(0, y, image.size().width(), y);

		imagePainter.end();

		QIcon trayIconImage(image);

		trayIcon->setIcon(trayIconImage);
		setWindowIcon(trayIconImage);
	}
}

void TeaAlarmProgressDialog::onTeaAlarmFinished() {
	saveSettings();
}

void TeaAlarmProgressDialog::saveSettings() {
	QSettings settings("tea-alarm.ini", QSettings::IniFormat);

	QSize dialogSize = size();
	settings.setValue("dialog-size", dialogSize);

	QPoint position = pos();
	settings.setValue("dialog-pos", position);

	settings.sync();
}

void TeaAlarmProgressDialog::loadSettings() {
	QSettings settings("tea-alarm.ini", QSettings::IniFormat);

	QSize dialogSize = settings.value("dialog-size", QSizeF(200, 95)).toSize();
	resize(dialogSize);

	QPoint dialogPos = settings.value("dialog-pos", QPoint(250, 250)).toPoint();
	move(dialogPos);
}

void TeaAlarmProgressDialog::quitTimer() {
    saveSettings();
    teaAlarm->close();
}
