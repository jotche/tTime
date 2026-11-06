#include "countdown-label.h"

#include "digit-layout.h"

#include <QEvent>
#include <QFontMetricsF>
#include <QPainter>
#include <QStyle>

#include <cmath>

namespace {

struct Measured
{
    QFont font; // font without letter spacing (spacing is applied by the layout)
    DigitLayout layout;
    qreal ascent = 0;
    qreal height = 0;
};

Measured measure(const QFont &labelFont, const QString &text)
{
    Measured m;
    m.font = labelFont;

    qreal factor = 1.0;
    qreal absolute = 0.0;
    if (labelFont.letterSpacingType() == QFont::PercentageSpacing) {
        factor = labelFont.letterSpacing() / 100.0;
    } else {
        absolute = labelFont.letterSpacing();
    }
    m.font.setLetterSpacing(QFont::PercentageSpacing, 100);

    const QFontMetricsF metrics(m.font);
    m.layout = layoutFixedDigits(text, [&](QChar c) { return metrics.horizontalAdvance(c); }, factor, absolute);
    m.ascent = metrics.ascent();
    m.height = metrics.height();
    return m;
}

} // namespace

CountdownLabel::CountdownLabel(QWidget *parent)
    : QLabel(parent)
{
    setTextFormat(Qt::PlainText);
}

QSize CountdownLabel::sizeHint() const
{
    const Measured m = measure(font(), text());
    const QMargins margins = contentsMargins();
    return {static_cast<int>(std::ceil(m.layout.width)) + margins.left() + margins.right(),
            static_cast<int>(std::ceil(m.height)) + margins.top() + margins.bottom()};
}

QSize CountdownLabel::minimumSizeHint() const
{
    return sizeHint();
}

void CountdownLabel::changeEvent(QEvent *event)
{
    QLabel::changeEvent(event);
    if (event->type() == QEvent::FontChange) {
        updateGeometry();
    }
}

void CountdownLabel::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    drawFrame(&painter);

    const QString content = text();
    if (content.isEmpty()) {
        return;
    }

    const Measured m = measure(font(), content);
    const QRectF area = contentsRect();
    const Qt::Alignment align = QStyle::visualAlignment(layoutDirection(), alignment());

    qreal x = area.left();
    if (align & Qt::AlignRight) {
        x = area.right() + 1 - m.layout.width;
    } else if (align & Qt::AlignHCenter) {
        x = area.left() + (area.width() - m.layout.width) / 2;
    }

    qreal top = area.top() + (area.height() - m.height) / 2; // vertically centred by default
    if (align & Qt::AlignTop) {
        top = area.top();
    } else if (align & Qt::AlignBottom) {
        top = area.bottom() + 1 - m.height;
    }
    const qreal baseline = top + m.ascent;

    painter.setFont(m.font);
    painter.setPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled, foregroundRole()));
    for (qsizetype i = 0; i < content.size(); ++i) {
        painter.drawText(QPointF(x + m.layout.glyphX[i], baseline), QString(content[i]));
    }
}
