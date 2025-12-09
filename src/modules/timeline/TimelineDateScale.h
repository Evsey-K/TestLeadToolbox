// TimelineDateScale.h


#pragma once
#include <QGraphicsItem>
#include <QDate>


// Forward declaration
class TimelineCoordinateMapper;


/**
 * @class TimelineDateScale
 * @brief Renders date tick marks and labels along the timeline
 *
 * This component draws:
 * - Major ticks for months (with labels)
 * - Minor ticks for weeks
 * - Day ticks (when zoomed in enough)
 * - Hour ticks (when deeply zoomed)
 * - Half-hour ticks (when very deeply zoomed)
 * - Grid lines extending down through the timeline
 *
 * The scale adapts its rendering based on zoom level to maintain readability
 * and visual appeal across all zoom ranges.
 */
class TimelineDateScale : public QGraphicsItem
{
public:
    /**
     * @brief Construct a date scale for the timeline
     * @param mapper Coordinate mapping utility
     * @param parent Optional parent graphics item
     */
    explicit TimelineDateScale(TimelineCoordinateMapper* mapper, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;                                                                               ///< @brief Required by QGraphicsItem - defines the bounding rectangle
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;          ///< @brief Paint the date scale with ticks and labels
    void updateScale();                                                                                                 ///< @brief Update the scale when timeline bounds change
    void setTimelineHeight(double height);                                                                              ///< @brief Set the padded date range for rendering ticks beyond version dates
    void setPaddedDateRange(const QDate& paddedStart, const QDate& paddedEnd);                                          ///< @brief Set the padded date range for rendering ticks beyond version dates

    // Visual constants
    static constexpr double SCALE_HEIGHT = 70.0;            ///< Height of date scale area (increased for hour labels)
    static constexpr double MAJOR_TICK_HEIGHT = 25.0;       ///< Height of month ticks
    static constexpr double MINOR_TICK_HEIGHT = 15.0;       ///< Height of week ticks
    static constexpr double DAY_TICK_HEIGHT = 10.0;         ///< Height of day ticks
    static constexpr double HOUR_TICK_HEIGHT = 8.0;         ///< Height of hour ticks
    static constexpr double HALF_HOUR_TICK_HEIGHT = 5.0;    ///< Height of half-hour ticks

private:
    void drawMonthTicks(QPainter* painter);         ///< @brief Draw major ticks (months) with labels
    void drawWeekTicks(QPainter* painter);          ///< @brief Draw minor ticks (weeks)
    void drawDayTicks(QPainter* painter);           ///< @brief Draw day ticks (only when zoomed in)
    void drawHourTicks(QPainter* painter);          ///< @brief Draw hour ticks (only when deeply zoomed in)
    void drawHalfHourTicks(QPainter* painter);      ///< @brief Draw half-hour ticks (only when very deeply zoomed in)
    void drawGridLines(QPainter* painter);          ///< @brief Draw vertical grid lines
    bool shouldShowDayTicks() const;                ///< @brief Check if zoom level is sufficient for day ticks
    bool shouldShowWeekTicks() const;               ///< @brief Check if zoom level is sufficient for week ticks
    bool shouldShowHourTicks() const;               ///< @brief Check if zoom level is sufficient for hour ticks
    bool shouldShowHalfHourTicks() const;           ///< @brief Check if zoom level is sufficient for half-hour ticks
    bool shouldShowHourLabels() const;              ///< @brief Check if zoom level is sufficient for hour labels
    int getHourLabelInterval() const;               ///< @brief Get the hour label interval based on zoom level (returns 1, 2, 3, 4, 6, or 12)

    TimelineCoordinateMapper* mapper_;      ///< Coordinate mapper (not owned)
    double timelineHeight_;                 ///< Height of timeline for grid lines    
    QDate paddedStart_;                     ///< Padded start date (includes buffer)
    QDate paddedEnd_;                       ///< Padded end date (includes buffer)
};
