// VersionBoundaryMarker.h


#pragma once
#include <QGraphicsItem>
#include <QDate>


    // Forward declaration
    class TimelineCoordinateMapper;


/**
 * @class VersionBoundaryMarker
 * @brief Renders a vertical line indicating version start or end date
 *
 * This component draws a prominent vertical line at the version boundary,
 * helping users identify the timeline scope. Smaller and different color
 * than the current date marker.
 */
class VersionBoundaryMarker : public QGraphicsItem
{
public:
    enum MarkerType {
        VersionStart,
        VersionEnd
    };

    /**
     * @brief Construct a version boundary marker
     * @param mapper Coordinate mapping utility
     * @param type Start or end marker
     * @param parent Optional parent graphics item
     */
    explicit VersionBoundaryMarker(TimelineCoordinateMapper* mapper,
                                   MarkerType type,
                                   QGraphicsItem* parent = nullptr);

    /**
     * @brief Required by QGraphicsItem - defines the bounding rectangle
     */
    QRectF boundingRect() const override;

    /**
     * @brief Paint the version boundary marker
     */
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    /**
     * @brief Update marker position (call when dates change or zoom changes)
     */
    void updatePosition();

    /**
     * @brief Set the height of the timeline
     */
    void setTimelineHeight(double height);

private:
    TimelineCoordinateMapper* mapper_;      ///< Coordinate mapper (not owned)
    MarkerType type_;                       ///< Start or end marker
    double timelineHeight_;                 ///< Height of marker line
    QDate markerDate_;                      ///< Version boundary date
};
