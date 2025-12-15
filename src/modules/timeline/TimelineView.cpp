// TimelineView.cpp


#include "TimelineView.h"
#include "TimelineScene.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineItem.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QGraphicsItem>


TimelineView::TimelineView(TimelineModel* model,
                           TimelineCoordinateMapper* mapper,
                           QWidget* parent)
    : QGraphicsView(parent)
    , model_(model)
    , mapper_(mapper)
    , isPanning_(false)
    , potentialClick_(false)
{
    // Create the scene with model and mapper
    scene_ = new TimelineScene(model, mapper, this);
    setScene(scene_);

    setRenderHint(QPainter::Antialiasing, true);                ///< Enable anti-aliasing for smooth rendering
    setRenderHint(QPainter::TextAntialiasing, true);            ///< Enable text anti-aliasing for crisp text
    setRenderHint(QPainter::SmoothPixmapTransform, true);       ///< Enable smooth pixmap transforms
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);          ///< Enable vertical scroll bar as needed
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);        ///< Enable horizontal scroll bar
    setDragMode(QGraphicsView::RubberBandDrag);                 ///< Enable rubber band selection for left-click drag
    setBackgroundBrush(QBrush(QColor(250, 250, 250)));          ///< Set background color
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);  ///< Optimize view updates
    setCacheMode(QGraphicsView::CacheBackground);               ///< Set optimal cache mode

    // Initialize minimum zoom based on current viewport
    updateMinimumZoom();
}


void TimelineView::updateMinimumZoom()
{
    // Calculate the padded date range (1 month before version start, 1 month after version end)
    QDate paddedStart = model_->versionStartDate().addMonths(-1);
    QDate paddedEnd = model_->versionEndDate().addMonths(1);

    // Calculate total days in padded range
    int totalDays = paddedStart.daysTo(paddedEnd);

    if (totalDays <= 0)
    {
        // Fallback to absolute minimum if calculation fails
        mapper_->setMinPixelsPerDay(TimelineCoordinateMapper::ABSOLUTE_MIN_PIXELS_PER_DAY);
        return;
    }

    // Get viewport width
    int viewportWidth = viewport()->width();

    if (viewportWidth <= 0)
    {
        // Fallback if viewport not yet initialized
        mapper_->setMinPixelsPerDay(TimelineCoordinateMapper::ABSOLUTE_MIN_PIXELS_PER_DAY);
        return;
    }

    // Calculate minimum pixels per day so the entire padded range fits in viewport
    // This ensures user can see both padding boundaries at maximum zoom out
    double minPpd = static_cast<double>(viewportWidth) / static_cast<double>(totalDays);

    // Ensure we don't go below absolute minimum
    minPpd = qMax(minPpd, TimelineCoordinateMapper::ABSOLUTE_MIN_PIXELS_PER_DAY);

    // Set the dynamic minimum zoom level
    mapper_->setMinPixelsPerDay(minPpd);

    qDebug() << "Updated minimum zoom:"
             << "Viewport width =" << viewportWidth
             << "Total days (with padding) =" << totalDays
             << "Min PPD =" << minPpd;
}


void TimelineView::wheelEvent(QWheelEvent* event)
{
    // Check if Ctrl key is pressed for zoom
    if (event->modifiers() & Qt::ControlModifier)
    {
        // Define zoom factor
        const double zoomFactor = 1.15;

        // Get the center of the viewport in widget coordinates
        QPointF viewportCenter(viewport()->width() / 2.0, viewport()->height() / 2.0);

        // Map viewport center to scene coordinates BEFORE zoom
        QPointF scenePosBefore = mapToScene(viewportCenter.toPoint());

        // Convert scene position toDateTime (preserve the DATETIME at high zoom, not just date)
        QDateTime centerDateTime = mapper_->xToDateTime(scenePosBefore.x());

        // Store old zoom level for comparison
        double oldZoom = mapper_->pixelsPerday();

        // Determine zoom direction and apply zoom to mapper only
        if (event->angleDelta().y() > 0)
        {
            // Zoom in
            mapper_->zoom(zoomFactor);
        }
        else
        {
            // Zoom out
            mapper_->zoom(1.0 / zoomFactor);
        }

        // Check if zoom level actually changed (it won't if we hit min/max limits)
        double newZoom = mapper_->pixelsPerday();
        if (qFuzzyCompare(oldZoom, newZoom))
        {
            // Zoom didn't change - we're at a limit, don't rebuild
            event->accept();
            return;
        }

        // Rebuild scene with new scale (updates all item positions and sizes at native resolution)
        scene_->rebuildFromModel();

        // Convert the SAME datetime back to new scene coordinates (after zoom and rebuild)
        double newSceneX = mapper_->dateTimeToX(centerDateTime);

        // Use centerOn to directly center on the target point
        centerOn(newSceneX, scenePosBefore.y());

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

    // Only process left-click events beyond this point
    if (event->button() != Qt::LeftButton)
    {
        QGraphicsView::mousePressEvent(event);
        return;
    }

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

    // For left-click, use default behavior (rubber band selection)
    QGraphicsView::mouseReleaseEvent(event);

    // Reset click tracking
    potentialClick_ = false;
}


void TimelineView::resizeEvent(QResizeEvent* event)
{
    // Call base implementation
    QGraphicsView::resizeEvent(event);

    // Recalculate minimum zoom based on new viewport size
    updateMinimumZoom();

    // Optionally rebuild scene if current zoom is now below the new minimum
    // (The mapper will automatically clamp, but we need to refresh visuals)
    if (mapper_->pixelsPerday() <= mapper_->minPixelsPerDay())
    {
        scene_->rebuildFromModel();
    }
}


double TimelineView::currentZoomLevel() const
{
    return mapper_->pixelsPerday();
}



