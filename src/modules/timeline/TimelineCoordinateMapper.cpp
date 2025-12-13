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

