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
    // Set Z-value to draw date scale above marker lines but below events
    setZValue(5);
}

/*
QRectF TimelineDateScale::boundingRect() const
{
    double width = mapper_->totalWidth();
    return QRectF(0, 0, width, SCALE_HEIGHT + timelineHeight_);
}
*/

QRectF TimelineDateScale::boundingRect() const
{
    // Calculate the full padded range to match scene rect
    double startX = mapper_->dateToX(paddedStart_);
    double endX = mapper_->dateToX(paddedEnd_);
    double width = endX - startX;

    return QRectF(startX, 0, width, SCALE_HEIGHT + timelineHeight_);
}


void TimelineDateScale::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Calculate the padded range coordinates
    double startX = mapper_->dateToX(paddedStart_);
    double endX = mapper_->dateToX(paddedEnd_);
    double width = endX - startX;

    // Draw background for date scale area with gradient
    QLinearGradient backgroundGradient(startX, 0, startX, SCALE_HEIGHT);
    backgroundGradient.setColorAt(0.0, QColor(245, 248, 250));     // Light blue-gray at top
    backgroundGradient.setColorAt(1.0, QColor(235, 240, 245));     // Slightly darker at bottom
    painter->fillRect(QRectF(startX, 0, width, SCALE_HEIGHT), backgroundGradient);

    // Draw separator line with accent color
    painter->setPen(QPen(QColor(100, 140, 180), 2));  // Professional blue
    painter->drawLine(QPointF(startX, SCALE_HEIGHT), QPointF(endX, SCALE_HEIGHT));

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

    // Use darker blue for month ticks (major divisions)
    painter->setPen(QPen(QColor(70, 100, 130), 0));  // Dark blue-gray
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

        // Draw month label centered in the middle of the month
        QString monthLabel = monthStart.toString("MMM yyyy");

        // Calculate the middle of the month
        QDate monthEnd = monthStart.addMonths(1);
        double monthStartX = mapper_->dateToX(monthStart);
        double monthEndX = mapper_->dateToX(monthEnd);
        double monthCenterX = (monthStartX + monthEndX) / 2.0;

        // Get text width for centering
        QFontMetrics fm(labelFont);
        int textWidth = fm.horizontalAdvance(monthLabel);

        // Position text centered on the month's midpoint
        QRectF textRect(monthCenterX - textWidth / 2.0, 5, textWidth, 20);

        // Use rich navy blue for month/year text
        painter->setPen(QColor(40, 60, 90));  // Navy blue
        painter->drawText(textRect, Qt::AlignCenter, monthLabel);

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

    // Use medium blue-gray for week ticks
    painter->setPen(QPen(QColor(100, 130, 160), 0));

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

    // Use lighter blue-gray for day ticks
    painter->setPen(QPen(QColor(120, 140, 165), 0));
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

        // Draw day number labels
        if (mapper_->pixelsPerday() >= 20.0)
        {
            QString dayLabel = QString::number(currentDate.day());
            QRectF textRect(xPos - 10, SCALE_HEIGHT - 28, 20, 15);

            // Use dark slate for day numbers
            painter->setPen(QColor(50, 70, 95));
            painter->drawText(textRect, Qt::AlignCenter, dayLabel);

            // Restore tick color
            painter->setPen(QPen(QColor(120, 140, 165), 0));
        }

        // Draw day-of-week abbreviations centered between day ticks
        if (mapper_->pixelsPerday() >= 20.0)  // Show when days have reasonable spacing
        {
            QDate nextDate = currentDate.addDays(1);

            // Only draw if next date is within range
            if (nextDate <= endDate)
            {
                // Calculate center position between current day and next day
                double currentX = mapper_->dateToX(currentDate);
                double nextX = mapper_->dateToX(nextDate);
                double centerX = (currentX + nextX) / 2.0;

                // Get 3-letter day abbreviation (Mon, Tue, Wed, etc.)
                QString dayOfWeek = currentDate.toString("ddd");

                // Position the label higher when hour labels are showing to avoid overlap
                double yPosition;
                if (shouldShowHourLabels())
                {
                    // When hour labels are present (at SCALE_HEIGHT - 42), place day-of-week above them
                    yPosition = SCALE_HEIGHT - 55;
                }
                else
                {
                    // When no hour labels, place between day numbers and scale bottom
                    yPosition = SCALE_HEIGHT - 43;
                }

                // Create text rectangle centered on the midpoint
                QRectF dowRect(centerX - 15, yPosition, 30, 12);

                // Use slightly smaller, lighter font for day-of-week
                QFont dowFont = painter->font();
                dowFont.setPointSize(7);
                painter->setFont(dowFont);

                // Use warm gray for day-of-week abbreviations
                painter->setPen(QPen(QColor(110, 120, 135), 0));

                painter->drawText(dowRect, Qt::AlignCenter, dayOfWeek);

                // Restore original font and pen
                painter->setFont(dayFont);
                painter->setPen(QPen(QColor(120, 140, 165), 0));
            }
        }

        currentDate = currentDate.addDays(1);
    }
}


void TimelineDateScale::drawHourTicks(QPainter* painter)
{
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    // Use subtle teal for hour ticks
    painter->setPen(QPen(QColor(100, 150, 160), 0));
    QFont hourFont = painter->font();
    hourFont.setPointSize(7);
    painter->setFont(hourFont);

    int labelInterval = getHourLabelInterval();

    while (currentDate <= endDate)
    {
        for (int hour = 0; hour < 24; ++hour)
        {
            QDateTime dateTime(currentDate, QTime(hour, 0));
            double xPos = mapper_->dateTimeToX(dateTime);

            painter->drawLine(QPointF(xPos, SCALE_HEIGHT - HOUR_TICK_HEIGHT),
                              QPointF(xPos, SCALE_HEIGHT));

            if (shouldShowHourLabels() && (hour % labelInterval == 0))
            {
                QString hourLabel = QString("%1:00").arg(hour, 2, 10, QChar('0'));
                QRectF textRect(xPos - 15, SCALE_HEIGHT - 42, 30, 12);

                // Use darker teal for hour labels
                painter->setPen(QColor(70, 110, 120));
                painter->drawText(textRect, Qt::AlignCenter, hourLabel);

                // Restore tick color
                painter->setPen(QPen(QColor(100, 150, 160), 0));
            }
        }

        currentDate = currentDate.addDays(1);
    }
}


void TimelineDateScale::drawHalfHourTicks(QPainter* painter)
{
    QDate currentDate = paddedStart_;
    QDate endDate = paddedEnd_;

    // Use very light teal for half-hour ticks (most subtle)
    painter->setPen(QPen(QColor(140, 170, 180), 0));

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

    // Very subtle grid lines
    painter->setPen(QPen(QColor(220, 230, 240), 0, Qt::DashLine));

    // At high zoom, draw grid lines for days
    if (shouldShowDayTicks() && !shouldShowHourTicks())
    {
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
            for (int hour = 0; hour < 24; hour += 6)
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
    // Show hour ticks when there's at least 8 pixels per hour
    // At 192 ppd: 192/24 = 8 pixels per hour (user-friendly spacing)
    return mapper_->pixelsPerday() >= 192.0;
}


bool TimelineDateScale::shouldShowHalfHourTicks() const
{
    // Show half-hour ticks when there's at least 20 pixels per half-hour
    // At 960 ppd: 960/24 = 40 pixels per hour = 20 pixels per half-hour
    // This provides comfortable spacing for snapping interactions
    return mapper_->pixelsPerday() >= 960.0;
}


bool TimelineDateScale::shouldShowHourLabels() const
{
    // Show hour labels when there's enough space (at least 16 pixels per hour)
    // At 384 ppd: 384/24 = 16 pixels per hour
    return mapper_->pixelsPerday() >= 384.0;
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





