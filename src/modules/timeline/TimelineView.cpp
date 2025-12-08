// TimelineView.cpp


#include "TimelineView.h"
#include "TimelineScene.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineItem.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QGraphicsItem>


TimelineView::TimelineView(TimelineModel* model,
                           TimelineCoordinateMapper* mapper,
                           QWidget* parent)
    : QGraphicsView(parent)
    , mapper_(mapper)
    , isPanning_(false)
    , potentialClick_(false)
{
    // Create the scene with model and mapper
    scene_ = new TimelineScene(model, mapper, this);
    setScene(scene_);

    // Enable anti-aliasing for smooth rendering
    setRenderHint(QPainter::Antialiasing, true);

    // Disable vertical scroll bar
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Enable horizontal scroll bar
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Enable rubber band selection for left-click drag
    setDragMode(QGraphicsView::RubberBandDrag);

    // Set background color
    setBackgroundBrush(QBrush(QColor(250, 250, 250)));
}


void TimelineView::wheelEvent(QWheelEvent* event)
{
    // Check if Ctrl key is pressed for zoom
    if (event->modifiers() & Qt::ControlModifier)
    {
        // Define zoom factor
        const double zoomFactor = 1.15;

        // Get the mouse position in widget coordinates
        QPointF widgetPos = event->position();

        // Convert to scene coordinates before zoom
        QPointF scenePosBefore = mapToScene(widgetPos.toPoint());

        // Determine zoom direction and apply zoom
        if (event->angleDelta().y() > 0)
        {
            // Zoom in
            mapper_->zoom(zoomFactor);
            scale(zoomFactor, 1.0);
        }
        else
        {
            // Zoom out
            mapper_->zoom(1.0 / zoomFactor);
            scale(1.0 / zoomFactor, 1.0);
        }

        // Rebuild scene with new scale
        scene_->rebuildFromModel();

        // Convert same widget position to scene coordinates after zoom
        QPointF scenePosAfter = mapToScene(widgetPos.toPoint());

        // Calculate the difference and adjust the scroll bars
        QPointF delta = scenePosAfter - scenePosBefore;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() + delta.y());

        event->accept();
    }
    else if (event->modifiers() & Qt::ShiftModifier)
    {
        // Shift + Mouse wheel: Vertical scrolling
        int delta = event->angleDelta().y();
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        event->accept();
    }
    else
    {
        // Default: Horizontal scrolling (no modifiers)
        int delta = event->angleDelta().y();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta);
        event->accept();
    }
}


void TimelineView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        // Start right-click panning
        isPanning_ = true;
        lastPanPoint_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    // Track mouse press position for click detection
    mousePressPos_ = event->pos();
    potentialClick_ = true;

    // Map to scene coordinates
    QPointF scenePos = mapToScene(event->pos());

    // Check if clicking on an item
    QGraphicsItem* clickedItem = scene_->itemAt(scenePos, transform());
    TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(clickedItem);

    // Handle Ctrl+click for multi-selection
    if (timelineItem && (event->modifiers() & Qt::ControlModifier))
    {
        // Toggle selection state immediately
        timelineItem->setSelected(!timelineItem->isSelected());

        // Accept event to prevent further processing
        event->accept();
        return;
    }

    // Handle Shift+click for range selection
    if (timelineItem && (event->modifiers() & Qt::ShiftModifier))
    {
        // Get all timeline items
        QList<QGraphicsItem*> allItems = scene_->items();

        // Find the last selected item
        TimelineItem* lastSelected = nullptr;
        for (QGraphicsItem* item : allItems)
        {
            TimelineItem* tItem = qgraphicsitem_cast<TimelineItem*>(item);
            if (tItem && tItem->isSelected() && tItem != timelineItem)
            {
                lastSelected = tItem;
                break; // Take the first one we find
            }
        }

        if (lastSelected)
        {
            // Select all items between lastSelected and timelineItem
            bool inRange = false;
            for (QGraphicsItem* item : allItems)
            {
                TimelineItem* tItem = qgraphicsitem_cast<TimelineItem*>(item);
                if (!tItem)
                    continue;

                // Check if this is one of our boundary items
                if (tItem == lastSelected || tItem == timelineItem)
                {
                    tItem->setSelected(true);
                    if (inRange)
                    {
                        // We've hit the second boundary, we're done
                        break;
                    }
                    else
                    {
                        // We've hit the first boundary, start selecting
                        inRange = true;
                    }
                }
                else if (inRange)
                {
                    // We're between the boundaries, select this item
                    tItem->setSelected(true);
                }
            }
        }
        else
        {
            // No previously selected item, just select this one
            timelineItem->setSelected(true);
        }

        event->accept();
        return;
    }

    // Handle normal click on item (not Ctrl or Shift)
    if (timelineItem && !(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)))
    {
        // If clicking on an unselected item, clear selection and select only this one
        if (!timelineItem->isSelected())
        {
            scene_->clearSelection();
            timelineItem->setSelected(true);
        }
        // If clicking on already selected item, keep selection (for dragging multiple items)

        // Now allow the event to propagate for dragging
        QGraphicsView::mousePressEvent(event);
        return;
    }

    // For clicks on empty space, use default behavior (rubber band selection)
    QGraphicsView::mousePressEvent(event);
}


void TimelineView::mouseMoveEvent(QMouseEvent* event)
{
    if (isPanning_)
    {
        // Calculate the delta movement
        QPoint delta = event->pos() - lastPanPoint_;
        lastPanPoint_ = event->pos();

        // Pan the view by adjusting scroll bars
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());

        event->accept();
        return;
    }

    // Check if mouse has moved significantly from press position
    if (potentialClick_)
    {
        QPoint delta = event->pos() - mousePressPos_;
        int distance = delta.manhattanLength();

        // If moved more than 3 pixels, it's a drag, not a click
        if (distance > 3)
        {
            potentialClick_ = false;
        }
    }

    // For left-click, use default behavior (rubber band selection)
    QGraphicsView::mouseMoveEvent(event);
}


void TimelineView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton && isPanning_)
    {
        // End right-click panning
        isPanning_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }

    // Reset click tracking
    potentialClick_ = false;

    // For left-click, use default behavior (rubber band selection)
    QGraphicsView::mouseReleaseEvent(event);
}
