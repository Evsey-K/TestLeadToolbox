// TimelineItem.cpp (Enhanced with Multi-Drag Support - FIXED)

#include "TimelineItem.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsScene>
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
    if (change == ItemPositionHasChanged && scene())
    {
        if (isMultiDragging_)
        {
            // Multi-drag: move all selected items together
            QPointF newPos = value.toPointF();
            QPointF delta = newPos - dragStartPos_;

            // Keep Y position fixed to preserve lane assignment
            delta.setY(0);

            // Move all other selected items by the same delta
            for (auto it = multiDragStartPositions_.constBegin();
                 it != multiDragStartPositions_.constEnd(); ++it)
            {
                QGraphicsItem* item = it.key();
                if (item != this) // Don't move self again
                {
                    QPointF newItemPos = it.value() + delta;
                    newItemPos.setY(it.value().y()); // Preserve Y
                    item->setPos(newItemPos);
                }
            }

            // Return corrected position with Y preserved
            return QPointF(newPos.x(), dragStartPos_.y());
        }
        else if (isDragging_ && !isResizing_)
        {
            // Single-drag: constrain movement to horizontal only (preserve lane)
            QPointF newPos = value.toPointF();
            newPos.setY(dragStartPos_.y());
            return newPos;
        }
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

    // Store starting position
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

    // Check if this item is already selected and there are multiple selections
    if (isSelected() && scene())
    {
        QList<QGraphicsItem*> selectedItems = scene()->selectedItems();
        if (selectedItems.size() > 1)
        {
            // Multi-drag mode: record all selected item positions
            isMultiDragging_ = true;
            multiDragStartPositions_.clear();

            for (QGraphicsItem* item : selectedItems)
            {
                multiDragStartPositions_[item] = item->pos();
            }
        }
        else
        {
            // Single item selected
            isDragging_ = true;
        }
    }
    else
    {
        // Item not selected or no multi-selection
        // Emit clicked signal to allow selection management
        if (!eventId_.isEmpty())
        {
            emit clicked(eventId_);
        }
        isDragging_ = true;
    }

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

    if (isDragging_ || isMultiDragging_)
    {
        QGraphicsObject::mouseMoveEvent(event);
    }
}

void TimelineItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (isResizing_)
        {
            // Resize completed - update model with new dates
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

        if (isMultiDragging_)
        {
            // Multi-drag completed - update all moved items in batch
            isMultiDragging_ = false;

            if (model_ && mapper_ && scene())
            {
                QList<QGraphicsItem*> selectedItems = scene()->selectedItems();

                // CRITICAL FIX: Block signals to prevent premature scene updates
                bool wasBlocked = model_->blockSignals(true);

                // Update all items in the model without triggering scene updates
                for (QGraphicsItem* item : selectedItems)
                {
                    TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(item);
                    if (timelineItem && !timelineItem->eventId().isEmpty())
                    {
                        // Get current event data
                        const TimelineEvent* currentEvent = model_->getEvent(timelineItem->eventId());
                        if (!currentEvent)
                        {
                            continue;
                        }

                        // Calculate new start date based on current X position
                        QPointF itemPos = timelineItem->pos();
                        QRectF itemRect = timelineItem->rect();
                        double xPos = itemRect.x() + itemPos.x();
                        QDate newStartDate = mapper_->xToDate(xPos);

                        // Calculate duration to preserve event length
                        int duration = currentEvent->startDate.daysTo(currentEvent->endDate);
                        QDate newEndDate = newStartDate.addDays(duration);

                        // Create updated event
                        TimelineEvent updatedEvent = *currentEvent;
                        updatedEvent.startDate = newStartDate;
                        updatedEvent.endDate = newEndDate;

                        // Update model (signals are blocked, so no scene update yet)
                        model_->updateEvent(timelineItem->eventId(), updatedEvent);
                    }
                }

                // Re-enable signals and trigger a single lane recalculation
                model_->blockSignals(wasBlocked);

                // Manually trigger lane recalculation for all updated events
                // This will cause a single scene rebuild instead of one per item
                if (!selectedItems.isEmpty())
                {
                    // The model should recalculate lanes and emit appropriate signals
                    // We can force this by calling a method or emitting a signal
                    // For now, we'll emit the lanesRecalculated signal if available
                    // Or we can call updateEvent on the first item with signals enabled

                    // Option 1: If model has a method to force lane recalculation
                    // model_->recalculateLanes(); // <-- If this method exists

                    // Option 2: Trigger one update with signals enabled
                    TimelineItem* firstItem = qgraphicsitem_cast<TimelineItem*>(selectedItems.first());
                    if (firstItem && !firstItem->eventId().isEmpty())
                    {
                        const TimelineEvent* evt = model_->getEvent(firstItem->eventId());
                        if (evt)
                        {
                            // This will trigger lane recalculation and scene update
                            model_->updateEvent(firstItem->eventId(), *evt);
                        }
                    }
                }
            }

            multiDragStartPositions_.clear();
            event->accept();
            return;
        }

        if (isDragging_)
        {
            // Single drag completed - update model with new dates
            isDragging_ = false;

            // Check if position actually changed
            if (pos() != dragStartPos_)
            {
                updateModelFromPosition();
            }

            event->accept();
            return;
        }
    }

    QGraphicsObject::mouseReleaseEvent(event);
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

void TimelineItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!resizable_)
    {
        QGraphicsObject::hoverMoveEvent(event);
        return;
    }

    ResizeHandle handle = getResizeHandle(event->pos());

    if (handle == LeftEdge || handle == RightEdge)
    {
        setCursor(Qt::SizeHorCursor);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }

    QGraphicsObject::hoverMoveEvent(event);
}

void TimelineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setCursor(Qt::ArrowCursor);
    QGraphicsObject::hoverLeaveEvent(event);
}

void TimelineItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;
    QAction* editAction = menu.addAction("✏️ Edit Event");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("🗑️ Delete Event");

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

TimelineItem::ResizeHandle TimelineItem::getResizeHandle(const QPointF& pos) const
{
    if (!resizable_)
    {
        return None;
    }

    const double edgeThreshold = 8.0; // pixels

    if (qAbs(pos.x() - rect_.left()) < edgeThreshold)
    {
        return LeftEdge;
    }
    else if (qAbs(pos.x() - rect_.right()) < edgeThreshold)
    {
        return RightEdge;
    }

    return None;
}
