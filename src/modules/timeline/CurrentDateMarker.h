// CurrentDateMarker.h


#pragma once
#include <QGraphicsItem>
#include <QDate>


// Forward declaration
class TimelineCoordinateMapper;


/**
 * @class CurrentDateMarker
 * @brief Renders a vertical line indicating the current date
 *
 * This component draws a prominent red vertical line at today's date,
 * helping users quickly identify the current point in time on the timeline.
 */
class CurrentDateMarker : public QGraphicsItem
{
public:
    /**
     * @brief Construct a current date marker
     * @param mapper Coordinate mapping utility
     * @param parent Optional parent graphics item
     */
    explicit CurrentDateMarker(TimelineCoordinateMapper* mapper, QGraphicsItem* parent = nullptr);

    /**
     * @brief Required by QGraphicsItem - defines the bounding rectangle
     */
    QRectF boundingRect() const override;

    /**
     * @brief Paint the current date marker
     */
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    /**
     * @brief Update marker position (call when date changes or zoom changes)
     */
    void updatePosition();

    /**
     * @brief Set the height of the timeline
     */
    void setTimelineHeight(double height);

private:
    TimelineCoordinateMapper* mapper_;      ///< Coordinate mapper (not owned)
    double timelineHeight_;                 ///< Height of marker line
    QDate currentDate_;                     ///< Today's date
};
