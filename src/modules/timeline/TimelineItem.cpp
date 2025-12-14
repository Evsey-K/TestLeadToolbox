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
            if (itemWidth >= 30.0)
            {
                painter->setPen(Qt::white);

                // Adaptive font sizing based on item width
                QFont font = painter->font();
                if (itemWidth < 60.0)
                {
                    font.setPointSize(7);
                }
                else if (itemWidth < 120.0)
                {
                    font.setPointSize(9);
                }
                else
                {
                    font.setPointSize(10);
                }
                painter->setFont(font);

                // Elide text to fit within the rectangle
                QFontMetrics fm(font);
                QString elidedText = fm.elidedText(event->title, Qt::ElideRight,
                                                   static_cast<int>(itemWidth - 10));

                // Draw text centered in the rectangle
                painter->drawText(rect_.adjusted(5, 0, -5, 0), Qt::AlignVCenter | Qt::AlignLeft, elidedText);
            }

            // Draw lock icon for lane-controlled events (only if item is wide enough)
            if (event->laneControlEnabled && itemWidth >= 50.0)
            {
                // Position icon in top-right corner
                QRectF iconRect(rect_.right() - 18, rect_.top() + 3, 14, 14);

                // Draw semi-transparent background circle
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
        static int changeCounter = 0;
        changeCounter++;

        // Only log every 20th change to avoid spam
        if (changeCounter % 20 == 0)
        {
            qDebug() << "│  ItemPositionChange #" << changeCounter;
            qDebug() << "│    Current pos:" << pos();
            qDebug() << "│    Proposed pos:" << value.toPointF();
            qDebug() << "│    isMultiDragging_:" << isMultiDragging_;
            qDebug() << "│    isDragging_:" << isDragging_;
        }

        if (isMultiDragging_)
        {
            // Multi-drag: move all selected items by the same delta
            QPointF newPos = value.toPointF();
            QPointF delta = newPos - dragStartPos_;

            for (auto it = multiDragStartPositions_.constBegin();
                 it != multiDragStartPositions_.constEnd(); ++it)
            {
                QGraphicsItem* item = it.key();
                if (item != this) // Don't move self again
                {
                    TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(item);

                    // Check lane control for EACH item individually
                    bool itemLaneControlEnabled = false;
                    if (timelineItem && model_ && !timelineItem->eventId().isEmpty())
                    {
                        const TimelineEvent* event = model_->getEvent(timelineItem->eventId());
                        if (event)
                        {
                            itemLaneControlEnabled = event->laneControlEnabled;
                        }
                    }

                    QPointF newItemPos = it.value() + delta;

                    if (!itemLaneControlEnabled)
                    {
                        // If THIS item's lane control is disabled, preserve its Y (horizontal only)
                        newItemPos.setY(it.value().y());
                    }

                    // Set flag so the item knows it's being moved by multi-drag
                    if (timelineItem)
                    {
                        timelineItem->isBeingMultiDragged_ = true;
                    }

                    item->setPos(newItemPos);

                    // Clear flag after position is set
                    if (timelineItem)
                    {
                        timelineItem->isBeingMultiDragged_ = false;
                    }
                }
            }

            // Return corrected position for the dragging item (check ITS lane control)
            bool laneControlEnabled = false;
            if (model_ && !eventId_.isEmpty())
            {
                const TimelineEvent* event = model_->getEvent(eventId_);
                if (event)
                {
                    laneControlEnabled = event->laneControlEnabled;
                }
            }

            QPointF finalPos = dragStartPos_ + delta;

            if (!laneControlEnabled)
            {
                // Preserve Y for the dragging item if lane control disabled
                finalPos.setY(dragStartPos_.y());
            }

            return finalPos;
        }
        else if (isBeingMultiDragged_)
        {
            // This item is being moved by another item's multi-drag operation
            // Accept the position as-is without any additional constraints
            // (The constraints were already applied by the initiating item)
            return value.toPointF();
        }
        else if (isDragging_ && !isResizing_)
        {
            // Single-drag: constrain movement based on lane control
            QPointF newPos = value.toPointF();

            // Check lane control for this item
            bool laneControlEnabled = false;
            if (model_ && !eventId_.isEmpty())
            {
                const TimelineEvent* event = model_->getEvent(eventId_);
                if (event)
                {
                    laneControlEnabled = event->laneControlEnabled;
                }
            }

            if (!laneControlEnabled)
            {
                // If lane control is disabled, preserve Y (horizontal movement only)
                newPos.setY(dragStartPos_.y());
            }
            // If lane control is enabled, allow both X and Y movement

            // NO SNAPPING during drag - return the position as-is for smooth movement
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

    qDebug() << "╔═══════════════════════════════════════════════════════════";
    qDebug() << "║ MOUSE PRESS EVENT - Event ID:" << eventId_;
    qDebug() << "╠═══════════════════════════════════════════════════════════";
    qDebug() << "║ dragStartPos_:" << dragStartPos_;
    qDebug() << "║ scenePos():" << scenePos();
    qDebug() << "║ rect_:" << rect_;
    qDebug() << "║ isSelected():" << isSelected();

    // Check if clicking on a resize handle
    if (resizable_)
    {
        ResizeHandle handle = getResizeHandle(event->pos());

        if (handle != None)
        {
            isResizing_ = true;
            activeHandle_ = handle;

            qDebug() << "║ MODE: RESIZING - Handle:" << (handle == LeftEdge ? "LEFT" : "RIGHT");
            qDebug() << "╚═══════════════════════════════════════════════════════════";

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

            qDebug() << "║ MODE: MULTI-DRAG - Selected items count:" << selectedItems.size();

            for (QGraphicsItem* item : selectedItems)
            {
                multiDragStartPositions_[item] = item->pos();

                TimelineItem* tItem = qgraphicsitem_cast<TimelineItem*>(item);
                if (tItem)
                {
                    qDebug() << "║   - Item:" << tItem->eventId() << "at position:" << item->pos();
                }
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

            qDebug() << "║ MODE: SINGLE DRAG (already selected)";

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

        qDebug() << "║ MODE: SINGLE DRAG (newly selected)";

        if (!eventId_.isEmpty())
        {
            emit clicked(eventId_);
        }
        isDragging_ = true;
    }

    qDebug() << "║ isDragging_:" << isDragging_;
    qDebug() << "║ isMultiDragging_:" << isMultiDragging_;
    qDebug() << "╚═══════════════════════════════════════════════════════════";

    QGraphicsObject::mousePressEvent(event);
}


void TimelineItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    static int moveCounter = 0;
    moveCounter++;

    // Only log every 10th move event to avoid spam
    if (moveCounter % 10 == 0)
    {
        qDebug() << "┌─ MOUSE MOVE EVENT #" << moveCounter << "- Event ID:" << eventId_;
        qDebug() << "│  Current pos():" << pos();
        qDebug() << "│  scenePos():" << scenePos();
        qDebug() << "│  isDragging_:" << isDragging_;
        qDebug() << "│  isMultiDragging_:" << isMultiDragging_;
        qDebug() << "│  isResizing_:" << isResizing_;
        qDebug() << "└─";
    }

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
    qDebug() << "";
    qDebug() << "╔═══════════════════════════════════════════════════════════";
    qDebug() << "║ MOUSE RELEASE EVENT - Event ID:" << eventId_;
    qDebug() << "╠═══════════════════════════════════════════════════════════";
    qDebug() << "║ Current pos():" << pos();
    qDebug() << "║ scenePos():" << scenePos();
    qDebug() << "║ dragStartPos_:" << dragStartPos_;
    qDebug() << "║ isDragging_:" << isDragging_;
    qDebug() << "║ isMultiDragging_:" << isMultiDragging_;
    qDebug() << "║ isResizing_:" << isResizing_;
    qDebug() << "║ Position changed:" << (pos() != dragStartPos_);

    if (event->button() == Qt::LeftButton)
    {
        if (isResizing_)
        {
            qDebug() << "║ RESIZE COMPLETE";
            qDebug() << "║ resizeStartRect_:" << resizeStartRect_;
            qDebug() << "║ rect_:" << rect_;
            qDebug() << "║ Size changed:" << (rect_ != resizeStartRect_);

            isResizing_ = false;
            activeHandle_ = None;

            if (rect_ != resizeStartRect_)
            {
                // Snap the rectangle visually first
                double rawLeftX = rect_.left();
                double rawRightX = rect_.right();

                qDebug() << "║ PRE-SNAP (rect local):";
                qDebug() << "║   rawLeftX:" << rawLeftX;
                qDebug() << "║   rawRightX:" << rawRightX;

                double snappedLeftX = mapper_->snapXToNearestTick(rawLeftX);
                double snappedRightX = mapper_->snapXToNearestTick(rawRightX);

                qDebug() << "║ POST-SNAP:";
                qDebug() << "║   snappedLeftX:" << snappedLeftX;
                qDebug() << "║   snappedRightX:" << snappedRightX;
                qDebug() << "║   leftX delta:" << (snappedLeftX - rawLeftX);
                qDebug() << "║   rightX delta:" << (snappedRightX - rawRightX);

                double minWidth = 10.0;
                if (snappedRightX - snappedLeftX < minWidth)
                {
                    snappedRightX = snappedLeftX + minWidth;
                    qDebug() << "║ Adjusted right edge for minimum width:" << snappedRightX;
                }

                QRectF snappedRect = rect_;
                snappedRect.setLeft(snappedLeftX);
                snappedRect.setRight(snappedRightX);

                setRect(snappedRect);

                qDebug() << "║ FINAL snapped rect_:" << rect_;
                qDebug() << "║ Calling updateModelFromSize()...";

                // DON'T set skipNextUpdate here
                // Let updateModelFromSize handle it AFTER the undo command
                updateModelFromSize();
            }
            else
            {
                qDebug() << "║ No size change detected, skipping model update";
            }

            qDebug() << "╚═══════════════════════════════════════════════════════════";
            event->accept();
            return;
        }

        if (isMultiDragging_)
        {
            qDebug() << "║ MULTI-DRAG COMPLETE";
            isMultiDragging_ = false;

            // Collect all event updates for batch command
            QList<QPair<QString, TimelineEvent>> eventUpdates;

            qDebug() << "║ Processing" << multiDragStartPositions_.size() << "items...";

            // First, snap all items and collect updates
            for (auto it = multiDragStartPositions_.constBegin();
                 it != multiDragStartPositions_.constEnd(); ++it)
            {
                TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(it.key());

                if (timelineItem && model_ && mapper_)
                {
                    qDebug() << "║ Item:" << timelineItem->eventId();
                    qDebug() << "║   Start pos:" << it.value();
                    qDebug() << "║   Current pos:" << timelineItem->pos();
                    qDebug() << "║   Position changed:" << (timelineItem->pos() != it.value());

                    const TimelineEvent* currentEvent = model_->getEvent(timelineItem->eventId());

                    if (currentEvent)
                    {
                        // Calculate the snapped X position
                        double currentXPos = timelineItem->rect().x() + timelineItem->pos().x();

                        qDebug() << "║   PRE-SNAP currentXPos:" << currentXPos;

                        double snappedXPos = mapper_->snapXToNearestTick(currentXPos);

                        qDebug() << "║   POST-SNAP snappedXPos:" << snappedXPos;
                        qDebug() << "║   Snap delta:" << (snappedXPos - currentXPos);

                        // Snap the visual position
                        QPointF snappedPos = timelineItem->pos();
                        snappedPos.setX(snappedXPos - timelineItem->rect().x());
                        timelineItem->setPos(snappedPos);

                        qDebug() << "║   Final snapped position:" << timelineItem->pos();

                        // Calculate new dates
                        QDateTime newStartDateTime;
                        QDateTime newEndDateTime;

                        double pixelsPerDay = mapper_->pixelsPerday();

                        if (pixelsPerDay >= 192.0)
                        {
                            // Hour/half-hour precision - use DateTime
                            newStartDateTime = mapper_->xToDateTime(snappedXPos);

                            // Calculate duration to preserve event length
                            qint64 durationSecs = currentEvent->startDate.secsTo(currentEvent->endDate);
                            newEndDateTime = newStartDateTime.addSecs(durationSecs);
                        }
                        else
                        {
                            // Day/week/month precision
                            QDate newStartDate = mapper_->xToDate(snappedXPos);

                            // Calculate duration to preserve event length
                            int duration = currentEvent->startDate.daysTo(currentEvent->endDate);
                            QDate newEndDate = newStartDate.addDays(duration);

                            // Preserve time components
                            newStartDateTime = QDateTime(newStartDate, currentEvent->startDate.time());
                            newEndDateTime = QDateTime(newEndDate, currentEvent->endDate.time());
                        }

                        qDebug() << "║   New start:" << newStartDateTime;
                        qDebug() << "║   New end:" << newEndDateTime;

                        // Create updated event
                        TimelineEvent updatedEvent = *currentEvent;
                        updatedEvent.startDate = newStartDateTime;
                        updatedEvent.endDate = newEndDateTime;

                        // If lane control is enabled, update the lane
                        if (currentEvent->laneControlEnabled)
                        {
                            int newLane = timelineItem->calculateLaneFromYPosition();
                            qDebug() << "║   New lane:" << newLane;
                            updatedEvent.manualLane = newLane;
                            updatedEvent.lane = newLane;
                        }

                        // Add to batch update list
                        eventUpdates.append(qMakePair(timelineItem->eventId(), updatedEvent));
                    }
                    else
                    {
                        qDebug() << "║   WARNING: Event not found in model!";
                    }
                }
            }

            qDebug() << "║ Total event updates:" << eventUpdates.size();

            // Use undo commands for multi-drag
            // The model's assignLanesToEvents() will handle collision detection
            if (!eventUpdates.isEmpty() && undoStack_)
            {
                qDebug() << "║ Pushing batch update command to undo stack...";

                if (eventUpdates.size() == 1)
                {
                    // Single event moved
                    skipNextUpdate_ = true;
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
                        qDebug() << "║   Updating event:" << pair.first;

                        // Set skipNextUpdate for the corresponding item
                        // Find the item by matching event ID
                        for (auto it = multiDragStartPositions_.constBegin();
                             it != multiDragStartPositions_.constEnd(); ++it)
                        {
                            TimelineItem* item = qgraphicsitem_cast<TimelineItem*>(it.key());
                            if (item && item->eventId() == pair.first)
                            {
                                item->setSkipNextUpdate(true);
                                break;
                            }
                        }

                        undoStack_->push(new UpdateEventCommand(
                            model_,
                            pair.first,
                            pair.second
                            ));
                    }

                    undoStack_->endMacro();
                    qDebug() << "║ Batch update complete";
                }
            }

            multiDragStartPositions_.clear();
            qDebug() << "╚═══════════════════════════════════════════════════════════";
            event->accept();
            return;
        }

        if (isDragging_)
        {
            qDebug() << "║ SINGLE-DRAG COMPLETE";
            isDragging_ = false;

            // Snap to nearest tick mark after drag
            if (pos() != dragStartPos_)
            {
                qDebug() << "║ Position changed, applying snap...";

                // Calculate the snapped X position based on VISUAL left edge
                double currentXPos = rect_.x() + pos().x();

                qDebug() << "║ PRE-SNAP:";
                qDebug() << "║   rect_.x():" << rect_.x();
                qDebug() << "║   pos().x():" << pos().x();
                qDebug() << "║   currentXPos (rect_.x + pos.x):" << currentXPos;

                double snappedXPos = mapper_->snapXToNearestTick(currentXPos);

                qDebug() << "║ POST-SNAP:";
                qDebug() << "║   snappedXPos:" << snappedXPos;
                qDebug() << "║   Snap delta:" << (snappedXPos - currentXPos);

                // Update position to snap to grid (visually)
                QPointF snappedPos = pos();
                snappedPos.setX(snappedXPos - rect_.x());

                qDebug() << "║ Setting snapped position:" << snappedPos;
                setPos(snappedPos);

                qDebug() << "║ Calling updateModelFromPosition()...";

                // Now update the model with the snapped position
                // The model's assignLanesToEvents() will handle collision detection
                updateModelFromPosition();
            }
            else
            {
                qDebug() << "║ No position change detected, skipping model update";
            }

            qDebug() << "╚═══════════════════════════════════════════════════════════";
            event->accept();
            return;
        }
    }

    qDebug() << "║ No drag or resize detected, passing event through";
    qDebug() << "╚═══════════════════════════════════════════════════════════";

    QGraphicsObject::mouseReleaseEvent(event);
}


void TimelineItem::updateModelFromPosition()
{
    qDebug() << "";
    qDebug() << "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    qDebug() << "┃ UPDATE MODEL FROM POSITION - Event ID:" << eventId_;
    qDebug() << "┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";

    if (!model_ || !mapper_ || eventId_.isEmpty())
    {
        qDebug() << "┃ ERROR: Missing dependencies";
        qDebug() << "┃   model_:" << (model_ != nullptr);
        qDebug() << "┃   mapper_:" << (mapper_ != nullptr);
        qDebug() << "┃   eventId_:" << eventId_;
        qDebug() << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        return;
    }

    // Get current event data
    const TimelineEvent* currentEvent = model_->getEvent(eventId_);

    if (!currentEvent)
    {
        qDebug() << "┃ ERROR: Event not found in model";
        qDebug() << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        return;
    }

    // Calculate new start date based on CURRENT (already snapped) X position
    // The position has already been snapped in mouseReleaseEvent, so use it directly
    double xPos = rect_.x() + pos().x();

    qDebug() << "┃ Current state:";
    qDebug() << "┃   pos():" << pos();
    qDebug() << "┃   scenePos():" << scenePos();
    qDebug() << "┃   rect_:" << rect_;
    qDebug() << "┃   xPos (rect_.x + pos.x):" << xPos;

    QDateTime newStartDateTime;
    QDateTime newEndDateTime;

    double pixelsPerDay = mapper_->pixelsPerday();

    qDebug() << "┃ Date calculations:";
    qDebug() << "┃   pixelsPerDay:" << pixelsPerDay;

    if (pixelsPerDay >= 192.0)
    {
        // Hour/half-hour precision - use DateTime
        newStartDateTime = mapper_->xToDateTime(xPos);

        // Calculate duration to preserve event length
        qint64 durationSecs = currentEvent->startDate.secsTo(currentEvent->endDate);
        newEndDateTime = newStartDateTime.addSecs(durationSecs);
    }
    else
    {
        // Day/week/month precision
        QDate newStartDate = mapper_->xToDate(xPos);

        // Calculate duration to preserve event length
        int duration = currentEvent->startDate.daysTo(currentEvent->endDate);
        QDate newEndDate = newStartDate.addDays(duration);

        // Preserve time components
        newStartDateTime = QDateTime(newStartDate, currentEvent->startDate.time());
        newEndDateTime = QDateTime(newEndDate, currentEvent->endDate.time());
    }

    qDebug() << "┃   newStartDateTime:" << newStartDateTime;
    qDebug() << "┃   newEndDateTime:" << newEndDateTime;
    qDebug() << "┃   Duration (days):" << newStartDateTime.daysTo(newEndDateTime);

    // Create updated event
    TimelineEvent updatedEvent = *currentEvent;
    updatedEvent.startDate = newStartDateTime;
    updatedEvent.endDate = newEndDateTime;

    // If lane control is enabled, update the lane based on Y position
    if (currentEvent->laneControlEnabled)
    {
        int newLane = calculateLaneFromYPosition();

        qDebug() << "┃   newLane:" << newLane;

        // Check for lane conflicts before updating
        bool hasConflict = model_->hasLaneConflict(
            newStartDateTime,
            newEndDateTime,
            newLane,
            eventId_
            );

        if (hasConflict)
        {
            qDebug() << "┃   WARNING: Lane conflict detected!";

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

    qDebug() << "┃ Updating model...";
    qDebug() << "┃   Old: " << currentEvent->startDate << "to" << currentEvent->endDate << "lane" << currentEvent->lane;
    qDebug() << "┃   New: " << updatedEvent.startDate << "to" << updatedEvent.endDate << "lane" << updatedEvent.lane;

    // Use undo command instead of direct model update
    if (undoStack_)
    {
        qDebug() << "┃ Using undo stack...";
        skipNextUpdate_ = true;
        undoStack_->push(new UpdateEventCommand(model_, eventId_, updatedEvent));
    }
    else
    {
        // Fallback: update directly if no undo stack available
        qDebug() << "┃ Direct model update (no undo stack)";
        qWarning() << "TimelineItem::updateModelFromPosition() - No undo stack available, updating directly";
        model_->updateEvent(eventId_, updatedEvent);
    }

    qDebug() << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
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
    qDebug() << "";
    qDebug() << "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    qDebug() << "┃ UPDATE MODEL FROM SIZE - Event ID:" << eventId_;
    qDebug() << "┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";

    if (!model_ || !mapper_ || eventId_.isEmpty())
    {
        qDebug() << "┃ ERROR: Missing dependencies";
        qDebug() << "┃   model_:" << (model_ != nullptr);
        qDebug() << "┃   mapper_:" << (mapper_ != nullptr);
        qDebug() << "┃   eventId_:" << eventId_;
        qDebug() << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        return;
    }

    const TimelineEvent* currentEvent = model_->getEvent(eventId_);

    if (!currentEvent)
    {
        qDebug() << "┃ ERROR: Event not found in model";
        qDebug() << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        return;
    }

    double leftX = rect_.left();
    double rightX = rect_.right();

    qDebug() << "┃ Current state:";
    qDebug() << "┃   pos():" << pos();
    qDebug() << "┃   scenePos():" << scenePos();
    qDebug() << "┃   rect_:" << rect_;

    qDebug() << "┃ Edge positions (already snapped in mouseReleaseEvent):";
    qDebug() << "┃   leftX:" << leftX;
    qDebug() << "┃   rightX:" << rightX;
    qDebug() << "┃ Original dates:" << currentEvent->startDate << "to" << currentEvent->endDate;

    // Calculate new dates based on edge positions
    QDateTime newStartDateTime;
    QDateTime newEndDateTime;

    double pixelsPerDay = mapper_->pixelsPerday();

    qDebug() << "┃ Date calculations:";
    qDebug() << "┃   pixelsPerDay:" << pixelsPerDay;

    if (pixelsPerDay >= 192.0)
    {
        // Hour/half-hour precision - use DateTime directly
        newStartDateTime = mapper_->xToDateTime(leftX);
        newEndDateTime = mapper_->xToDateTime(rightX);
    }
    else
    {
        // Day/week/month precision
        QDate newStartDate = mapper_->xToDate(leftX);
        QDate newEndDate = mapper_->xToDate(rightX);

        // For day-level events, the right edge represents the pixel AFTER the last day
        // So subtract 1 to get the actual last day included
        newEndDate = newEndDate.addDays(-1);

        // Preserve time components from original event
        newStartDateTime = QDateTime(newStartDate, currentEvent->startDate.time());
        newEndDateTime = QDateTime(newEndDate, currentEvent->endDate.time());
    }

    qDebug() << "┃   newStartDateTime:" << newStartDateTime;
    qDebug() << "┃   newEndDateTime:" << newEndDateTime;
    qDebug() << "┃   Duration (days):" << newStartDateTime.daysTo(newEndDateTime);

    // Check if dates actually changed
    bool datesChanged = (newStartDateTime != currentEvent->startDate ||
                         newEndDateTime != currentEvent->endDate);

    qDebug() << "┃ Changes detected:";
    qDebug() << "┃   datesChanged:" << datesChanged;

    if (!datesChanged)
    {
        qDebug() << "┃ No changes detected, skipping model update";
        qDebug() << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        return;
    }

    // Create updated event
    TimelineEvent updatedEvent = *currentEvent;
    updatedEvent.startDate = newStartDateTime;
    updatedEvent.endDate = newEndDateTime;

    qDebug() << "┃ Updating model...";
    qDebug() << "┃   Old: " << currentEvent->startDate << "to" << currentEvent->endDate;
    qDebug() << "┃   New: " << updatedEvent.startDate << "to" << updatedEvent.endDate;

    // Use undo command instead of direct model update
    if (undoStack_)
    {
        qDebug() << "┃ Using undo stack...";
        skipNextUpdate_ = true;
        undoStack_->push(new UpdateEventCommand(model_, eventId_, updatedEvent));
    }
    else
    {
        // Fallback: update directly if no undo stack available
        qDebug() << "┃ Direct model update (no undo stack)";
        qWarning() << "TimelineItem::updateModelFromSize() - No undo stack available, updating directly";
        model_->updateEvent(eventId_, updatedEvent);
    }

    qDebug() << "Final dates:" << updatedEvent.startDate << "to" << updatedEvent.endDate;
    qDebug() << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
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
