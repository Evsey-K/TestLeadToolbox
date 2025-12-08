// TimelineItem.cpp


#include "TimelineItem.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineCommands.h"
#include "LaneAssigner.h"
#include <QCursor>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsScene>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <QUndoStack>


// Constants for lane calculation (must match TimelineScene)
constexpr double ITEM_HEIGHT = 30.0;
constexpr double LANE_SPACING = 5.0;
constexpr double DATE_SCALE_OFFSET = 40.0;


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

    // Draw lane control icon in bottom-right corner
    if (model_ && !eventId_.isEmpty())
    {
        const TimelineEvent* event = model_->getEvent(eventId_);
        if (event && event->laneControlEnabled)
        {
            // Icon parameters
            constexpr double ICON_SIZE = 12.0;
            constexpr double ICON_MARGIN = 2.0;

            // Position in bottom-right corner
            QRectF iconRect(
                rect_.right() - ICON_SIZE - ICON_MARGIN,
                rect_.bottom() - ICON_SIZE - ICON_MARGIN,
                ICON_SIZE,
                ICON_SIZE
                );

            // Draw icon background (white circle)
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 255, 255, 200));  // Semi-transparent white
            painter->drawEllipse(iconRect);

            // Draw lock icon (simplified)
            painter->setPen(QPen(QColor(76, 175, 80), 1.5));  // Green color
            painter->setBrush(Qt::NoBrush);

            // Lock body (rectangle)
            QRectF lockBody(
                iconRect.center().x() - 3,
                iconRect.center().y(),
                6,
                4
                );
            painter->drawRect(lockBody);

            // Lock shackle (arc)
            QRectF shackleRect(
                iconRect.center().x() - 2,
                iconRect.center().y() - 4,
                4,
                4
                );
            painter->drawArc(shackleRect, 0 * 16, 180 * 16);  // Top half circle
        }
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
    if (change == ItemPositionChange && scene())
    {
        // Get the event to check if lane control is enabled
        const TimelineEvent* event = nullptr;

        if (model_ && !eventId_.isEmpty())
        {
            event = model_->getEvent(eventId_);
        }

        bool laneControlEnabled = (event && event->laneControlEnabled);

        if (isMultiDragging_)
        {
            // Multi-drag: move all selected items together
            QPointF newPos = value.toPointF();
            QPointF delta = newPos - dragStartPos_;

            // Constrain movement based on lane control
            if (!laneControlEnabled)
            {
                delta.setY(0); // Preserve Y if lane control is disabled
            }

            // Move all other selected items by the same delta
            for (auto it = multiDragStartPositions_.constBegin();
                 it != multiDragStartPositions_.constEnd(); ++it)
            {
                QGraphicsItem* item = it.key();
                if (item != this) // Don't move self again
                {
                    QPointF newItemPos = it.value() + delta;

                    if (!laneControlEnabled)
                    {
                        newItemPos.setY(it.value().y()); // Preserve Y
                    }
                    item->setPos(newItemPos);
                }
            }

            // Return corrected position
            QPointF finalPos = dragStartPos_ + delta;

            return finalPos;
        }
        else if (isDragging_ && !isResizing_)
        {
            // Single-drag: constrain movement based on lane control
            QPointF newPos = value.toPointF();

            if (!laneControlEnabled)
            {
                // If lane control is disabled, preserve Y (horizontal movement only)
                newPos.setY(dragStartPos_.y());
            }
            // If lane control is enabled, allow both X and Y movement

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

            // Always emit clicked signal to update detail panel
            if (!eventId_.isEmpty())
            {
                emit clicked(eventId_);
            }
        }
        else
        {
            // Single item selected - emit clicked to update details
            if (!eventId_.isEmpty())
            {
                emit clicked(eventId_);
            }
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
            isResizing_ = false;
            activeHandle_ = None;

            // Update model with new dates from resized rectangle
            if (rect_ != resizeStartRect_)
            {
                updateModelFromSize();
            }

            event->accept();
            return;
        }

        if (isMultiDragging_)
        {
            isMultiDragging_ = false;

            // Collect all event updates for batch command
            QList<QPair<QString, TimelineEvent>> eventUpdates;

            if (scene())
            {
                QList<QGraphicsItem*> selectedItems = scene()->selectedItems();

                for (QGraphicsItem* item : selectedItems)
                {
                    TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(item);

                    if (timelineItem && !timelineItem->eventId().isEmpty())
                    {
                        // Check if position actually changed
                        QPointF startPos = multiDragStartPositions_.value(item);

                        if (timelineItem->pos() != startPos)
                        {
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

                            // If lane control is enabled, update the lane
                            if (currentEvent->laneControlEnabled)
                            {
                                int newLane = timelineItem->calculateLaneFromYPosition();
                                updatedEvent.manualLane = newLane;
                                updatedEvent.lane = newLane;
                            }

                            // Add to batch update list
                            eventUpdates.append(qMakePair(timelineItem->eventId(), updatedEvent));
                        }
                    }
                }

                // Use undo commands for multi-drag
                if (!eventUpdates.isEmpty() && undoStack_)
                {
                    if (eventUpdates.size() == 1)
                    {
                        // Single event moved
                        undoStack_->push(new UpdateEventCommand(
                            model_,
                            eventUpdates.first().first,
                            eventUpdates.first().second
                            ));
                    }
                    else
                    {
                        // Multiple events moved - group into single undo step
                        undoStack_->beginMacro(QString("Move %1 Events").arg(eventUpdates.size()));

                        for (const auto& pair : eventUpdates)
                        {
                            undoStack_->push(new UpdateEventCommand(model_, pair.first, pair.second));
                        }

                        undoStack_->endMacro();
                    }
                }
                else if (!eventUpdates.isEmpty())
                {
                    // Fallback: update directly if no undo stack
                    qWarning() << "TimelineItem::mouseReleaseEvent() - No undo stack for multi-drag, updating directly";

                    bool wasBlocked = model_->blockSignals(true);
                    for (const auto& pair : eventUpdates)
                    {
                        model_->updateEvent(pair.first, pair.second);
                    }
                    model_->blockSignals(wasBlocked);

                    // Trigger update
                    if (!eventUpdates.isEmpty())
                    {
                        model_->updateEvent(eventUpdates.first().first, eventUpdates.first().second);
                    }
                }
            }

            multiDragStartPositions_.clear();
            event->accept();
            return;
        }

        if (isDragging_)
        {
            // Single drag completed - update model with new dates and lane
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

    // If lane control is enabled, update the lane based on Y position
    if (currentEvent->laneControlEnabled)
    {
        int newLane = calculateLaneFromYPosition();

        // Check for lane conflicts before updating
        bool hasConflict = model_->hasLaneConflict(
            newStartDate,
            newEndDate,
            newLane,
            eventId_ // Exclude self from conflict check
            );

        if (hasConflict)
        {
            // Show warning but allow the update (user can fix it later)
            QMessageBox::warning(
                nullptr,
                "Lane Conflict",
                QString("Warning: Another manually-controlled event already occupies lane %1 "
                        "during this time period.\n\n"
                        "This may cause visual overlap. Consider adjusting the lane assignment.")
                    .arg(newLane)
                );
        }

        updatedEvent.manualLane = newLane;
        updatedEvent.lane = newLane;
    }

    // Use undo command instead of direct model update
    if (undoStack_)
    {
        undoStack_->push(new UpdateEventCommand(model_, eventId_, updatedEvent));
    }
    else
    {
        // Fallback: update directly if no undo stack available
        qWarning() << "TimelineItem::updateModelFromPosition() - No undo stack available, updating directly";
        model_->updateEvent(eventId_, updatedEvent);
    }
}


int TimelineItem::calculateLaneFromYPosition() const
{
    // Calculate the Y position in scene coordinates
    double yPos = rect_.y() + pos().y();

    // Remove the date scale offset to get lane-relative Y position
    double laneY = yPos - DATE_SCALE_OFFSET;

    // Convert Y position to lane number using inverse of LaneAssigner::laneToY
    // Formula: lane = yPos / (laneHeight + laneSpacing)
    int lane = static_cast<int>(laneY / (ITEM_HEIGHT + LANE_SPACING));

    // Ensure lane is non-negative
    if (lane < 0)
    {
        lane = 0;
    }

    return lane;
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

    // Calculate new dates based on resized rectangle
    double leftX = rect_.x() + pos().x();
    double rightX = leftX + rect_.width();

    QDate newStartDate = mapper_->xToDate(leftX);
    QDate newEndDate = mapper_->xToDate(rightX).addDays(-1);    // Subtract 1 day because rightX represents start of next day

    // Ensure at least 1-day duration
    if (newEndDate < newStartDate)
    {
        newEndDate = newStartDate;
    }

    // Create updated event
    TimelineEvent updatedEvent = *currentEvent;
    updatedEvent.startDate = newStartDate;
    updatedEvent.endDate = newEndDate;

    // Use undo command instead of direct model update
    if (undoStack_)
    {
        undoStack_->push(new UpdateEventCommand(model_, eventId_, updatedEvent));
    }
    else
    {
        // Fallback: update directly if no undo stack available
        qWarning() << "TimelineItem::updateModelFromSize() - No undo stack available, updating directly";
        model_->updateEvent(eventId_, updatedEvent);
    }
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
