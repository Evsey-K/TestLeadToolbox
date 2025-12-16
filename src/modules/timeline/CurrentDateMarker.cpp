// CurrentDateMarker.cpp


#include "CurrentDateMarker.h"
#include "TimelineCoordinateMapper.h"
#include <QPainter>
#include <QDate>


CurrentDateMarker::CurrentDateMarker(TimelineCoordinateMapper* mapper, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , mapper_(mapper)
    , timelineHeight_(500.0)
    , currentDate_(QDate::currentDate())
{
    // Set Z-value to draw on top of most items
    setZValue(100);
}


QRectF CurrentDateMarker::boundingRect() const
{
    double xPos = mapper_->dateToX(currentDate_);

    // Make bounding rect wide enough for the line and label
    return QRectF(xPos - 50, -60, 100, timelineHeight_);
}


void CurrentDateMarker::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    // Only draw if current date is within version range
    if (currentDate_ < mapper_->versionStart() || currentDate_ > mapper_->versionEnd())
    {
        return;
    }

    double xPos = mapper_->dateToX(currentDate_);

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Draw the vertival line
    QPen linePen(QColor(220, 20, 60), 3);       // Crimson Red, thick line (we like em thick)
    linePen.setStyle(Qt::SolidLine);
    painter->setPen(linePen);
    painter->drawLine(QPointF(xPos, 0), QPointF(xPos, timelineHeight_));

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
    currentDate_ = QDate::currentDate();
    prepareGeometryChange();
    update();
}


void CurrentDateMarker::setTimelineHeight(double height)
{
    timelineHeight_ = height;
    prepareGeometryChange();
    update();
}
