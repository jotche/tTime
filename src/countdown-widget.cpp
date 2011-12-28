/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the example classes of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>
#include "countdown-widget.h"

CountdownWidget::CountdownWidget(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint)
{

    setAttribute(Qt::WA_TranslucentBackground);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);

    QAction *quitAction = new QAction(tr("E&xit"), this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    addAction(quitAction);

    setContextMenuPolicy(Qt::ActionsContextMenu);
    setToolTip(tr("Drag the clock with the left mouse button.\n"
                  "Use the right mouse button to open a context menu."));
    setWindowTitle(tr("Shaped Analog Clock"));

    QPushButton *quit = new QPushButton(tr("Quit"));
    quit->setFont(QFont("Times", 14, QFont::Bold));

    connect(quit, SIGNAL(clicked()), qApp, SLOT(quit()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(quit);
    setLayout(layout);
}

void CountdownWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void CountdownWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - dragPosition);
        event->accept();
    }
}

void CountdownWidget::paintEvent(QPaintEvent *)
{
//    static const QPoint hourHand[3] = {
//        QPoint(7, 8),
//        QPoint(-7, 8),
//        QPoint(0, -40)
//    };
//    static const QPoint minuteHand[3] = {
//        QPoint(7, 8),
//        QPoint(-7, 8),
//        QPoint(0, -70)
//    };

//    QColor hourColor(127, 0, 127);
//    QColor minuteColor(0, 127, 127, 191);

//    int side = qMin(width(), height()) - 20;
    QTime time = QTime::currentTime();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.translate(width() / 2, height() / 2);

    QRadialGradient gradient(0.0, 0.0, height() / 2,
                             0.0, 0.0);
    gradient.setColorAt(0.0, QColor(34, 85, 255, 190));
//    gradient.setColorAt(0.1, QColor(255, 255, 255, 31));
//    gradient.setColorAt(0.7, QColor(255, 255, 255, 31));
//    gradient.setColorAt(0.8, QColor(0, 31, 0, 31));
    gradient.setColorAt(0.9, QColor(67, 85, 255, 150));
    gradient.setColorAt(1.0, QColor(23, 85, 255, 100));
    painter.setPen(QColor(0, 0, 0, 32));
    painter.setBrush(gradient);
    //painter.drawEllipse(-side/2.0 + 1, -side/2.0 + 1,
    //                    side - 2, side - 2);
    painter.drawRoundRect(- width() / 2, - height() / 2,
                        width(), height(), 20, 20);

    painter.translate(- width() / 2, - height() / 2);

    painter.setPen(QColor(0, 0, 0));
    painter.setFont(QFont("Helvetica", 15, QFont::Normal, false));
    QString s;
    s = time.toString("hh:mm:ss");
//    s.append(QString::number(time.hour()))
//             .append(":")
//             .append(QString::number(time.minute()))
//             .append(":")
//             .append(QString::number(time.second()));
    painter.drawText(25, 25, s);

    /*painter.scale(side / 200.0, side / 200.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(hourColor);

    painter.save();
    painter.rotate(30.0 * ((time.hour() + time.minute() / 60.0)));
    painter.drawConvexPolygon(hourHand, 3);
    painter.restore();

    painter.setPen(hourColor);

    for (int i = 0; i < 12; ++i) {
        painter.drawLine(88, 0, 96, 0);
        painter.rotate(30.0);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(minuteColor);

    painter.save();
    painter.rotate(6.0 * (time.minute() + time.second() / 60.0));
    painter.drawConvexPolygon(minuteHand, 3);
    painter.restore();

    painter.setPen(minuteColor);

    for (int j = 0; j < 60; ++j) {
        if ((j % 5) != 0)
            painter.drawLine(92, 0, 96, 0);
        painter.rotate(6.0);
    }*/
}

//void ShapedClock::resizeEvent(QResizeEvent * /* event */)
/*{
    int side = qMin(width(), height());
    QRegion maskedRegion(width() / 2 - side / 2, height() / 2 - side / 2, side,
                         side, QRegion::Ellipse);

//    QPixmap pixmap(width(), height());
//    pixmap.fill(Qt::transparent);
//
//    QRadialGradient gradient(width() / 2 - side / 2,  height() / 2 - side / 2, side,
//                             width() / 2 - side / 2, height() / 2 - side / 2);
//    gradient.setColorAt(1, QColor::fromRgbF(1, 0, 0, 1));
//    gradient.setColorAt(0, QColor::fromRgbF(0, 0, 0, 0));
//    QPainter painter(&pixmap);
//    painter.fillRect(0, 0, width(), height(), gradient);
//
//    QRegion maskedRegion1(pixmap);


    setMask(maskedRegion);
}*/

QSize CountdownWidget::sizeHint() const
{
    return QSize(350, 200);
}
//! [5]
