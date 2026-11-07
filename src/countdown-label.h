#pragma once

#include <QLabel>

// QLabel for the countdown: digits are drawn in equal-width slots, so the time
// does not shift sideways as it changes, even with fonts whose digits have
// different widths (e.g. Jura). Plain text only; honours alignment, font
// letter spacing, contents margins and the label's text color.
class CountdownLabel : public QLabel
{
    Q_OBJECT

public:
    explicit CountdownLabel(QWidget *parent = 0);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);
    void changeEvent(QEvent *event);
};
