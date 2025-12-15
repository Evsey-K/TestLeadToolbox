// TimelineCoordinateMapper.cpp


#include "TimelineCoordinateMapper.h"
#include <algorithm>
#include <cmath>


TimelineCoordinateMapper::TimelineCoordinateMapper(const QDate& versionStart,
                                                   const QDate& versionEnd,
                                                   double pixelsPerday)
    : versionStart_(versionStart)
    , versionEnd_(versionEnd)
    , pixelsPerDay_(pixelsPerday)
    , minPixelsPerDay_(ABSOLUTE_MIN_PIXELS_PER_DAY)
{
}


double TimelineCoordinateMapper::dateToX(const QDate& date) const
{
    if(!date.isValid())
    {
        return 0.0;
    }

    // Calculate days from version start
    int dayOffset = versionStart_.daysTo(date);

    // Convert to pixel coordinate
    return dayOffset * pixelsPerDay_;
}


double TimelineCoordinateMapper::dateTimeToX(const QDateTime& dateTime) const
{
    if(!dateTime.isValid())
    {
        return 0.0;
    }

    // Calculate days from version start
    int dayOffset = versionStart_.daysTo(dateTime.date());

    // Add fractional day based on time
    double fractionalDay = dateTime.time().hour() / 24.0 +
                           dateTime.time().minute() / (1440.0) +
                           dateTime.time().second() / (86400.0);

    // Convert to pixel coordinate
    return (dayOffset + fractionalDay) * pixelsPerDay_;
}


QPointF TimelineCoordinateMapper::dateToPoint(const QDate& date, double yPos) const
{
    return QPointF(dateToX(date), yPos);
}


QRectF TimelineCoordinateMapper::dateRangeToRect(const QDate& start,
                                                 const QDate& end,
                                                 double yPos,
                                                 double height) const
{
    // Calculate inclusive end (add 1 day to visually include the end date)
    QDate inclusiveEnd = end.addDays(1);

    double x1 = dateToX(start);
    double x2 = dateToX(inclusiveEnd);
    double width = x2 - x1;

    return QRectF(x1, yPos, width, height);
}


QRectF TimelineCoordinateMapper::dateTimeRangeToRect(const QDateTime& start,
                                                     const QDateTime& end,
                                                     double yPos,
                                                     double height) const
{
    double x1 = dateTimeToX(start);
    double x2 = dateTimeToX(end);
    double width = x2 - x1;

    return QRectF(x1, yPos, width, height);
}


QDate TimelineCoordinateMapper::xToDate(double xCoord) const
{
    // Convert pixel position back to days offset
    int daysOffset = static_cast<int>(std::round(xCoord / pixelsPerDay_));

    // Add offset to version start
    return versionStart_.addDays(daysOffset);
}


QDateTime TimelineCoordinateMapper::xToDateTime(double xCoord) const
{
    // Convert pixel position to days offset (with fractional part)
    double daysOffsetFloat = xCoord / pixelsPerDay_;
    int daysOffset = static_cast<int>(std::floor(daysOffsetFloat));
    double fractionalDay = daysOffsetFloat - daysOffset;

    // Calculate time from fractional day - ROUND instead of truncate
    int totalSeconds = static_cast<int>(std::round(fractionalDay * 86400.0));
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    // Clamp values to valid ranges
    hours = qBound(0, hours, 23);
    minutes = qBound(0, minutes, 59);
    seconds = qBound(0, seconds, 59);

    // Handle edge case where rounding pushes us to the next day
    if (totalSeconds >= 86400)
    {
        daysOffset++;
        totalSeconds = 0;
    }

    // Construct date-time
    QDate date = versionStart_.addDays(daysOffset);
    QTime time(hours, minutes, seconds);

    return QDateTime(date, time);
}


int TimelineCoordinateMapper::daysBetween(const QDate& start, const QDate& end) const
{
    return start.daysTo(end);
}


void TimelineCoordinateMapper::setPixelsPerDay(double ppd)
{
    pixelsPerDay_ = std::clamp(ppd, minPixelsPerDay_, MAX_PIXELS_PER_DAY);
}


void TimelineCoordinateMapper::zoom(double factor)
{
    double newScale = pixelsPerDay_ * factor;
    setPixelsPerDay(newScale);
}


void TimelineCoordinateMapper::setMinPixelsPerDay(double minPpd)
{
    // Ensure minimum doesn't go below absolute floor
    minPixelsPerDay_ = qMax(minPpd, ABSOLUTE_MIN_PIXELS_PER_DAY);

    // If current zoom is now below the new minimum, clamp it
    if (pixelsPerDay_ < minPixelsPerDay_)
    {
        pixelsPerDay_ = minPixelsPerDay_;
    }
}


void TimelineCoordinateMapper::setVersionDates(const QDate& start, const QDate& end)
{
    if(start.isValid() && end.isValid() && start <= end)
    {
        versionStart_ = start;
        versionEnd_ = end;
    }
}


double TimelineCoordinateMapper::totalWidth() const
{
    int totalDays = versionStart_.daysTo(versionEnd_) + 1;

    return totalDays * pixelsPerDay_;
}


double TimelineCoordinateMapper::snapXToNearestTick(double xCoord) const
{
    // Determine snap interval based on zoom level
    // These thresholds MUST match TimelineDateScale rendering logic

    if (pixelsPerDay_ >= 960.0)
    {
        // Very deep zoom: snap to half-hour intervals (30 minutes)
        QDateTime dateTime = xToDateTime(xCoord);

        // Round to nearest 30 minutes
        int totalMinutes = dateTime.time().hour() * 60 + dateTime.time().minute();
        int roundedMinutes = static_cast<int>(std::round(totalMinutes / 30.0)) * 30;

        int hours = roundedMinutes / 60;
        int minutes = roundedMinutes % 60;

        QDate targetDate = dateTime.date();
        if (hours >= 24)
        {
            hours = 0;
            targetDate = targetDate.addDays(1);
        }

        QDateTime snappedDateTime(targetDate, QTime(hours, minutes, 0));
        return dateTimeToX(snappedDateTime);
    }
    else if (pixelsPerDay_ >= 192.0)
    {
        // Deep zoom: snap to hour intervals
        QDateTime dateTime = xToDateTime(xCoord);

        qDebug() << "Hour snap: Input xCoord =" << xCoord << "-> DateTime =" << dateTime;

        // Round to nearest hour
        double totalHours = dateTime.time().hour() + dateTime.time().minute() / 60.0;
        int roundedHour = static_cast<int>(std::round(totalHours));

        QDate targetDate = dateTime.date();
        if (roundedHour >= 24)
        {
            roundedHour = 0;
            targetDate = targetDate.addDays(1);
        }

        QDateTime snappedDateTime(targetDate, QTime(roundedHour, 0, 0));
        double snappedX = dateTimeToX(snappedDateTime);

        qDebug() << "Snapped to:" << snappedDateTime << "at X =" << snappedX;

        return snappedX;
    }
    else if (pixelsPerDay_ >= 10.0)
    {
        // Medium zoom: snap to day boundaries
        QDate date = xToDate(xCoord);
        double snappedX = dateToX(date);

        qDebug() << "Day snap: Input xCoord =" << xCoord << "-> Date =" << date << "at X =" << snappedX;

        return snappedX;
    }
    else if (pixelsPerDay_ >= 3.0)
    {
        // Low zoom: snap to week boundaries (Monday)
        QDate date = xToDate(xCoord);

        // Find distance to Monday
        int daysFromMonday = date.dayOfWeek() - Qt::Monday;
        if (daysFromMonday < 0)
        {
            daysFromMonday += 7;
        }

        // Snap to nearest Monday
        QDate snappedDate;
        if (daysFromMonday <= 3)
        {
            // Closer to previous/current Monday
            snappedDate = date.addDays(-daysFromMonday);
        }
        else
        {
            // Closer to next Monday
            snappedDate = date.addDays(7 - daysFromMonday);
        }

        return dateToX(snappedDate);
    }
    else
    {
        // Very low zoom: snap to month boundaries (1st of month)
        QDate date = xToDate(xCoord);

        // Get first day of current month and next month
        QDate firstOfMonth(date.year(), date.month(), 1);
        QDate firstOfNextMonth = firstOfMonth.addMonths(1);

        // Calculate which is closer
        int daysFromStart = firstOfMonth.daysTo(date);
        int daysToEnd = date.daysTo(firstOfNextMonth);

        QDate snappedDate = (daysFromStart < daysToEnd) ? firstOfMonth : firstOfNextMonth;

        return dateToX(snappedDate);
    }
}


QDate TimelineCoordinateMapper::xToDateSnapped(double xCoord) const
{
    double snappedX = snapXToNearestTick(xCoord);

    if (pixelsPerDay_ >= 192.0)
    {
        // For hour precision, use DateTime
        QDateTime dt = xToDateTime(snappedX);
        return dt.date();
    }
    else
    {
        // For day/week/month precision, use standard Date conversion
        return xToDate(snappedX);
    }
}


QDateTime TimelineCoordinateMapper::xToDateTimeSnapped(double xCoord) const
{
    double snappedX = snapXToNearestTick(xCoord);
    return xToDateTime(snappedX);
}
