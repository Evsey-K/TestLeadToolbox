// TimelineCoordinateMapper.cpp


#include "TimelineCoordinateMapper.h"
#include <algorithm>
#include <cmath>


TimelineCoordinateMapper::TimelineCoordinateMapper(const QDate& versionStart, const QDate& versionEnd, double pixelsPerday)
    : versionStart_(versionStart), versionEnd_(versionEnd), pixelsPerDay_(pixelsPerday)
{
    // Clamp initial pixels per day to valid range
    pixelsPerDay_ = std::clamp(pixelsPerDay_, MIN_PIXELS_PER_DAY, MAX_PIXELS_PER_DAY);
}


double TimelineCoordinateMapper::dateToX(const QDate& date) const
{
    if(!date.isValid())
    {
        return 0.0;
    }

    // Calculate days from version start
    int days = versionStart_.daysTo(date);

    // Convert to pixel coordinate
    return days * pixelsPerDay_;
}


double TimelineCoordinateMapper::dateTimeToX(const QDateTime& dateTime) const
{
    if(!dateTime.isValid())
    {
        return 0.0;
    }

    // Calculate days from version start
    int days = versionStart_.daysTo(dateTime.date());

    // Calculate fractional day from time
    QTime time = dateTime.time();
    double fractionalDay = (time.hour() * 3600.0 + time.minute() * 60.0 + time.second()) / 86400.0;

    // Convert to pixel coordinate
    return (days + fractionalDay) * pixelsPerDay_;
}


QPointF TimelineCoordinateMapper::dateToPoint(const QDate& date, double yPos) const
{
    return QPointF(dateToX(date), yPos);
}


QRectF TimelineCoordinateMapper::dateRangeToRect(const QDate& start, const QDate& end, double yPos, double height) const
{
    double x1 = dateToX(start);
    double x2 = dateToX(end.addDays(1));    // +1 to include end date visually
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
    double daysOffset = xCoord / pixelsPerDay_;
    int wholeDays = static_cast<int>(std::floor(daysOffset));
    double fractionalDay = daysOffset - wholeDays;

    // Calculate time from fractional day
    int totalSeconds = static_cast<int>(fractionalDay * 86400.0);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    // Construct date-time
    QDate date = versionStart_.addDays(wholeDays);
    QTime time(hours, minutes, seconds);

    return QDateTime(date, time);
}


int TimelineCoordinateMapper::daysBetween(const QDate& start, const QDate& end) const
{
    return start.daysTo(end);
}


void TimelineCoordinateMapper::setPixelsPerDay(double ppd)
{
    pixelsPerDay_ = std::clamp(ppd, MIN_PIXELS_PER_DAY, MAX_PIXELS_PER_DAY);
}


void TimelineCoordinateMapper::zoom(double factor)
{
    double newScale = pixelsPerDay_ * factor;
    setPixelsPerDay(newScale);
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

    if (pixelsPerDay_ >= 960.0)  // ← Changed from 100.0 to 960.0
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
    else if (pixelsPerDay_ >= 192.0)  // ← Changed from 50.0 to 192.0
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

    if (pixelsPerDay_ >= 192.0)  // ← Changed from 50.0 to 192.0
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
