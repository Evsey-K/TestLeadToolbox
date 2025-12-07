// TimelineLegend.cpp


#include "TimelineLegend.h"
#include <QPainter>
#include <QFontMetrics>


TimelineLegend::TimelineLegend(QWidget* parent)
    : QWidget(parent)
{
    // Set widget to be transparent to mouse events (click-through)
    setAttribute(Qt::WA_TransparentForMouseEvents);

    // Make widget background semi-transparent
    setAutoFillBackground(false);

    // Start hidden
    hide();
}


void TimelineLegend::setEntries(const QVector<LegendEntry>& entries)
{
    entries_ = entries;
    calculateSize();
    update();
}


void TimelineLegend::calculateSize()
{
    if (entries_.isEmpty())
    {
        resize(0, 0);
        return;
    }

    // Calculate width based on longest label
    QFont font("Arial", 10);
    QFontMetrics metrics(font);

    int maxLabelWidth = 0;
    for (const LegendEntry& entry : entries_)
    {
        int labelWidth = metrics.horizontalAdvance(entry.label);
        if (labelWidth > maxLabelWidth)
        {
            maxLabelWidth = labelWidth;
        }
    }

    int totalWidth = PADDING * 2 + SWATCH_SIZE + LABEL_SPACING + maxLabelWidth;

    // Calculate height
    int totalHeight = PADDING * 2 + (entries_.size() * (SWATCH_SIZE + ENTRY_SPACING)) - ENTRY_SPACING;

    // Update widget size
    resize(totalWidth, totalHeight);
}


QSize TimelineLegend::sizeHint() const
{
    return size();
}


void TimelineLegend::paintEvent(QPaintEvent* /*event*/)
{
    if (entries_.isEmpty())
    {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background with border
    painter.setPen(QPen(QColor(100, 100, 100), BORDER_WIDTH));
    painter.setBrush(QBrush(QColor(255, 255, 255, 240)));  // Semi-transparent white
    painter.drawRect(rect().adjusted(1, 1, -1, -1));  // Slight inset for border

    // Draw each legend entry
    painter.setFont(QFont("Arial", 10));

    int currentY = PADDING;

    for (const LegendEntry& entry : entries_)
    {
        // Draw color swatch
        QRect swatchRect(PADDING, currentY, SWATCH_SIZE, SWATCH_SIZE);
        painter.setBrush(QBrush(entry.color));
        painter.setPen(QPen(Qt::black, 1));
        painter.drawRect(swatchRect);

        // Draw label
        int labelX = PADDING + SWATCH_SIZE + LABEL_SPACING;
        int labelY = currentY + SWATCH_SIZE / 2;  // Center vertically with swatch

        painter.setPen(QPen(Qt::black));
        painter.drawText(QPoint(labelX, labelY + 5), entry.label);  // +5 for vertical centering

        // Move to next entry position
        currentY += SWATCH_SIZE + ENTRY_SPACING;
    }
}
