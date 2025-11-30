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

