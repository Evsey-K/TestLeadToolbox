// TimelineItem.h
#pragma once
#include <QGraphicsRectItem>
#include <QBrush>

/**
 * @class TimelineItem
 * @brief Represents a single event item on the timeline.
 *
 * This is a rectangular graphical item that can be selected and moved by the user.
 * It visually represents a timeline event bar.
 */
class TimelineItem : public QGraphicsRectItem {
public:
    /**
     * @brief Constructs a TimelineItem with the specified rectangle geometry.
     * @param rect Defines the size and shape of the item.
     */
    TimelineItem(const QRectF &rect) : QGraphicsRectItem(rect) {
        // Enable the item to be movable by the user with mouse dragging
        setFlag(QGraphicsItem::ItemIsMovable);

        // Enable selection highlighting and interaction
        setFlag(QGraphicsItem::ItemIsSelectable);

        // Set the fill color of the item to blue
        setBrush(QBrush(Qt::blue));
    }
};
