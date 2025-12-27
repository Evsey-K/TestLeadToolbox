// CurrentDateMarker.cpp

#include "CurrentDateMarker.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineDateScale.h"
#include <QPainter>
#include <QDateTime>

CurrentDateMarker::CurrentDateMarker(TimelineCoordinateMapper* mapper, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , mapper_(mapper)
    , timelineHeight_(500.0)
    , currentDateTime_(QDateTime::currentDateTime())
    , snappedXPosition_(0.0)
{
    // Set Z-value to draw below date scale and events
    setZValue(0);

    // Calculate initial position
    updatePosition();
}

QRectF CurrentDateMarker::boundingRect() const
{
    // Use snappedXPosition_ for bounding rect
    // Make bounding rect wide enough for the line and label
    return QRectF(snappedXPosition_ - 50, -60, 100, timelineHeight_);
}

void CurrentDateMarker::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    // Only draw if current date is within version range
    if (currentDateTime_.date() < mapper_->versionStart() ||
        currentDateTime_.date() > mapper_->versionEnd())
    {
        return;
    }

    // Use the exact current time position (not snapped)
    double xPos = snappedXPosition_;

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Draw the vertival line
    QPen linePen(QColor(220, 20, 60), 3);       // Crimson Red, thick line (we like em thick)
    linePen.setStyle(Qt::SolidLine);
    painter->setPen(linePen);
    painter->drawLine(QPointF(xPos, TimelineDateScale::SCALE_HEIGHT), QPointF(xPos, timelineHeight_));

    // Draw "TODAY" label at the top
    QFont labelFont = painter->font();
    labelFont.setPointSize(10);
    labelFont.setBold(true);
    painter->setFont(labelFont);

    // Draw label background
    QString label = "TODAY";
    QFontMetrics fm(labelFont);
    int labelWidth = fm.horizontalAdvance(label);
    QRectF labelRect(xPos - labelWidth / 2 - 5, -46, labelWidth + 10, 20);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(200, 20, 60, 200));    // Semi-transparent red
    painter->drawRoundedRect(labelRect, 3, 3);

    // Draw label text
    painter->setPen(Qt::white);
    painter->drawText(labelRect, Qt::AlignCenter, label);
}

void CurrentDateMarker::updatePosition()
{
    // Get current date and time
    currentDateTime_ = QDateTime::currentDateTime();

    // Use exact current time position - DO NOT snap
    // The marker should show the actual current time, not round to nearest tick
    snappedXPosition_ = mapper_->dateTimeToX(currentDateTime_);

    prepareGeometryChange();
    update();
}

void CurrentDateMarker::setTimelineHeight(double height)
{
    timelineHeight_ = height;
    prepareGeometryChange();
    update();
}
