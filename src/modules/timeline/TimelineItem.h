// TimelineItem.h


#pragma once
#include <QGraphicsRectItem>
#include <QBrush>
#include <QString>


class TimelineModel;
class TimelineCoordinateMapper;


/**
 * @class TimelineItem
 * @brief Represents a single draggable event item on the timeline
 *
 * This item:
 * - Displays an event as a colored rectangle
 * - Can be dragged horizontally to change dates
 * - Updates the model when dragging completes
 * - Supports selection highlighting
 */
class TimelineItem : public QGraphicsRectItem
{
public:

    /**
     * @brief Constructs a TimelineItem with the specified rectangle geometry
     * @param rect Initial rectangle (position and size)
     * @param parent Optional parent graphics item
     */
    explicit TimelineItem(const QRectF& rect, QGraphicsItem* parent = nullptr);

    /**
     * @brief Set the event ID this item represents
     */
    void setEventId(const QString& eventId)
    {
        eventId_ = eventId;
    }

    /**
     * @brief Get the event ID
     */
    QString eventId() const
    {
        return eventId_;
    }

    /**
     * @brief Set the model reference (required for updates)
     */
    void setModel(TimelineModel* model)
    {
        model_ = model;
    }

    /**
     * @brief Set the coordinate mapper (required for date conversion)
     */
    void setCoordinateMapper(TimelineCoordinateMapper* mapper)
    {
        mapper_ = mapper;
    }

protected:
    /**
     * @brief Override to handle drag completion and update model
     */
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    /**
     * @brief Track when drag starts
     */
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    /**
     * @brief Track when drag ends
     */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    /**
     * @brief Update the model with new dates based on current position
     */
    void updateModelFromPosition();

    QString eventId_;                               ///< ID of the event this item represents
    TimelineModel* model_;                          ///< Model reference (not owned)
    TimelineCoordinateMapper* mapper_ = nullptr;    ///< Coordinate mapper (not owned)
    QPointF dragStartPos_;                          ///< Position when drag started
    bool isDragging_ = false;                        ///< Whether currently being draggeds
};
