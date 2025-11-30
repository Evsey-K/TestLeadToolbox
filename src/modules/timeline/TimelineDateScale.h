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
 * - Grid lines extending down through the timeline
 *
 * The scale adapts its rendering based on zoom level to maintain readability.
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

    /**
     * @brief Required by QGraphicsItem - defines the bounding rectangle
     */
    QRectF boundingRect() const override;

    /**
     * @brief Paint the date scale with ticks and labels
     */
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    /**
     * @brief Update the scale when timeline bounds change
     */
    void updateScale();

    /**
     * @brief Set the height of the timeline (for grid lines)
     */
    void setTimelineHeight(double height);

    // Visual constants
    static constexpr double SCALE_HEIGHT = 60.0;            ///< Height of date scale area
    static constexpr double MAJOR_TICK_HEIGHT = 20.0;       ///< Height of month ticks
    static constexpr double MINOR_TICK_HEIGHT = 12.0;       ///< Height of week ticks
    static constexpr double DAY_TICK_HEIGHT = 6.0;          ///< Height of day ticks

private:
    /**
     * @brief Draw major ticks (months) with labels
     */
    void drawMonthTicks(QPainter* painter);

    /**
     * @brief Draw minor ticks (weeks)
     */
    void drawWeekTicks(QPainter* painter);

    /**
     * @brief Draw day ticks (only when zoomed in)
     */
    void drawDayTicks(QPainter* painter);

    /**
     * @brief Draw vertical grid lines
     */
    void drawGridLines(QPainter* painter);

    /**
     * @brief Check if zoom level is sufficient for day ticks
     */
    bool shouldShowDayTicks() const;

    /**
     * @brief Check if zoom level is sufficient for week ticks
     */
    bool shouldShowWeekTicks() const;

    TimelineCoordinateMapper* mapper_;      ///< Coordinate mapper (not owned)
    double timelineHeight_;                 ///< Height of timeline for grid lines
};
