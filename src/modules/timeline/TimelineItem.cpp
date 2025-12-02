// TimelineItem.cpp (Enhanced)


#include "TimelineItem.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPen>
#include <QCursor>


TimelineItem::TimelineItem(const QRectF& rect, QGraphicsItem* parent)
    : QGraphicsRectItem(rect, parent)
{
    // Enable the item to be movable by the user with mouse dragging
    setFlag(QGraphicsItem::ItemIsMovable);

    // Enable selection highlighting and interaction
    setFlag(QGraphicsItem::ItemIsSelectable);

    // Send position changes through itemChange
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);

    // Enable hover events for resize cursor
    setAcceptHoverEvents(true);

    // Set default brush (will be overriden by scene)
    setBrush(QBrush(Qt::blue));

    // Set pen for border
    setPen(QPen(Qt::black, 1));
}


QVariant TimelineItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged && scene() && isDragging_ && !isResizing_)
    {
        // Constrain movement to horizontal only (preserve lane)
        QPointF newPos = value.toPointF();

        // Keep Y position fixed to preserve lane assignment
        newPos.setY(dragStartPos_.y());

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
    if (event->button() != Qt::LeftButton)
    {
        QGraphicsRectItem::mousePressEvent(event);

        return;
    }

    dragStartPos_ = pos();
    resizeStartRect_ = rect();

    // Check if clicking on a resize handle
    if (resizable_)
    {
        ResizeHandle handle = getResizeHandle(event->pos());
        if (handle != None)
        {
            isResizing_ = true;
            activeHandle_ = handle;
            event->accept();

            return;
        }
    }

    // Otherwise, it's a drag operation
    isDragging_ = true;
    QGraphicsRectItem::mousePressEvent(event);
}


void TimelineItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (isResizing_)
    {
        QRectF newRect = rect();
        QPointF delta = event->pos() - event->lastPos();

        if (activeHandle_ == LeftEdge)
        {
            // Resize from left edge
            double newLeft = newRect.left() + delta.x();
            double minWidth = 10.0; // Minimum width

            if (newRect.right() - newLeft >= minWidth)
            {
                newRect.setLeft(newLeft);
                setRect(newRect);
            }
        }
        else if (activeHandle_ == RightEdge)
        {
            // Resize from right edge
            double newRight = newRect.right() + delta.x();
            double minWidth = 10.0;

            if (newRight - newRect.left() >= minWidth)
            {
                newRect.setRight(newRight);
                setRect(newRect);
            }
        }

        event->accept();
        return;
    }

    if (isDragging_)
    {
        QGraphicsRectItem::mouseMoveEvent(event);
    }
}


void TimelineItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QGraphicsRectItem::mouseReleaseEvent(event);

        return;
    }

    if (isResizing_)
    {
        // Resize operation completed
        isResizing_ = false;
        activeHandle_ = None;

        // Check if size actually changed
        if (rect() != resizeStartRect_)
        {
            updateModelFromSize();
        }

        event->accept();

        return;
    }

    if (isDragging_)
    {
        // Drag operation completed
        isDragging_ = false;

        // Check if position actually changed
        if (pos() != dragStartPos_)
        {
            updateModelFromPosition();
        }
    }

    QGraphicsRectItem::mouseReleaseEvent(event);
}


void TimelineItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!resizable_ || isResizing_ || isDragging_)
    {
        QGraphicsRectItem::hoverMoveEvent(event);

        return;
    }

    ResizeHandle handle = getResizeHandle(event->pos());
    updateCursor(handle);

    QGraphicsRectItem::hoverMoveEvent(event);
}


void TimelineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!isDragging_ && !isResizing_)
    {
        unsetCursor();
    }

    QGraphicsRectItem::hoverLeaveEvent(event);
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
    double xPos = rect().x() + pos().x();
    QDate newStartDate = mapper_->xToDate(xPos);

    // Calculate duration to preserve event length
    int duration = currentEvent->startDate.daysTo(currentEvent->endDate);
    QDate newEndDate = newStartDate.addDays(duration);

    // Create updated event
    TimelineEvent updatedEvent = *currentEvent;
    updatedEvent.startDate = newStartDate;
    updatedEvent.endDate = newEndDate;

    // Update model (this will trigger lane recalculation and eventUpdated signal)
    model_->updateEvent(eventId_, updatedEvent);
}


void TimelineItem::updateModelFromSize()
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

    // Calculate new dates based on current rectangle
    QDate newStartDate = mapper_->xToDate(rect().left() + pos().x());
    QDate newEndDate = mapper_->xToDate(rect().right() + pos().x());

    // Ensure at least 1-day duration
    if (newEndDate < newStartDate)
    {
        newEndDate = newStartDate;
    }

    // Create updated event
    TimelineEvent updatedEvent = *currentEvent;
    updatedEvent.startDate = newStartDate;
    updatedEvent.endDate = newEndDate;

    // Update model
    model_->updateEvent(eventId_, updatedEvent);
}


TimelineItem::ResizeHandle TimelineItem::getResizeHandle(const QPointF& pos) const
{
    QRectF bounds = rect();

    // Check left edge
    if (qAbs(pos.x() - bounds.left()) <= RESIZE_HANDLE_WIDTH)
    {
        return LeftEdge;
    }

    // Check right edge
    if (qAbs(pos.x() - bounds.right()) <= RESIZE_HANDLE_WIDTH)
    {
        return RightEdge;
    }

    return None;
}


void TimelineItem::updateCursor(ResizeHandle handle)
{
    switch (handle)
    {
    case LeftEdge:
    case RightEdge:
        setCursor(Qt::SizeHorCursor);
        break;
    case None:
    default:
        unsetCursor();
        break;
    }
}
