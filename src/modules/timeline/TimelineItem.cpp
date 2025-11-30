// TimelineItem.cpp (Enhanced)


#include "TimelineItem.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include <QGraphicsSceneMouseEvent>
#include <QPen>


TimelineItem::TimelineItem(const QRectF& rect, QGraphicsItem* parent)
    : QGraphicsRectItem(rect, parent)
{
    // Enable the item to be movable by the user with mouse dragging
    setFlag(QGraphicsItem::ItemIsMovable);

    // Enable selection highlighting and interaction
    setFlag(QGraphicsItem::ItemIsSelectable);

    // Send position changes through itemChange
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);

    // Set default brush (will be overriden by scene)
    setBrush(QBrush(Qt::blue));

    // Set pen for border
    setPen(QPen(Qt::black, 1));
}


QVariant TimelineItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged && scene())
    {
        // Constrain movement to horizontal only (preserve lane)
        QPointF newPos = value.toPointF();

        // Keep Y position fixed to preserve lane assignment
        newPos.setY(rect().y());

        return newPos;
    }

    if (change == ItemPositionHasChanged && isDragging_)
    {
        // Position changed during drag - could update preview here
        // For now, we'll update the model only on release
    }

    return QGraphicsRectItem::itemChange(change, value);
}


void TimelineItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    isDragging_ = true;
    dragStartPos_ = pos();
    QGraphicsRectItem::mousePressEvent(event);
}


void TimelineItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    isDragging_ = false;

    // Check if position actually changed
    if (pos() != dragStartPos_)
    {
        updateModelFromPosition();
    }

    QGraphicsRectItem::mouseReleaseEvent(event);
}


void TimelineItem::updateModelFromPosition()
{
    if (!model_ || !mapper_ || eventId_.isEmpty())
    {
        return;
    }

    // Get current event data
    const TimelineEvent* currentEvent = model_->getEvent(eventId_);
    if (!currentEvent)
    {
        return;
    }

    // Calculate new start date based on current X position
    // Use rect().x() which includes the item's horizontal offset
    double xPos = rect().x() + pos().x();
    QDate newStartDate = mapper_->xToDate(xPos);

    // Calculate duration to preserve event length
    int duration = currentEvent->startDate.daysTo(currentEvent->endDate);
    QDate newEndDate = newStartDate.addDays(duration);

    // Create updated event
    TimelineEvent updatedEvent = *currentEvent;
    updatedEvent.startDate = newStartDate;
    updatedEvent.endDate = newEndDate;
    // Lane is preserved - model will recalculate if needed

    // Update model (this will trigger lane recalculation and eventUpdated signal)
    model_->updateEvent(eventId_, updatedEvent);

    // IMPORTANT: Don't manually update position here
    // The scene will update our visual representation via the eventUpdated signal
    // This ensures proper lane assignment after the drag
}

