// VersionBoundaryMarker.cpp


#include "VersionBoundaryMarker.h"
#include "TimelineCoordinateMapper.h"
#include <QPainter>
#include <QDate>


VersionBoundaryMarker::VersionBoundaryMarker(TimelineCoordinateMapper* mapper,
                                             MarkerType type,
                                             QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , mapper_(mapper)
    , type_(type)
    , timelineHeight_(500.0)
{
    // Set Z-value to draw on top of items but below current date marker
    setZValue(50);

    // Set the marker date based on type
    updatePosition();
}


QRectF VersionBoundaryMarker::boundingRect() const
{
    double xPos = mapper_->dateToX(markerDate_);

    // Make bounding rect wide enough for the line and label
    return QRectF(xPos - 40, -25, 80, timelineHeight_);
}


void VersionBoundaryMarker::paint(QPainter* painter,
                                  const QStyleOptionGraphicsItem* /*option*/,
                                  QWidget* /*widget*/)
{
    double xPos = mapper_->dateToX(markerDate_);

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Choose color based on marker type
    QColor lineColor = (type_ == VersionStart)
                           ? QColor(70, 130, 180)      // Steel Blue for start
                           : QColor(255, 140, 0);      // Dark Orange for end

    // Draw the vertical line (thinner than current date marker)
    QPen linePen(lineColor, 2);
    linePen.setStyle(Qt::DashLine);
    painter->setPen(linePen);
    painter->drawLine(QPointF(xPos, 0), QPointF(xPos, timelineHeight_));

    // Draw label at the top
    QFont labelFont = painter->font();
    labelFont.setPointSize(8);
    labelFont.setBold(true);
    painter->setFont(labelFont);

    // Choose label text
    QString label = (type_ == VersionStart) ? "START" : "END";
    QFontMetrics fm(labelFont);
    int labelWidth = fm.horizontalAdvance(label);
    QRectF labelRect(xPos - labelWidth / 2 - 4, -21, labelWidth + 8, 16);

    // Draw label background
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(lineColor.red(), lineColor.green(), lineColor.blue(), 180));
    painter->drawRoundedRect(labelRect, 2, 2);

    // Draw label text
    painter->setPen(Qt::white);
    painter->drawText(labelRect, Qt::AlignCenter, label);
}


void VersionBoundaryMarker::updatePosition()
{
    // Update marker date based on type
    markerDate_ = (type_ == VersionStart)
                      ? mapper_->versionStart()
                      : mapper_->versionEnd();

    prepareGeometryChange();
    update();
}


void VersionBoundaryMarker::setTimelineHeight(double height)
{
    timelineHeight_ = height;
    prepareGeometryChange();
    update();
}
