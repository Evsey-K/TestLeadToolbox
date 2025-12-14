// TimelineCoordinateMapper.h


#pragma once
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QPointF>
#include <QRectF>


/**
 * @class TimelineCoordinateMapper
 * @brief Utility class for converting between dates and scene coordinates
 *
 * This class handles the mathematical mapping between calendar dates and
 * pixel positions on the timeline canvas. It supports dynamic zoom levels,
 * including fine-grained zoom for hour and half-hour precision.
 */
class TimelineCoordinateMapper
{
public:
    /**
     * @brief Construct mapper with version date range and initial pixels per day
     * @param versionStart Start date of the timeline
     * @param versionEnd End date of the timeline
     * @param pixelsPerDay Initial scale factor (default: 20 pixels per day)
     */
    TimelineCoordinateMapper(const QDate& versionStart, const QDate& versionEnd, double pixelsPerDay = 20.0);


    // Date to coordinate conversion
    double dateToX(const QDate& date) const;
    double dateTimeToX(const QDateTime& dateTime) const;
    QPointF dateToPoint(const QDate& date, double yPos = 0.0) const;
    QRectF dateRangeToRect(const QDate& start, const QDate& end, double yPos, double height) const;
    QRectF dateTimeRangeToRect(const QDateTime& start, const QDateTime& end, double yPos, double height) const;


    // Coordinate to date conversion
    QDate xToDate(double xCoord) const;
    QDateTime xToDateTime(double xCoord) const;
    int daysBetween(const QDate& start, const QDate& end) const;


    // Zoom-aware snapping methods
    double snapXToNearestTick(double xCoord) const;         ///< @brief Snap X coordinate to nearest tick based on zoom level
    QDate xToDateSnapped(double xCoord) const;              ///< @brief Convert X to date with zoom-aware snapping
    QDateTime xToDateTimeSnapped(double xCoord) const;      ///< @brief Convert X to datetime with zoom-aware snapping


    // Zoom/scale management
    void setPixelsPerDay(double ppd);
    double pixelsPerday() const { return pixelsPerDay_; }
    void zoom(double factor); // Multiple current scale by factor


    // Version date range management
    void setVersionDates(const QDate& start, const QDate& end);
    QDate versionStart() const { return versionStart_; }
    QDate versionEnd() const { return versionEnd_; }


    // Calculate total timeline width
    double totalWidth() const;


    // Constants
    static constexpr double DEFAULT_PIXELS_PER_DAY = 20.0;
    static constexpr double MIN_PIXELS_PER_DAY = 2.0;
    static constexpr double MAX_PIXELS_PER_DAY = 2000.0;


private:
    QDate versionStart_;
    QDate versionEnd_;
    double pixelsPerDay_;
};


