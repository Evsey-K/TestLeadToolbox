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

#include <QDebug>


// Constants for lane calculation (must match TimelineScene)
constexpr double ITEM_HEIGHT = 30.0;
constexpr double LANE_SPACING = 5.0;
constexpr double DATE_SCALE_OFFSET = 40.0;


TimelineItem::TimelineItem(const QRectF& rect, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , rect_(rect)
    , brush_(Qt::blue)
    , pen_(Qt::black, 1)
    , skipNextUpdate_(false)
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
    // Enable high-quality rendering for crisp visuals at all zoom levels
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

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

    // Draw event title with adaptive text rendering based on item width
    if (model_ && !eventId_.isEmpty())
    {
        const TimelineEvent* event = model_->getEvent(eventId_);
        if (event)
        {
            double itemWidth = rect_.width();

            // Only show text if item is wide enough (minimum 30 pixels)
            if (itemWidth > 30.0)
            {
                // Set up text rendering with white color for contrast
                painter->setPen(Qt::white);

                // Adjust font size based on item width for optimal readability
                QFont textFont = painter->font();

                if (itemWidth < 60.0)
                {
                    // Very narrow items: small font
                    textFont.setPointSize(8);
                }
                else if (itemWidth < 120.0)
                {
                    // Medium items: normal font
                    textFont.setPointSize(9);
                }
                else
                {
                    // Wide items: larger bold font
                    textFont.setPointSize(10);
                    textFont.setBold(true);
                }

                painter->setFont(textFont);

                // Calculate text rectangle with padding
                QRectF textRect = rect_.adjusted(5, 2, -5, -2);

                // Elide text if it doesn't fit within the available width
                QFontMetrics metrics(textFont);
                QString displayText = event->title;

                if (metrics.horizontalAdvance(displayText) > textRect.width())
                {
                    displayText = metrics.elidedText(displayText, Qt::ElideRight,
                                                     static_cast<int>(textRect.width()));
                }

                // Draw text centered vertically, aligned left
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, displayText);
            }
        }

        // Draw lane control icon in bottom-right corner if enabled
        if (event && event->laneControlEnabled)
        {
            double itemWidth = rect_.width();

            // Only show icon if item is wide enough (minimum 50 pixels)
            if (itemWidth > 50.0)
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

                // Draw icon background (semi-transparent white circle)
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor(255, 255, 255, 180));
                painter->drawEllipse(iconRect);

                // Draw lock icon symbol (simplified)
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
}


void TimelineItem::setRect(const QRectF& rect)
{
    qDebug() << "⚠️ setRect called - FROM" << rect_.left() << "-" << rect_.right()
        << "TO" << rect.left() << "-" << rect.right();
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

            if (rect_ != resizeStartRect_)
            {
                // Snap the rectangle visually first
                double rawLeftX = rect_.left();
                double rawRightX = rect_.right();

                double snappedLeftX = mapper_->snapXToNearestTick(rawLeftX);
                double snappedRightX = mapper_->snapXToNearestTick(rawRightX);

                double minWidth = 10.0;
                if (snappedRightX - snappedLeftX < minWidth)
                {
                    snappedRightX = snappedLeftX + minWidth;
                }

                QRectF snappedRect = rect_;
                snappedRect.setLeft(snappedLeftX);
                snappedRect.setRight(snappedRightX);

                setRect(snappedRect);

                // DON'T set skipNextUpdate here
                // Let updateModelFromSize handle it AFTER the undo command
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

    // Calculate edge positions (these are already in scene coordinates since pos() is 0)
    double leftX = rect_.left();
    double rightX = rect_.right();

    qDebug() << "=== UPDATE MODEL FROM SIZE ===";
    qDebug() << "Input rect coords:" << leftX << "to" << rightX;

    // Apply zoom-aware snapping to both edges
    double snappedLeftX = mapper_->snapXToNearestTick(leftX);
    double snappedRightX = mapper_->snapXToNearestTick(rightX);

    qDebug() << "Re-snapped coords:" << snappedLeftX << "to" << snappedRightX;
    qDebug() << "Pixels per day:" << mapper_->pixelsPerday();

    // Convert snapped X coordinates to dates
    QDate newStartDate;
    QDate newEndDate;

    double pixelsPerDay = mapper_->pixelsPerday();

    if (pixelsPerDay >= 192.0)
    {
        // Hour/half-hour precision - use DateTime to preserve snapped hour position
        QDateTime startDateTime = mapper_->xToDateTime(snappedLeftX);
        QDateTime endDateTime = mapper_->xToDateTime(snappedRightX);

        newStartDate = startDateTime.date();

        // If at midnight (00:00), the event ends on the PREVIOUS day
        // If at any other hour, the event ends on THAT day
        if (endDateTime.time() == QTime(0, 0, 0))
        {
            newEndDate = endDateTime.date().addDays(-1);
        }
        else
        {
            newEndDate = endDateTime.date();  // Don't subtract!
        }
    }
    else
    {
        // Day/week/month precision - use standard date conversion
        newStartDate = mapper_->xToDate(snappedLeftX);

        // Right edge represents start of next day, so subtract 1
        QDate rightEdgeDate = mapper_->xToDate(snappedRightX);
        newEndDate = rightEdgeDate.addDays(-1);
    }

    // Ensure at least 1-day minimum duration
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
        // Set skip flag BEFORE pushing (so it's active when redo() executes)
        skipNextUpdate_ = true;
        undoStack_->push(new UpdateEventCommand(model_, eventId_, updatedEvent));
        // The flag will be cleared by the scene after handling the signal from redo()
    }
    else
    {
        skipNextUpdate_ = true;
        qWarning() << "TimelineItem::updateModelFromSize() - No undo stack available, updating directly";
        model_->updateEvent(eventId_, updatedEvent);
    }

    qDebug() << "Final dates:" << newStartDate << "to" << newEndDate;
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


