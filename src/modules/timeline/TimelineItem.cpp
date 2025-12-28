// TimelineItem.cpp


#include "TimelineItem.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineCommands.h"
#include "LaneAssigner.h"
#include <QApplication>
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
#include <QMimeData>
#include <QMouseEvent>
#include <QUrl>
#include <QDebug>


// Constants for lane calculation (must match TimelineScene)
constexpr double ITEM_HEIGHT = 30.0;
constexpr double LANE_SPACING = 5.0;
constexpr double DATE_SCALE_OFFSET = 80.0;


TimelineItem::TimelineItem(const QRectF& rect, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , rect_(rect)
    , brush_(Qt::blue)
    , pen_(Qt::black, 1)
    , skipNextUpdate_(false)
{

    setFlag(QGraphicsItem::ItemIsMovable);                  ///< Enable the item to be movable by the user with mouse dragging
    setFlag(QGraphicsItem::ItemIsSelectable);               ///< Enable selection highlighting and interaction
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);       ///< Send position changes through itemChange
    setAcceptHoverEvents(true);                             ///< Enable hover events for resize cursor
    setAcceptDrops(true);                                   ///< Enable drag-and-drop for attachments
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

            // Calculate icon positioning based on what icons need to be shown
            // Icons are positioned right-to-left: [Lock Icon] [Attachment Icon] (right edge)
            const double LOCK_ICON_SIZE = 16.0;
            const double ATTACHMENT_ICON_SIZE = 16.0;
            const double ICON_MARGIN = 4.0;
            const double ICON_SPACING = 2.0;

            double currentRightEdge = rect_.right() - ICON_MARGIN;
            bool hasLockIcon = event->laneControlEnabled && itemWidth >= 50.0;
            bool hasAttachmentIcon = showAttachmentIndicator_ && attachmentCount_ > 0;

            // Draw lock icon for lane-controlled events (rightmost position if both icons present)
            if (hasLockIcon)
            {
                // Position icon at current right edge
                currentRightEdge -= LOCK_ICON_SIZE;
                QRectF iconRect(currentRightEdge, rect_.top() + 3, LOCK_ICON_SIZE, LOCK_ICON_SIZE);

                // Draw semi-transparent background circle
                painter->setBrush(QColor(255, 255, 255, 180));
                painter->drawEllipse(iconRect);

                // Draw lock icon symbol (simplified) - scaled up proportionally
                painter->setPen(QPen(QColor(76, 175, 80), 1.5));  // Green color
                painter->setBrush(Qt::NoBrush);

                // Lock body (rectangle) - scaled proportionally
                QRectF lockBody(
                    iconRect.center().x() - 3.5,
                    iconRect.center().y(),
                    7,
                    5
                    );
                painter->drawRect(lockBody);

                // Lock shackle (arc) - scaled proportionally
                QRectF shackleRect(
                    iconRect.center().x() - 2.5,
                    iconRect.center().y() - 4.5,
                    5,
                    5
                    );
                painter->drawArc(shackleRect, 0 * 16, 180 * 16);  // Top half circle

                // Move left for next icon (if any)
                if (hasAttachmentIcon)
                {
                    currentRightEdge -= ICON_SPACING;
                }
            }

            // Draw attachment indicator (to the left of lock icon if both present)
            if (hasAttachmentIcon)
            {
                drawAttachmentIndicator(painter, currentRightEdge - ATTACHMENT_ICON_SIZE);
            }
        }
    }

    // Draw drag-over overlay
    drawDragOverlay(painter);
}


void TimelineItem::drawLockIcon(QPainter* painter) {
    QRectF itemRect = rect();

    // Position in top-right corner
    QRectF lockIconRect(itemRect.width() - 20, 2, 16, 16);

    painter->save();
    painter->setPen(QPen(Qt::white, 2));
    painter->setBrush(QColor(60, 60, 60));

    // Draw padlock body
    QRectF body(lockIconRect.x() + 3, lockIconRect.y() + 8, 10, 7);
    painter->drawRect(body);

    // Draw padlock shackle
    QPainterPath shackle;
    shackle.arcMoveTo(lockIconRect.adjusted(4, 2, -4, 8), 0);
    shackle.arcTo(lockIconRect.adjusted(4, 2, -4, 8), 0, 180);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(shackle);

    painter->restore();
}


void TimelineItem::drawPinIcon(QPainter* painter) {
    QRectF itemRect = rect();

    // Position in top-left corner
    QRectF pinIconRect(2, 2, 16, 16);

    painter->save();
    painter->setPen(QPen(QColor(70, 130, 180), 2)); // Steel blue
    painter->setBrush(QColor(70, 130, 180));

    // Draw pin/thumbtack shape
    // Pin head (circle)
    QPointF pinHead(pinIconRect.center().x(), pinIconRect.y() + 5);
    painter->drawEllipse(pinHead, 3, 3);

    // Pin shaft (line going down)
    QLineF shaft(pinHead.x(), pinHead.y() + 3, pinHead.x(), pinIconRect.bottom() - 2);
    painter->drawLine(shaft);

    // Pin point (small triangle)
    QPolygonF point;
    point << QPointF(shaft.x2() - 2, shaft.y2() - 2)
          << QPointF(shaft.x2() + 2, shaft.y2() - 2)
          << QPointF(shaft.x2(), shaft.y2() + 2);
    painter->drawPolygon(point);

    painter->restore();
}


void TimelineItem::setRect(const QRectF& rect)
{
    qDebug() << "⚠️ setRect called - FROM" << rect_.left() << "-" << rect_.right()
        << "TO" << rect.left() << "-" << rect.right();
    prepareGeometryChange();
    rect_ = rect;
    update();
}


void TimelineItem::updateLockState() {
    updateItemFlags();
    updateVisualState();
}


void TimelineItem::updateItemFlags()
{
    if (!model_ || eventId_.isEmpty()) {
        return;
    }

    const TimelineEvent* event = model_->getEvent(eventId_);
    if (!event) {
        return;
    }

    QGraphicsItem::GraphicsItemFlags flags = QGraphicsItem::ItemSendsGeometryChanges;

    // Always allow selection (even for locked events)
    // This enables right-click context menu and toolbar edit button
    flags |= QGraphicsItem::ItemIsSelectable;

    // Only allow movement if event can be manipulated
    if (event->canManipulateInView()) {
        flags |= QGraphicsItem::ItemIsMovable;
    }

    // Enable hover events for cursor feedback
    setAcceptHoverEvents(true);

    setFlags(flags);
}


void TimelineItem::updateVisualState()
{
    update(); // Trigger repaint
}


QVariant TimelineItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene())
    {
        // Block position changes for fixed or locked items**
        if (model_ && !eventId_.isEmpty())
        {
            const TimelineEvent* event = model_->getEvent(eventId_);
            if (event && (event->isFixed || event->isLocked))
            {
                // Reject position change - return current position
                qDebug() << "TimelineItem: Blocked position change for event" << eventId_
                         << "(event is" << (event->isLocked ? "locked" : "fixed") << ")";
                return pos(); // Keep current position
            }
        }

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

                    // Skip fixed/locked items during multi-drag**
                    bool itemIsLocked = false;
                    bool itemIsFixed = false;
                    bool itemLaneControlEnabled = false;

                    if (timelineItem && model_ && !timelineItem->eventId().isEmpty())
                    {
                        const TimelineEvent* event = model_->getEvent(timelineItem->eventId());
                        if (event)
                        {
                            itemIsLocked = event->isLocked;
                            itemIsFixed = event->isFixed;
                            itemLaneControlEnabled = event->laneControlEnabled;
                        }
                    }

                    // Skip this item if it's locked or fixed
                    if (itemIsLocked || itemIsFixed)
                    {
                        qDebug() << "TimelineItem: Skipping"
                                 << (itemIsLocked ? "locked" : "fixed")
                                 << "item during multi-drag:" << timelineItem->eventId();
                        continue; // Don't move this item
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

    // Check lock state before allowing any interaction
    bool isLocked = false;
    bool isFixed = false;

    if (model_ && !eventId_.isEmpty())
    {
        const TimelineEvent* evt = model_->getEvent(eventId_);
        if (evt)
        {
            isLocked = evt->isLocked;
            isFixed = evt->isFixed;
        }
    }

    // If locked or fixed, prevent resize and drag but allow selection
    if (isLocked || isFixed)
    {
        qDebug() << "║ Event is " << (isLocked ? "LOCKED" : "FIXED")
                 << " - blocking resize and drag, but allowing selection";

        // Allow selection by handling click appropriately
        if (!isSelected())
        {
            // If not selected, select it
            if (!(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)))
            {
                // Normal click: clear other selections and select this one
                if (scene())
                {
                    scene()->clearSelection();
                }
            }
            setSelected(true);
        }

        // Emit clicked signal to update detail panel
        if (!eventId_.isEmpty())
        {
            emit clicked(eventId_);
        }

        qDebug() << "╚═══════════════════════════════════════════════════════════";
        event->accept();
        return;
    }

    // Check if clicking on a resize handle
    if (resizable_)
    {
        ResizeHandle handle = getResizeHandle(event->pos());

        if (handle != None)
        {
            isResizing_ = true;
            activeHandle_ = handle;

            originalZValue_ = zValue();
            setZValue(100.0);

            qDebug() << "║ Z-ORDER: Raised from" << originalZValue_ << "to 100.0 (resize)";
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

        qDebug() << "║ Selected items count:" << selectedItems.size();

        if (selectedItems.size() > 1)
        {
            // Multi-drag mode: record all selected item positions
            isMultiDragging_ = true;
            multiDragStartPositions_.clear();

            qDebug() << "║ MODE: MULTI-DRAG - Selected items count:" << selectedItems.size();

            for (QGraphicsItem* item : selectedItems)
            {
                multiDragStartPositions_[item] = item->pos();

                qDebug() << "║ Multi-drag: storing start position for item at" << item->pos();

                TimelineItem* tItem = qgraphicsitem_cast<TimelineItem*>(item);
                if (tItem)
                {
                    tItem->originalZValue_ = tItem->zValue();
                    tItem->setZValue(100.0);

                    qDebug() << "║   - Item:" << tItem->eventId()
                             << "at position:" << item->pos()
                             << "Z:" << tItem->originalZValue_ << "→ 100.0";
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

            originalZValue_ = zValue();
            setZValue(100.0);

            qDebug() << "║ Z-ORDER: Raised from" << originalZValue_ << "to 100.0 (single drag)";

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

        originalZValue_ = zValue();
        setZValue(100.0);

        qDebug() << "║ Z-ORDER: Raised from" << originalZValue_ << "to 100.0 (newly selected)";

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

            setZValue(originalZValue_);

            qDebug() << "║ Z-ORDER: Restored to" << originalZValue_;

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

            for (QGraphicsItem* item : multiDragStartPositions_.keys())
            {
                TimelineItem* tItem = qgraphicsitem_cast<TimelineItem*>(item);
                if (tItem)
                {
                    tItem->setZValue(tItem->originalZValue_);
                    qDebug() << "║ Z-ORDER: Restored item" << tItem->eventId()
                             << "to" << tItem->originalZValue_;
                }
            }

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
                            qDebug() << "║ Lane control enabled - calculating lane from Y position";
                            qDebug() << "║ Current event lane (before):" << currentEvent->lane;

                            int newLane = timelineItem->calculateLaneFromYPosition();

                            qDebug() << "║   New lane (after calculation):" << newLane;
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

            // **NEW: Check for lane conflicts BEFORE applying updates**
            QStringList conflictingEvents;
            for (const auto& pair : eventUpdates)
            {
                const TimelineEvent& updatedEvent = pair.second;

                // Only check conflicts for lane-controlled events
                if (updatedEvent.laneControlEnabled)
                {
                    // Check against OTHER lane-controlled events (excluding events in this batch)
                    bool hasConflict = false;

                    for (const auto& otherPair : eventUpdates)
                    {
                        // Skip if checking against itself
                        if (otherPair.first == pair.first)
                            continue;

                        const TimelineEvent& otherEvent = otherPair.second;

                        // Check if both are lane-controlled and in the same lane
                        if (otherEvent.laneControlEnabled &&
                            otherEvent.manualLane == updatedEvent.manualLane)
                        {
                            // Check for time overlap
                            if (updatedEvent.startDate <= otherEvent.endDate &&
                                updatedEvent.endDate >= otherEvent.startDate)
                            {
                                hasConflict = true;
                                break;
                            }
                        }
                    }

                    // Also check against events NOT in this batch
                    if (!hasConflict)
                    {
                        hasConflict = model_->hasLaneConflict(
                            updatedEvent.startDate,
                            updatedEvent.endDate,
                            updatedEvent.manualLane,
                            pair.first  // Exclude this event
                            );
                    }

                    if (hasConflict)
                    {
                        conflictingEvents.append(pair.first);
                    }
                }
            }

            // **NEW: If conflicts detected, revert ALL positions and abort**
            if (!conflictingEvents.isEmpty())
            {
                qDebug() << "║ ERROR: Lane conflicts detected for" << conflictingEvents.size() << "events";
                qDebug() << "║ Reverting all positions...";

                // Restore ALL items to their original positions
                for (auto it = multiDragStartPositions_.constBegin();
                     it != multiDragStartPositions_.constEnd(); ++it)
                {
                    TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(it.key());
                    if (timelineItem)
                    {
                        timelineItem->setPos(it.value());
                        qDebug() << "║   Restored" << timelineItem->eventId() << "to" << it.value();
                    }
                }

                // Show warning to user
                QString message;
                if (conflictingEvents.size() == 1)
                {
                    message = QString("Cannot place events here: One or more events would overlap with "
                                      "existing manually-controlled events in the same lane.\n\n"
                                      "All events have been returned to their original positions.");
                }
                else
                {
                    message = QString("Cannot place events here: %1 events would overlap with "
                                      "existing manually-controlled events in the same lanes.\n\n"
                                      "All events have been returned to their original positions.")
                                  .arg(conflictingEvents.size());
                }

                QMessageBox::warning(nullptr, "Lane Conflicts", message);

                multiDragStartPositions_.clear();
                qDebug() << "╚═══════════════════════════════════════════════════════════";
                event->accept();
                return;  // ABORT - don't update the model
            }

            // No conflicts - proceed with updates
            // Use undo commands for multi-drag
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

            setZValue(originalZValue_);

            qDebug() << "║ Z-ORDER: Restored to" << originalZValue_;

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

    // **NEW: Store original event state for potential rollback**
    TimelineEvent originalEvent = *currentEvent;

    // Calculate new start date based on CURRENT (already snapped) X position
    // The position has already been snapped in mouseReleaseEvent, so use it directly
    double xPos = rect_.x() + pos().x();

    qDebug() << "┃ Current state:";
    qDebug() << "┃   pos():" << pos();
    qDebug() << "┃   scenePos():" << scenePos();
    qDebug() << "┃   rect_:" << rect_;
    qDebug() << "┃   xPos (rect_.x + pos.x):" << xPos;
    qDebug() << "┃   Event type:" << currentEvent->type;

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

    // **NEW: Update type-specific date fields based on event type**
    switch (currentEvent->type)
    {
    case TimelineEventType_Meeting:
    case TimelineEventType_TestEvent:
    case TimelineEventType_Reminder:
        // These types use startDate and endDate directly
        updatedEvent.startDate = newStartDateTime;
        updatedEvent.endDate = newEndDateTime;
        qDebug() << "┃ Type uses startDate/endDate - both updated";
        break;

    case TimelineEventType_Action:
    case TimelineEventType_JiraTicket:
        // These types use startDate and dueDateTime
        updatedEvent.startDate = newStartDateTime;
        updatedEvent.dueDateTime = newEndDateTime;
        // Also update endDate for rendering purposes (it mirrors dueDateTime)
        updatedEvent.endDate = newEndDateTime;
        qDebug() << "┃ Type uses startDate/dueDateTime - both updated, endDate mirrored";
        break;

    default:
        // Fallback: update both startDate and endDate
        qWarning() << "┃ Unknown event type" << currentEvent->type << "- defaulting to startDate/endDate";
        updatedEvent.startDate = newStartDateTime;
        updatedEvent.endDate = newEndDateTime;
        break;
    }

    // Update Reminder-specific field if applicable
    if (currentEvent->type == TimelineEventType_Reminder)
    {
        updatedEvent.reminderDateTime = newStartDateTime;
        qDebug() << "┃ Reminder type - reminderDateTime also updated";
    }

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
            qDebug() << "┃   ERROR: Lane conflict detected - REVERTING POSITION";

            // Restore the item's visual position to where it started
            setPos(dragStartPos_);

            // **NEW: Also ensure model state is unchanged (in case it was modified)**
            // This is a safety measure - the model shouldn't have been updated yet,
            // but this ensures consistency
            if (originalEvent.startDate != currentEvent->startDate ||
                originalEvent.endDate != currentEvent->endDate ||
                originalEvent.lane != currentEvent->lane)
            {
                qDebug() << "┃   Restoring original model state as safety measure";
                model_->updateEvent(eventId_, originalEvent);
            }

            // Show warning to user
            QMessageBox::warning(
                nullptr,
                "Lane Conflict",
                QString("Cannot place event here: Another manually-controlled event already occupies lane %1 "
                        "during this time period.\n\n"
                        "The event has been returned to its original position.")
                    .arg(newLane)
                );

            qDebug() << "┃   Position restored to dragStartPos_:" << dragStartPos_;
            qDebug() << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
            return;  // ABORT - do not update the model
        }

        updatedEvent.manualLane = newLane;
        updatedEvent.lane = newLane;
    }

    qDebug() << "┃ Updating model...";
    qDebug() << "┃   Old: " << currentEvent->startDate << "to" << currentEvent->endDate << "lane" << currentEvent->lane;
    qDebug() << "┃   New: " << updatedEvent.startDate << "to" << updatedEvent.endDate << "lane" << updatedEvent.lane;
    if (currentEvent->type == TimelineEventType_Action || currentEvent->type == TimelineEventType_JiraTicket)
    {
        qDebug() << "┃   Due: " << updatedEvent.dueDateTime;
    }

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
    qDebug() << "";
    qDebug() << "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    qDebug() << "┃ CALCULATE LANE FROM Y POSITION - Event ID:" << eventId_;
    qDebug() << "┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";

    // Calculate the Y position in scene coordinates
    double rectY = rect_.y();
    double itemPosY = pos().y();
    double yPos = rectY + itemPosY;

    qDebug() << "┃ Y Position Components:";
    qDebug() << "┃   rect_.y():" << rectY;
    qDebug() << "┃   pos().y():" << itemPosY;
    qDebug() << "┃   Total yPos (rect_.y + pos.y):" << yPos;

    // Remove the date scale offset to get lane-relative Y position
    double laneY = yPos - DATE_SCALE_OFFSET;

    qDebug() << "┃ Lane Calculation:";
    qDebug() << "┃   DATE_SCALE_OFFSET:" << DATE_SCALE_OFFSET;
    qDebug() << "┃   laneY (yPos - offset):" << laneY;
    qDebug() << "┃   ITEM_HEIGHT:" << ITEM_HEIGHT;
    qDebug() << "┃   LANE_SPACING:" << LANE_SPACING;
    qDebug() << "┃   (ITEM_HEIGHT + LANE_SPACING):" << (ITEM_HEIGHT + LANE_SPACING);

    // Convert Y position to lane number using inverse of LaneAssigner::laneToY
    double exactLane = laneY / (ITEM_HEIGHT + LANE_SPACING);
    int truncatedLane = static_cast<int>(exactLane);
    int roundedLane = static_cast<int>(std::round(exactLane));

    qDebug() << "┃ Lane Number Calculation:";
    qDebug() << "┃   exactLane (laneY / (height+spacing)):" << exactLane;
    qDebug() << "┃   truncatedLane (old method):" << truncatedLane;
    qDebug() << "┃   roundedLane (new method):" << roundedLane;

    // Use rounded lane
    int lane = roundedLane;

    // Ensure lane is non-negative
    if (lane < 0)
    {
        qDebug() << "┃   Lane was negative, clamping to 0";
        lane = 0;
    }

    // Show what lane this Y position SHOULD correspond to visually
    double expectedYForLane = LaneAssigner::laneToY(lane, ITEM_HEIGHT, LANE_SPACING) + DATE_SCALE_OFFSET;
    qDebug() << "┃ Validation:";
    qDebug() << "┃   Calculated lane:" << lane;
    qDebug() << "┃   Expected Y for this lane:" << expectedYForLane;
    qDebug() << "┃   Actual Y:" << yPos;
    qDebug() << "┃   Difference:" << (yPos - expectedYForLane);

    qDebug() << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";

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
    qDebug() << "┃   Event type:" << currentEvent->type;

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

    // **NEW: Update type-specific date fields based on event type**
    switch (currentEvent->type)
    {
    case TimelineEventType_Meeting:
    case TimelineEventType_TestEvent:
    case TimelineEventType_Reminder:
        // These types use startDate and endDate directly
        updatedEvent.startDate = newStartDateTime;
        updatedEvent.endDate = newEndDateTime;
        qDebug() << "┃ Type uses startDate/endDate - both updated";
        break;

    case TimelineEventType_Action:
    case TimelineEventType_JiraTicket:
        // These types use startDate and dueDateTime
        updatedEvent.startDate = newStartDateTime;
        updatedEvent.dueDateTime = newEndDateTime;
        // Also update endDate for rendering purposes (it mirrors dueDateTime)
        updatedEvent.endDate = newEndDateTime;
        qDebug() << "┃ Type uses startDate/dueDateTime - both updated, endDate mirrored";
        break;

    default:
        // Fallback: update both startDate and endDate
        qWarning() << "┃ Unknown event type" << currentEvent->type << "- defaulting to startDate/endDate";
        updatedEvent.startDate = newStartDateTime;
        updatedEvent.endDate = newEndDateTime;
        break;
    }

    // Update Reminder-specific field if applicable
    if (currentEvent->type == TimelineEventType_Reminder)
    {
        updatedEvent.reminderDateTime = newStartDateTime;
        qDebug() << "┃ Reminder type - reminderDateTime also updated";
    }

    qDebug() << "┃ Updating model...";
    qDebug() << "┃   Old: " << currentEvent->startDate << "to" << currentEvent->endDate;
    qDebug() << "┃   New: " << updatedEvent.startDate << "to" << updatedEvent.endDate;
    if (currentEvent->type == TimelineEventType_Action || currentEvent->type == TimelineEventType_JiraTicket)
    {
        qDebug() << "┃   Due: " << updatedEvent.dueDateTime;
    }

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

    // Check lock state - don't change cursor for locked/fixed events
    bool isLocked = false;
    bool isFixed = false;
    if (model_ && !eventId_.isEmpty())
    {
        const TimelineEvent* evt = model_->getEvent(eventId_);
        if (evt)
        {
            isLocked = evt->isLocked;
            isFixed = evt->isFixed;
        }
    }

    // If locked or fixed, keep the ForbiddenCursor and don't show resize handles
    if (isLocked || isFixed)
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


void TimelineItem::hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent)
{
    Q_UNUSED(hoverEvent)
    isHovered_ = true;

    // Change cursor based on lock state
    if (model_ && !eventId_.isEmpty()) {
        const TimelineEvent* event = model_->getEvent(eventId_);
        if (event && (event->isLocked || event->isFixed)) {
            QApplication::setOverrideCursor(Qt::ForbiddenCursor);
        }
    }

    QGraphicsObject::hoverEnterEvent(hoverEvent);
}


void TimelineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent)
{
    Q_UNUSED(hoverEvent)
    isHovered_ = false;

    // Restore cursor
    if (model_ && !eventId_.isEmpty()) {
        const TimelineEvent* event = model_->getEvent(eventId_);
        if (event && (event->isLocked || event->isFixed)) {
            QApplication::restoreOverrideCursor();
        }
    }

    QGraphicsObject::hoverLeaveEvent(hoverEvent);
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


void TimelineItem::setAttachmentCount(int count)
{
    if (attachmentCount_ != count)
    {
        attachmentCount_ = count;
        showAttachmentIndicator_ = (count > 0);
        update();  // Trigger repaint

        qDebug() << "TimelineItem" << eventId_ << "attachment count set to:" << count;
    }
}


void TimelineItem::drawAttachmentIndicator(QPainter* painter, double iconX)
{
    if (!showAttachmentIndicator_ || attachmentCount_ <= 0)
    {
        return;
    }

    // Constants for attachment indicator
    const double ICON_SIZE = 16.0;
    const double BADGE_SIZE = 13.0;
    const double MARGIN = 4.0;

    // Position in top-right corner of the item
    double iconY = rect_.top() + MARGIN;

    // Save painter state
    painter->save();

    // Draw paperclip icon background (light circle)
    QRectF iconRect(iconX, iconY, ICON_SIZE, ICON_SIZE);
    painter->setBrush(QColor(255, 255, 255, 200));  // Semi-transparent white
    painter->setPen(QPen(QColor(100, 100, 100), 1));
    painter->drawEllipse(iconRect);

    // Draw document/file icon
    painter->setPen(QPen(QColor(60, 60, 60), 1.2));
    painter->setBrush(QColor(220, 220, 220));

    // Paperclip shape (simplified curved path)
    double centerX = iconX + ICON_SIZE / 2.0;
    double centerY = iconY + ICON_SIZE / 2.0;

    // Document body (small rectangle)
    QRectF docRect(centerX - 3.5, centerY - 3, 7, 7);

    // Create path for document with folded corner
    QPainterPath docPath;
    docPath.moveTo(docRect.left(), docRect.top());
    docPath.lineTo(docRect.right() - 2.5, docRect.top());  // Top edge (minus corner)
    docPath.lineTo(docRect.right(), docRect.top() + 2.5);  // Folded corner diagonal
    docPath.lineTo(docRect.right(), docRect.bottom());     // Right edge
    docPath.lineTo(docRect.left(), docRect.bottom());      // Bottom edge
    docPath.lineTo(docRect.left(), docRect.top());         // Left edge
    docPath.closeSubpath();

    painter->drawPath(docPath);

    // Draw fold line
    painter->setPen(QPen(QColor(60, 60, 60), 0.8));
    painter->drawLine(QPointF(docRect.right() - 2.5, docRect.top()),
                      QPointF(docRect.right(), docRect.top() + 2.5));

    // Draw horizontal lines to represent text
    painter->setPen(QPen(QColor(100, 100, 100), 0.6));
    painter->drawLine(QPointF(centerX - 2, centerY), QPointF(centerX + 2, centerY));
    painter->drawLine(QPointF(centerX - 2, centerY + 1.5), QPointF(centerX + 2, centerY + 1.5));

    // Draw attachment count badge if > 1
    if (attachmentCount_ > 1)
    {
        // Position badge at bottom-right of icon
        double badgeX = iconX + ICON_SIZE - BADGE_SIZE / 2.0;
        double badgeY = iconY + ICON_SIZE - BADGE_SIZE / 2.0;

        QRectF badgeRect(badgeX, badgeY, BADGE_SIZE, BADGE_SIZE);

        // Draw badge circle
        painter->setBrush(QColor(220, 50, 50));  // Red badge
        painter->setPen(QPen(Qt::white, 1));
        painter->drawEllipse(badgeRect);

        // Draw count text
        painter->setPen(Qt::white);
        QFont badgeFont = painter->font();
        badgeFont.setPixelSize(9);
        badgeFont.setBold(true);
        painter->setFont(badgeFont);

        QString countText = (attachmentCount_ > 9) ? "9+" : QString::number(attachmentCount_);
        painter->drawText(badgeRect, Qt::AlignCenter, countText);
    }

    // Restore painter state
    painter->restore();
}


void TimelineItem::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    // Check if the drag contains files
    if (event->mimeData()->hasUrls())
    {
        // Filter to only file URLs (not web URLs)
        QList<QUrl> urls = event->mimeData()->urls();
        bool hasFiles = false;

        for (const QUrl& url : urls)
        {
            if (url.isLocalFile())
            {
                hasFiles = true;
                break;
            }
        }

        if (hasFiles)
        {
            isDragHover_ = true;
            event->acceptProposedAction();
            update();  // Trigger repaint to show drag overlay

            qDebug() << "TimelineItem" << eventId_ << ": Drag enter with files";
            return;
        }
    }

    event->ignore();
}


void TimelineItem::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
{
    isDragHover_ = false;
    update();  // Trigger repaint to hide drag overlay

    qDebug() << "TimelineItem" << eventId_ << ": Drag leave";
    event->accept();
}


void TimelineItem::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    isDragHover_ = false;
    update();  // Clear drag overlay

    // DEBUG: Log event ID
    qDebug() << "TimelineItem::dropEvent() - eventId_:" << eventId_;
    qDebug() << "  isEmpty:" << eventId_.isEmpty();

    if (!event->mimeData()->hasUrls())
    {
        event->ignore();
        return;
    }

    // Extract file paths from dropped URLs
    QStringList filePaths;
    QList<QUrl> urls = event->mimeData()->urls();

    for (const QUrl& url : urls)
    {
        if (url.isLocalFile())
        {
            QString filePath = url.toLocalFile();
            filePaths.append(filePath);
            qDebug() << "  - File:" << filePath;
        }
    }

    if (!filePaths.isEmpty())
    {
        qDebug() << "TimelineItem" << eventId_ << ": Dropped" << filePaths.size() << "file(s)";

        // Check if eventId_ is valid before emitting
        if (eventId_.isEmpty())
        {
            qWarning() << "TimelineItem::dropEvent() - eventId_ is EMPTY! Cannot emit filesDropped signal.";
            QMessageBox::warning(nullptr, "Attachment Error",
                                 "Timeline item has no event ID. Cannot add attachments.");
            event->ignore();
            return;
        }

        // Emit signal with the dropped files
        emit filesDropped(eventId_, filePaths);

        event->acceptProposedAction();
    }
    else
    {
        qDebug() << "TimelineItem" << eventId_ << ": No valid files dropped";
        event->ignore();
    }
}


void TimelineItem::drawDragOverlay(QPainter* painter)
{
    if (!isDragHover_)
    {
        return;
    }

    // Save painter state
    painter->save();

    // Draw semi-transparent overlay
    QColor overlayColor(100, 150, 255, 80);  // Light blue with transparency
    painter->fillRect(rect_, overlayColor);

    // Draw dashed border
    QPen dashedPen(QColor(50, 100, 255), 2, Qt::DashLine);
    painter->setPen(dashedPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect_.adjusted(2, 2, -2, -2));

    // Draw hint text if item is wide enough
    if (rect_.width() > 120)
    {
        painter->setPen(QColor(0, 50, 150));
        QFont hintFont = painter->font();
        hintFont.setPixelSize(11);
        hintFont.setBold(true);
        painter->setFont(hintFont);

        QString hintText = "Drop files to attach";
        QRectF textRect = rect_.adjusted(10, 0, -10, 0);
        painter->drawText(textRect, Qt::AlignCenter, hintText);
    }

    // Restore painter state
    painter->restore();
}
