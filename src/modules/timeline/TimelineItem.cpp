// TimelineItem.cpp (Enhanced)


#include "TimelineItem.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QCursor>
#include <QPainter>
#include <QPen>
#include <QMenu>
#include <QStyleOptionGraphicsItem>


TimelineItem::TimelineItem(const QRectF& rect, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , rect_(rect)
    , brush_(Qt::blue)
    , pen_(Qt::black, 1)
{
    // Enable the item to be movable by the user with mouse dragging
    setFlag(QGraphicsItem::ItemIsMovable);

    // Enable selection highlighting and interaction
    setFlag(QGraphicsItem::ItemIsSelectable);

    // Send position changes through itemChange
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);

    // Enable hover events for resize cursor
    setAcceptHoverEvents(true);
}


QRectF TimelineItem::boundingRect() const
{
    // Return the rectangle bounds (with some padding for pen width)
    return rect_.adjusted(-pen_.widthF()/2, -pen_.widthF()/2,
                          pen_.widthF()/2, pen_.widthF()/2);
}


void TimelineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* /*widget*/)
{
    painter->setBrush(brush_);
    painter->setPen(pen_);

    // Draw rectangle
    painter->drawRect(rect_);

    // Draw selection highlight if selected
    if (option->state & QStyle::State_Selected)
    {
        QPen highlightPen(Qt::yellow, 2, Qt::DashLine);
        painter->setPen(highlightPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect_);
    }
}


void TimelineItem::setRect(const QRectF& rect)
{
    prepareGeometryChange();
    rect_ = rect;
    update();
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

    return QGraphicsObject::itemChange(change, value);
}


void TimelineItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    dragStartPos_ = pos();
    resizeStartRect_ = rect_;

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
    QGraphicsObject::mousePressEvent(event);
}


void TimelineItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (isResizing_)
    {
        QRectF newRect = rect_;
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
        QGraphicsObject::mouseMoveEvent(event);
    }
}


void TimelineItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QGraphicsObject::mouseReleaseEvent(event);
        return;
    }

    if (isResizing_)
    {
        // Resize operation completed
        isResizing_ = false;
        activeHandle_ = None;

        // Check if size actually changed
        if (rect_ != resizeStartRect_)
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

    QGraphicsObject::mouseReleaseEvent(event);
}


void TimelineItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!resizable_ || isResizing_ || isDragging_)
    {
        QGraphicsObject::hoverMoveEvent(event);
        return;
    }

    ResizeHandle handle = getResizeHandle(event->pos());
    updateCursor(handle);

    QGraphicsObject::hoverMoveEvent(event);
}


void TimelineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!isDragging_ && !isResizing_)
    {
        unsetCursor();
    }

    QGraphicsObject::hoverLeaveEvent(event);
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
    double xPos = rect_.x() + pos().x();
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
    QDate newStartDate = mapper_->xToDate(rect_.left() + pos().x());
    QDate newEndDate = mapper_->xToDate(rect_.right() + pos().x()).addDays(-1);

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
    // Check left edge
    if (qAbs(pos.x() - rect_.left()) <= RESIZE_HANDLE_WIDTH)
    {
        return LeftEdge;
    }

    // Check right edge
    if (qAbs(pos.x() - rect_.right()) <= RESIZE_HANDLE_WIDTH)
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


void TimelineItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    // Create context menu (Phase 5)
    QMenu menu;

    QAction* editAction = menu.addAction("✏️ Edit Event");
    editAction->setToolTip("Open edit dialog for this event");

    menu.addSeparator();

    QAction* deleteAction = menu.addAction("🗑️ Delete Event");
    deleteAction->setToolTip("Delete this event permanently");

    QFont deleteFont = deleteAction->font();
    deleteFont.setBold(true);
    deleteAction->setFont(deleteFont);

    QAction* selected = menu.exec(event->screenPos());

    if (selected == editAction)
    {
        emit editRequested(eventId_);
    }
    else if (selected == deleteAction)
    {
        emit deleteRequested(eventId_);
    }

    event->accept();
}
