// CurrentDateMarker.h


#pragma once
#include <QGraphicsItem>
#include <QDateTime>


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
    explicit CurrentDateMarker(TimelineCoordinateMapper* mapper, QGraphicsItem* parent = nullptr);                  ///< @brief Construct a current date marker

    QRectF boundingRect() const override;                                                                           ///< @brief Required by QGraphicsItem - defines the bounding rectangle
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;      ///< @brief Paint the current date marker
    void updatePosition();                                                                                          ///< @brief Update marker position (call when date changes or zoom changes)
    void setTimelineHeight(double height);                                                                          ///< @brief Set the height of the timeline

private:
    TimelineCoordinateMapper* mapper_;      ///< Coordinate mapper (not owned)
    double timelineHeight_;                 ///< Height of marker line
    QDateTime currentDateTime_;             ///< Current date and time (was QDate currentDate_)
    double snappedXPosition_;               ///< Cached X position snapped to tick
};
