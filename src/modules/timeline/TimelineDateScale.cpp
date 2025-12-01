// TimelineDateScale.cpp


#include "TimelineDateScale.h"
#include "TimelineCoordinateMapper.h"
#include <QPainter>
#include <QFont>
#include <QDate>


TimelineDateScale::TimelineDateScale(TimelineCoordinateMapper* mapper, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , mapper_(mapper)
    , timelineHeight_(500.0)
    , paddedStart_(mapper->versionStart())
    , paddedEnd_(mapper->versionEnd())
{
    // Set Z-value to draw below event items but above background
    setZValue(-1);
}


QRectF TimelineDateScale::boundingRect() const
{
    // Calculate width based on padded range, not version range
    double startX = mapper_->dateToX(paddedStart_);
    double endX = mapper_->dateToX(paddedEnd_);
    double width = endX - startX;

    return QRectF(startX, 0, width, SCALE_HEIGHT + timelineHeight_);
}


void TimelineDateScale::paint(QPainter* painter,
                              const QStyleOptionGraphicsItem* /*option*/,
                              QWidget* /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing, false);      // Crisp lines

    // Calculate the padded range coordinates
    double startX = mapper_->dateToX(paddedStart_);
    double endX = mapper_->dateToX(paddedEnd_);
    double width = endX - startX;

    // Draw background for date scale area across full padded range
    QRectF scaleRect(startX, 0, width, SCALE_HEIGHT);
    painter->fillRect(scaleRect, QColor(240, 240, 240));

    // Draw border line below date scale across full padded range
    painter->setPen(QPen(Qt::darkGray, 2));
    painter->drawLine(startX, SCALE_HEIGHT, endX, SCALE_HEIGHT);

    // Draw grid lines first (behind ticks and labels)
    drawGridLines(painter);

    // Draw ticks and labels based on zoom level
    drawMonthTicks(painter);

    if (shouldShowWeekTicks())
    {
        drawWeekTicks(painter);
    }

    if (shouldShowDayTicks())
    {
        drawDayTicks(painter);
    }
}


void TimelineDateScale::drawMonthTicks(QPainter* painter)
{
    // Use padded date range instead of version dates
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    // Start from the first day of the month containing version start
    QDate monthStart(currentDate.year(), currentDate.month(), 1);

    painter->setPen(QPen(Qt::black, 2));
    QFont labelFont = painter->font();
    labelFont.setPointSize(10);
    labelFont.setBold(true);
    painter->setFont(labelFont);

    while(monthStart <= endDate)
    {
        double xPos = mapper_->dateToX(monthStart);

        // Draw major tick line
        painter->drawLine(QPointF(xPos, SCALE_HEIGHT - MAJOR_TICK_HEIGHT), QPointF(xPos, SCALE_HEIGHT));

        // Draw month label
        QString monthLabel = monthStart.toString("MMM yyyy");
        QRectF textRect(xPos + 5, 5, 100, 20);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, monthLabel);

        // Move to next month
        monthStart = monthStart.addMonths(1);
    }
}


void TimelineDateScale::drawWeekTicks(QPainter* painter)
{
    // Use padded date range instead of version dates
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    // Find the first Monday on or after version start
    while (currentDate.dayOfWeek() != Qt::Monday && currentDate <= endDate)
    {
        currentDate = currentDate.addDays(1);
    }

    painter->setPen(QPen(Qt::darkGray, 1));

    while (currentDate <= endDate)
    {
        double xPos = mapper_->dateToX(currentDate);

        // Draw minor tick line (skip if it coincides with month start)
        if (currentDate.day() != 1)
        {
            painter->drawLine(QPointF(xPos, SCALE_HEIGHT - MINOR_TICK_HEIGHT),
                              QPointF(xPos, SCALE_HEIGHT));
        }

        currentDate = currentDate.addDays(7);
    }
}


void TimelineDateScale::drawDayTicks(QPainter* painter)
{
    // Use padded date range instead of version dates
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    painter->setPen(QPen(Qt::lightGray, 1));
    QFont dayFont = painter->font();
    dayFont.setPointSize(8);
    painter->setFont(dayFont);

    while (currentDate <= endDate)
    {
        double xPos = mapper_->dateToX(currentDate);

        // Skip month starts and Mondays (already drawn)
        if (currentDate.day() != 1 && currentDate.dayOfWeek() != Qt::Monday)
        {
            // Draw tiny tick
            painter->drawLine(QPointF(xPos, SCALE_HEIGHT - DAY_TICK_HEIGHT),
                              QPointF(xPos, SCALE_HEIGHT));

            // Draw day number if enough space
            if (mapper_->pixelsPerday() >= 15.0)
            {
                QString dayLabel = QString::number(currentDate.day());
                QRectF textRect(xPos - 10, SCALE_HEIGHT - 25, 20, 15);
                painter->drawText(textRect, Qt::AlignCenter, dayLabel);
            }
        }

        currentDate = currentDate.addDays(1);
    }
}


void TimelineDateScale::drawGridLines(QPainter* painter)
{
    // Use padded date range instead of version dates
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    // Start from first of month
    QDate monthStart(currentDate.year(), currentDate.month(), 1);

    painter->setPen(QPen(QColor(230, 230, 230), 1, Qt::DashLine));

    while (monthStart <= endDate)
    {
        double xPos = mapper_->dateToX(monthStart);

        // Draw vertical grid line extending through the timeline
        painter->drawLine(QPointF(xPos, SCALE_HEIGHT),
                          QPointF(xPos, SCALE_HEIGHT + timelineHeight_));

        monthStart = monthStart.addMonths(1);
    }
}


bool TimelineDateScale::shouldShowDayTicks() const
{
    // Show day ticks when pixels per day is large enough
    return mapper_->pixelsPerday() >= 10.0;
}


bool TimelineDateScale::shouldShowWeekTicks() const
{
    // Show week ticks when pixels per day is reasonable
    return mapper_->pixelsPerday() >= 3.0;
}


void TimelineDateScale::updateScale()
{
    // Trigger repaint when scale needs updating
    update();
}


void TimelineDateScale::setTimelineHeight(double height)
{
    timelineHeight_ = height;
    prepareGeometryChange();    // Notify scene of bounds change
    update();
}


void TimelineDateScale::setPaddedDateRange(const QDate& paddedStart, const QDate& paddedEnd)
{
    paddedStart_ = paddedStart;
    paddedEnd_ = paddedEnd;
    update();
}





