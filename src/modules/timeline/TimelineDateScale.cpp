// TimelineDateScale.cpp


#include "TimelineDateScale.h"
#include "TimelineCoordinateMapper.h"
#include <QPainter>
#include <QDateTime>
#include <cmath>


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
    double width = mapper_->totalWidth();
    return QRectF(0, 0, width, SCALE_HEIGHT + timelineHeight_);
}


void TimelineDateScale::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Draw background for date scale area
    painter->fillRect(QRectF(0, 0, boundingRect().width(), SCALE_HEIGHT),
                      QColor(245, 245, 248));

    // Draw separator line
    painter->setPen(QPen(QColor(200, 200, 200), 2));
    painter->drawLine(QPointF(0, SCALE_HEIGHT), QPointF(boundingRect().width(), SCALE_HEIGHT));

    // Draw grid lines first (behind ticks and labels)
    drawGridLines(painter);

    // Draw ticks and labels based on zoom level
    // Order matters for visual hierarchy
    drawMonthTicks(painter);

    if (shouldShowWeekTicks())
    {
        drawWeekTicks(painter);
    }

    if (shouldShowDayTicks())
    {
        drawDayTicks(painter);
    }

    if (shouldShowHourTicks())
    {
        drawHourTicks(painter);
    }

    if (shouldShowHalfHourTicks())
    {
        drawHalfHourTicks(painter);
    }
}


void TimelineDateScale::drawMonthTicks(QPainter* painter)
{
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    QDate monthStart(currentDate.year(), currentDate.month(), 1);

    // Use cosmetic pen (width 0) for crisp lines at any zoom
    painter->setPen(QPen(QColor(60, 60, 60), 0));  // Width 0 = cosmetic
    QFont labelFont = painter->font();
    labelFont.setPointSize(10);
    labelFont.setBold(true);
    painter->setFont(labelFont);

    while(monthStart <= endDate)
    {
        double xPos = mapper_->dateToX(monthStart);

        // Draw major tick line
        painter->drawLine(QPointF(xPos, SCALE_HEIGHT - MAJOR_TICK_HEIGHT),
                          QPointF(xPos, SCALE_HEIGHT));

        // Draw month label
        QString monthLabel = monthStart.toString("MMM yyyy");
        QRectF textRect(xPos + 5, 5, 100, 20);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, monthLabel);

        monthStart = monthStart.addMonths(1);
    }
}

void TimelineDateScale::drawWeekTicks(QPainter* painter)
{
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    while (currentDate.dayOfWeek() != Qt::Monday && currentDate <= endDate)
    {
        currentDate = currentDate.addDays(1);
    }

    painter->setPen(QPen(QColor(100, 100, 100), 0));  // Cosmetic pen

    while (currentDate <= endDate)
    {
        double xPos = mapper_->dateToX(currentDate);

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
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    painter->setPen(QPen(QColor(150, 150, 150), 0));  // Cosmetic pen
    QFont dayFont = painter->font();
    dayFont.setPointSize(8);
    painter->setFont(dayFont);

    while (currentDate <= endDate)
    {
        double xPos = mapper_->dateToX(currentDate);

        if (currentDate.day() != 1 && currentDate.dayOfWeek() != Qt::Monday)
        {
            painter->drawLine(QPointF(xPos, SCALE_HEIGHT - DAY_TICK_HEIGHT),
                              QPointF(xPos, SCALE_HEIGHT));
        }

        if (mapper_->pixelsPerday() >= 20.0)
        {
            QString dayLabel = QString::number(currentDate.day());
            QRectF textRect(xPos - 10, SCALE_HEIGHT - 28, 20, 15);
            painter->drawText(textRect, Qt::AlignCenter, dayLabel);
        }

        currentDate = currentDate.addDays(1);
    }
}


void TimelineDateScale::drawHourTicks(QPainter* painter)
{
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    painter->setPen(QPen(QColor(180, 180, 180), 0));  // Cosmetic pen
    QFont hourFont = painter->font();
    hourFont.setPointSize(7);
    painter->setFont(hourFont);

    // Get the hour labeling interval based on zoom level
    int labelInterval = getHourLabelInterval();

    while (currentDate <= endDate)
    {
        // Draw tick for each hour (0-23)
        for (int hour = 0; hour < 24; ++hour)
        {
            QDateTime dateTime(currentDate, QTime(hour, 0));
            double xPos = mapper_->dateTimeToX(dateTime);

            // Draw hour tick
            painter->drawLine(QPointF(xPos, SCALE_HEIGHT - HOUR_TICK_HEIGHT),
                              QPointF(xPos, SCALE_HEIGHT));

            // Draw hour label based on interval and zoom level
            if (shouldShowHourLabels() && (hour % labelInterval == 0))
            {
                QString hourLabel = QString("%1:00").arg(hour, 2, 10, QChar('0'));
                QRectF textRect(xPos - 15, SCALE_HEIGHT - 42, 30, 12);
                painter->drawText(textRect, Qt::AlignCenter, hourLabel);
            }
        }

        currentDate = currentDate.addDays(1);
    }
}


void TimelineDateScale::drawHalfHourTicks(QPainter* painter)
{
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    painter->setPen(QPen(QColor(200, 200, 200), 0));  // Cosmetic pen

    while (currentDate <= endDate)
    {
        for (int hour = 0; hour < 24; ++hour)
        {
            QDateTime dateTime(currentDate, QTime(hour, 30));
            double xPos = mapper_->dateTimeToX(dateTime);

            painter->drawLine(QPointF(xPos, SCALE_HEIGHT - HALF_HOUR_TICK_HEIGHT),
                              QPointF(xPos, SCALE_HEIGHT));
        }

        currentDate = currentDate.addDays(1);
    }
}


void TimelineDateScale::drawGridLines(QPainter* painter)
{
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    // Determine grid line frequency based on zoom level
    painter->setPen(QPen(QColor(230, 230, 230), 0, Qt::DashLine));  // Cosmetic pen

    // At high zoom, draw grid lines for days
    if (shouldShowDayTicks() && !shouldShowHourTicks())
    {
        // Daily grid lines
        while (currentDate <= endDate)
        {
            double xPos = mapper_->dateToX(currentDate);
            painter->drawLine(QPointF(xPos, SCALE_HEIGHT),
                              QPointF(xPos, SCALE_HEIGHT + timelineHeight_));
            currentDate = currentDate.addDays(1);
        }
    }
    // At extreme zoom, draw grid lines for hours
    else if (shouldShowHourTicks())
    {
        currentDate = paddedStart_;
        while (currentDate <= endDate)
        {
            for (int hour = 0; hour < 24; hour += 6)  // Every 6 hours
            {
                QDateTime dateTime(currentDate, QTime(hour, 0));
                double xPos = mapper_->dateTimeToX(dateTime);
                painter->drawLine(QPointF(xPos, SCALE_HEIGHT),
                                  QPointF(xPos, SCALE_HEIGHT + timelineHeight_));
            }
            currentDate = currentDate.addDays(1);
        }
    }
    // At low zoom, draw grid lines for months
    else
    {
        QDate monthStart(currentDate.year(), currentDate.month(), 1);
        while (monthStart <= endDate)
        {
            double xPos = mapper_->dateToX(monthStart);
            painter->drawLine(QPointF(xPos, SCALE_HEIGHT),
                              QPointF(xPos, SCALE_HEIGHT + timelineHeight_));
            monthStart = monthStart.addMonths(1);
        }
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
    return mapper_->pixelsPerday() >= 3.0 && mapper_->pixelsPerday() < 80.0;
}


bool TimelineDateScale::shouldShowHourTicks() const
{
    // Show hour ticks when deeply zoomed (about 2+ pixels per hour)
    return mapper_->pixelsPerday() >= 50.0;
}


bool TimelineDateScale::shouldShowHalfHourTicks() const
{
    // Show half-hour ticks when very deeply zoomed (about 4+ pixels per hour)
    return mapper_->pixelsPerday() >= 100.0;
}


bool TimelineDateScale::shouldShowHourLabels() const
{
    // Show hour labels when there's enough space
    return mapper_->pixelsPerday() >= 80.0;
}


int TimelineDateScale::getHourLabelInterval() const
{
    double ppd = mapper_->pixelsPerday();

    // Calculate pixels per hour
    double pixelsPerHour = ppd / 24.0;

    // Minimum label width is approximately 35 pixels (for "00:00" text)
    const double MIN_LABEL_SPACING = 40.0;

    // Determine interval based on available space
    if (pixelsPerHour * 1 >= MIN_LABEL_SPACING)
    {
        return 1;  // Show every hour (ppd >= ~960)
    }
    else if (pixelsPerHour * 2 >= MIN_LABEL_SPACING)
    {
        return 2;  // Show every 2 hours (ppd >= ~480)
    }
    else if (pixelsPerHour * 3 >= MIN_LABEL_SPACING)
    {
        return 3;  // Show every 3 hours (ppd >= ~320)
    }
    else if (pixelsPerHour * 4 >= MIN_LABEL_SPACING)
    {
        return 4;  // Show every 4 hours (ppd >= ~240)
    }
    else if (pixelsPerHour * 6 >= MIN_LABEL_SPACING)
    {
        return 6;  // Show every 6 hours (ppd >= ~160)
    }
    else
    {
        return 12;  // Show every 12 hours (ppd < ~160)
    }
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





