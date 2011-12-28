#include <QtGui>
#include <QSystemTrayIcon>
#include "tea-clock.h"
#include "timer-finished-dialog.h"

TeaClock::TeaClock(QWidget *parent, Qt::WFlags flags)
	: QWidget(parent, flags)
{
	ui.setupUi(this);

	createActions();

	resizeEvent = false;
	setAttribute(Qt::WA_TranslucentBackground);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);

    addAction(restartAction);

	addAction(minimizeAction);
	addAction(restoreAction);
	addAction(quitAction);

    setContextMenuPolicy(Qt::ActionsContextMenu);
    setToolTip(tr("Drag the clock with the left mouse button.\n"
                  "Use the right mouse button to open a context menu."));
    setWindowTitle(tr("Tea Clock"));

	duration = 0;
	QRegExp rx("(?:(\\d+)h)?\\s*(?:(\\d+)(?:m|min))?\\s*(?:(\\d+)(?:s|sec))?");
	int pos = rx.indexIn("5s");
	QStringList list = rx.capturedTexts();
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
	requestedDuration = duration;
	running = false;

	QFontDatabase::addApplicationFont(":/fonts/stalker2");
	QFont timeFont = QFont("stalker2", 22, QFont::DemiBold, false);
	ui.timeLabel->setFont(timeFont);

	ui.descriptionLabel->setText("green tea");
	QFont descriptionFont = QFont("stalker2", 12, QFont::Light, false);
	ui.descriptionLabel->setFont(descriptionFont);


    QIcon trayIconImage;
    trayIconImage.addPixmap(QPixmap(QString::fromUtf8(":/icons/alarm")), QIcon::Normal, QIcon::Off);
    setWindowIcon(trayIconImage);

	createTrayIcon();
	trayIcon->setIcon(trayIconImage);
	trayIcon->show();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
             this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

	//connect(ui_playPauseButton, SIGNAL(clicked()), this, SLOT(on_playButton_clicked()));
	ui.restartButton->setDefaultAction(restartAction);

	ui.minimizeButton->setDefaultAction(minimizeAction);
	ui.exitButton->setDefaultAction(quitAction);

    /*QPushButton *quit = new QPushButton(tr("Quit"));
    quit->setFont(QFont("Times", 14, QFont::Bold));

    connect(quit, SIGNAL(clicked()), qApp, SLOT(quit()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(quit);
    setLayout(layout);*/
}

TeaClock::~TeaClock()
{

}

void TeaClock::mousePressEvent(QMouseEvent *event)
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

void TeaClock::mouseMoveEvent(QMouseEvent *event)
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

void TeaClock::setVisible(bool visible)
{
	 minimizeAction->setEnabled(visible);
	 restoreAction->setEnabled(!visible);
	 QWidget::setVisible(visible);
}


void TeaClock::createActions()
{
	minimizeAction = new QAction(tr("Mi&nimize"), this);
	minimizeAction->setIcon(ui.minimizeButton->icon());
	connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

	restoreAction = new QAction(tr("&Restore"), this);
	connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

	quitAction = new QAction(tr("&Quit"), this);
	quitAction->setIcon(ui.exitButton->icon());
	connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

	restartAction = new QAction(tr("Restart"), this);
	restartAction->setIcon(ui.restartButton->icon());
	connect(restartAction, SIGNAL(triggered()), this, SLOT(restart()));
}


void TeaClock::createTrayIcon()
{
	trayIconMenu = new QMenu(this);
	trayIconMenu->addAction(minimizeAction);
	trayIconMenu->addAction(restoreAction);
	trayIconMenu->addSeparator();
	trayIconMenu->addAction(quitAction);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setContextMenu(trayIconMenu);
}

void TeaClock::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
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

void TeaClock::paintEvent(QPaintEvent *)
{
    QTime time = QTime::currentTime();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.translate(width() / 2, height() / 2);

	/*QColor cstart = QColor(3, 78, 150, 235);
	QColor cinset = QColor(89, 146, 194, 190);
	QColor cmiddle = QColor(106, 185, 228, 150);

	QColor cstartv = QColor(3, 78, 150, 150);
	QColor cinsetv = QColor(89, 146, 194, 100);
	QColor cmiddlev = QColor(106, 185, 228, 80);*/

	QColor cstart = QColor(3, 66, 124, 235);
	QColor cinset = QColor(69, 134, 186, 190);
	QColor cmiddle = QColor(164, 212, 238, 150);

	QColor cstartv = QColor(3, 66, 124, 150);
	QColor cinsetv = QColor(69, 134, 186, 100);
	QColor cmiddlev = QColor(164, 212, 238, 80);

	qreal ins = 20;
	qreal p1(ins / height());
	qreal p2(1.0 - p1);

	/*QLinearGradient hlgradient(-width() / 2, -height() / 2,
		-width() / 2, height() / 2);
	hlgradient.setColorAt(0.0, QColor(35, 53, 87, 235));
	hlgradient.setColorAt(p1, QColor(61, 94, 142, 190));
	hlgradient.setColorAt(0.5, QColor(150, 181, 215, 150));
	hlgradient.setColorAt(p2, QColor(61, 94, 142, 190));
	hlgradient.setColorAt(1.0, QColor(35, 53, 87, 235));*/

	QLinearGradient hlgradient(-width() / 2, -height() / 2,
		-width() / 2, height() / 2);
	hlgradient.setColorAt(0.0, cstart);
	hlgradient.setColorAt(p1, cinset);
	hlgradient.setColorAt(0.5, cmiddle);
	hlgradient.setColorAt(p2, cinset);
	hlgradient.setColorAt(1.0, cstart);

	p1 = ins / width();
	p2 = 1 - p1;

	/*QLinearGradient vlgradient(-width() / 2, -height() / 2,
		width() / 2, -height() / 2);
	vlgradient.setColorAt(0.0, QColor(35, 53, 87, 150));
	vlgradient.setColorAt(p1, QColor(61, 94, 142, 100));
	vlgradient.setColorAt(0.5, QColor(150, 181, 215, 80));
	vlgradient.setColorAt(p2, QColor(61, 94, 142, 100));
	vlgradient.setColorAt(1.0, QColor(35, 53, 87, 150));*/

	QLinearGradient vlgradient(-width() / 2, -height() / 2,
		width() / 2, -height() / 2);
	vlgradient.setColorAt(0.0, cstartv);
	vlgradient.setColorAt(p1, cinsetv);
	vlgradient.setColorAt(0.5, cmiddlev);
	vlgradient.setColorAt(p2, cinsetv);
	vlgradient.setColorAt(1.0, cstartv);

    QRadialGradient gradient(0.0, 0.0, height() / 2,
                             0.0, 0.0);
    gradient.setColorAt(0.0, QColor(34, 85, 255, 190));
//    gradient.setColorAt(0.1, QColor(255, 255, 255, 31));
//    gradient.setColorAt(0.7, QColor(255, 255, 255, 31));
//    gradient.setColorAt(0.8, QColor(0, 31, 0, 31));
    gradient.setColorAt(0.9, QColor(67, 85, 255, 150));
    gradient.setColorAt(1.0, QColor(23, 85, 255, 100));
    painter.setPen(QColor(0, 0, 0, 32));
    painter.setBrush(hlgradient);
    painter.drawRoundedRect(- width() / 2, - height() / 2,
        width(), height(), 8, 8, Qt::AbsoluteSize);

	painter.setPen(QColor(220, 220, 220, 100));
	painter.setBrush(vlgradient);
	painter.drawRoundedRect(- width() / 2, - height() / 2,
		width(), height(), 8, 8, Qt::AbsoluteSize);

	painter.translate(- width() / 2, - height() / 2);

	QDateTime currentTime(QDateTime::currentDateTime());
	if (running && currentTime >= endTime) {
		stopTimer();

		hide();
		TimerFinishedDialog *dialog = new TimerFinishedDialog(0, 0);
		dialog->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
		dialog->show();
	}

	if (running) {
		int secTo = currentTime.secsTo(endTime);
		int hours = secTo / (60 * 60);
		int minutes = (secTo % (60 * 60) ) / 60;
		int seconds = (secTo % (60 * 60) ) % 60;

		QString s;
		s = QString("%1:%2:%3")
			.arg((int) hours, 2, 10, (QChar) '0')
			.arg((int) minutes, 2, 10, (QChar) '0')
			.arg((int) seconds, 2, 10, (QChar) '0');
		ui.timeLabel->setText(s);
		/*painter.setPen(QColor(255, 255, 255, 255));
		painter.setFont(QFont("stalker2", 22, QFont::DemiBold, false));
		painter.drawText(25, 80, s);*/
	} else {
		int secTo = (int) duration;
		int hours = secTo / (60 * 60);
		int minutes = (secTo % (60 * 60) ) / 60;
		int seconds = (secTo % (60 * 60) ) % 60;

		QString s;
		s = QString("%1:%2:%3")
			.arg((int) hours, 2, 10, (QChar) '0')
			.arg((int) minutes, 2, 10, (QChar) '0')
			.arg((int) seconds, 2, 10, (QChar) '0');
		ui.timeLabel->setText(s);
		/*painter.setPen(QColor(255, 255, 255, 255));
		painter.setFont(QFont("stalker2", 22, QFont::DemiBold, false));
		painter.drawText(25, 80, s);*/
	}

    /*painter.setPen(QColor(255, 255, 255, 255));
    painter.setFont(QFont("Helvetica", 18, QFont::DemiBold, false));
    QString s;
    s = time.toString("hh:mm:ss");
    painter.drawText(25, 80, s);*/
}

void TeaClock::on_playPauseButton_clicked() {
	if (!running) {
		startTimer();
	} else {
		pauseTimer();
	}
	update();
}

void TeaClock::on_stopButton_clicked() {
	stopTimer();
	update();
}

void TeaClock::restart() {
	stopTimer();
	startTimer();
	update();
}

void TeaClock::on_stayOnTopButton_clicked() {
	Qt::WindowFlags flags = windowFlags();
	flags ^= Qt::WindowStaysOnTopHint;
	setWindowFlags(flags);
	show();
}

//void TeaClock::on_minimizeButton_clicked() {
//	showMinimized();
//}

//void TeaClock::on_exitButton_clicked() {
//	qApp->exit();
//}

void TeaClock::startTimer()
{
	startTime = QDateTime::currentDateTime();
	endTime = startTime;
	endTime = endTime.addSecs(duration);
	running = true;

    QIcon icon(QPixmap(QString::fromUtf8(":/icons/pause")));
    ui.playPauseButton->setIcon(icon);

    update();
}

void TeaClock::pauseTimer() {
	running = false;
	duration = QDateTime::currentDateTime().secsTo(endTime);

    QIcon icon(QPixmap(QString::fromUtf8(":/icons/play")));
    ui.playPauseButton->setIcon(icon);
}

void TeaClock::stopTimer() {
	running = false;
	duration = requestedDuration;

    QIcon icon = QIcon(QPixmap(QString::fromUtf8(":/icons/play")));
    ui.playPauseButton->setIcon(icon);
}
