// TimelineView.cpp


#include "TimelineView.h"
#include "TimelineScene.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineItem.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QResizeEvent>
#include <QScrollBar>
#include <QGraphicsItem>
#include <QMenu>
#include <QKeyEvent>
#include <algorithm>


TimelineView::TimelineView(TimelineModel* model,
                           TimelineCoordinateMapper* mapper,
                           QWidget* parent)
    : QGraphicsView(parent)
    , model_(model)
    , mapper_(mapper)
    , isPanning_(false)
    , potentialClick_(false)
    , editAction_(nullptr)
    , deleteAction_(nullptr)
    , legendAction_(nullptr)
    , sidePanelAction_(nullptr)
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

    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    qDebug() << "🎯 TimelineView constructor - Focus policy set. Has focus:" << hasFocus();

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
    if (event->modifiers() & Qt::ControlModifier)
    {
        const double baseZoomFactor = 1.15;

        // --- NEW: use mouse position as zoom anchor ---
        const QPointF mouseViewportPos = event->position();
        const QPointF mouseScenePosBefore = mapToScene(mouseViewportPos.toPoint());

        // Preserve the datetime under the mouse
        const QDateTime anchorDateTime = mapper_->xToDateTime(mouseScenePosBefore.x());

        // Old zoom for min/max clamp detection
        const double oldZoom = mapper_->pixelsPerday();

        // --- NEW: apply zoom using pow() so fast wheel / multi-notch feels stable ---
        // Typical mouse wheel notch is 120. Trackpads may produce smaller deltas.
        const double steps = static_cast<double>(event->angleDelta().y()) / 120.0;
        if (qFuzzyIsNull(steps))
        {
            event->accept();
            return;
        }

        const double factor = std::pow(baseZoomFactor, steps);
        mapper_->zoom(factor);

        const double newZoom = mapper_->pixelsPerday();
        if (qFuzzyCompare(oldZoom, newZoom))
        {
            event->accept();
            return;
        }

        scene_->rebuildFromModel();

        // Anchor X after rebuild
        const double newSceneX = mapper_->dateTimeToX(anchorDateTime);

        // --- NEW: "soft centering" so outer zoom levels don’t jump around ---
        // At low zoom: keep anchor near the mouse (stable control).
        // As you zoom in: gradually pull it toward the viewport center.
        const double minPpd = mapper_->minPixelsPerDay();
        const double fullCenterPpd = 192.0; // same threshold your DateScale uses for hour ticks

        double t = 0.0;
        if (fullCenterPpd > minPpd)
        {
            t = (newZoom - minPpd) / (fullCenterPpd - minPpd);
            t = qBound(0.0, t, 1.0);
        }

        // Optional: give it a tiny baseline centering so it still "helps" even when zoomed out
        // (set to 0.0 if you want strictly "stay under mouse" at minimum zoom)
        const double minCenterPull = 0.0;
        t = minCenterPull + (1.0 - minCenterPull) * t;

        const double viewportCenterX = viewport()->width() / 2.0;
        const double desiredViewportX = mouseViewportPos.x() + (viewportCenterX - mouseViewportPos.x()) * t;

        // Keep Y stable (don’t jump vertically based on mouse Y)
        const QPointF viewportCenter(viewportCenterX, viewport()->height() / 2.0);
        const double currentCenterSceneY = mapToScene(viewportCenter.toPoint()).y();

        // In your setup, scene units are pixels (no QGraphicsView scaling),
        // so this offset is correct in scene coordinates.
        const double newCenterSceneX = newSceneX + (viewportCenterX - desiredViewportX);

        centerOn(newCenterSceneX, currentCenterSceneY);

        event->accept();
        return;
    }
    else if (event->modifiers() & Qt::ShiftModifier)
    {
        int delta = event->angleDelta().y();
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        event->accept();
        return;
    }
    else
    {
        int delta = event->angleDelta().y();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta);
        event->accept();
        return;
    }
}



bool TimelineView::event(QEvent* event)
{
    // Intercept Tab/Backtab BEFORE Qt's focus system handles them
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        qDebug() << "⚡ TimelineView::event() - Key:" << keyEvent->key();

        // Handle Tab/Shift+Tab ourselves
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
        {
            qDebug() << "⚡ Intercepting Tab - routing to keyPressEvent()";
            keyPressEvent(keyEvent);
            return true;  // Event consumed - don't let Qt handle it
        }
    }

    // Pass other events to base class
    return QGraphicsView::event(event);
}


void TimelineView::mousePressEvent(QMouseEvent* event)
{
    qDebug() << "🖱️ Mouse click - Setting focus. Current focus:" << hasFocus();
    setFocus();

    if (event->button() == Qt::RightButton)
    {
        // Store press position for right-click
        // We'll decide in mouseReleaseEvent whether this was a pan or context menu request
        isPanning_ = false;  // Don't start panning yet
        lastPanPoint_ = event->pos();
        mousePressPos_ = event->pos();  // Store for distance calculation
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

    // Check if this is a right-click drag (for panning)
    if (event->buttons() & Qt::RightButton)
    {
        // Calculate distance from initial press
        QPoint delta = event->pos() - mousePressPos_;
        int distance = delta.manhattanLength();

        // If moved more than 5 pixels, start panning
        if (distance > 5)
        {
            if (!isPanning_)
            {
                // Start panning mode
                isPanning_ = true;
                setCursor(Qt::ClosedHandCursor);
            }
        }

        if (isPanning_)
        {
            // Calculate the delta movement from last position
            QPoint moveDelta = event->pos() - lastPanPoint_;
            lastPanPoint_ = event->pos();

            // Pan the view by adjusting scroll bars
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - moveDelta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - moveDelta.y());

            event->accept();
            return;
        }
    }

    // For left-click, use default behavior (rubber band selection)
    QGraphicsView::mouseMoveEvent(event);
}


void TimelineView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        // Calculate distance from initial press
        QPoint releasePos = event->pos();
        int distance = (releasePos - mousePressPos_).manhattanLength();

        if (isPanning_)
        {
            // Was panning - end panning mode
            isPanning_ = false;
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }
        else if (distance < 5)
        {
            // Mouse didn't move much - treat as context menu request
            showContextMenu(event->pos());
            event->accept();
            return;
        }
        else
        {
            // Mouse moved but panning wasn't triggered (shouldn't happen, but handle gracefully)
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }
    }

    // Detect click vs drag for selection
    if (event->button() == Qt::LeftButton && potentialClick_)
    {
        QPoint releasePos = event->pos();
        int distance = (releasePos - mousePressPos_).manhattanLength();

        // If mouse moved very little, treat as a click
        if (distance < 3)
        {
            QPointF scenePos = mapToScene(event->pos());
            QGraphicsItem* clickedItem = scene_->itemAt(scenePos, transform());
            TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(clickedItem);

            if (!timelineItem && !(event->modifiers() & Qt::ControlModifier))
            {
                // Clicked on empty space without Ctrl → deselect all
                scene_->clearSelection();
            }

            // Ensure focus remains on view for keyboard navigation
            setFocus();
        }
    }

    potentialClick_ = false;
    QGraphicsView::mouseReleaseEvent(event);
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

    // Update version name label position to stay centered in new viewport
    if (scene_)
    {
        scene_->updateVersionNamePosition();
    }
}


void TimelineView::scrollContentsBy(int dx, int dy)
{
    // Call base implementation to perform actual scrolling
    QGraphicsView::scrollContentsBy(dx, dy);

    // Update version name label position to stay centered in viewport
    if (scene_)
    {
        scene_->updateVersionNamePosition();
    }
}


void TimelineView::keyPressEvent(QKeyEvent* event)
{
    // Handle Tab and Shift+Tab for event navigation
    if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab)
    {
        if (!scene_ || !model_)
        {
            event->ignore();
            return;
        }

        // Build sorted list of event IDs from the MODEL (stable source of truth)
        // This avoids caching any TimelineItem pointers
        QVector<TimelineEvent> allEvents = model_->getAllEvents();
        QList<QString> eventIds;

        for (const TimelineEvent& event : allEvents)
        {
            // Only include non-archived events
            if (!event.archived)
            {
                eventIds.append(event.id);
            }
        }

        if (eventIds.isEmpty())
        {
            event->ignore();
            return;
        }

        // Sort event IDs by their start date using the model
        std::sort(eventIds.begin(), eventIds.end(),
                  [this](const QString& idA, const QString& idB) {
                      const TimelineEvent* evtA = model_->getEvent(idA);
                      const TimelineEvent* evtB = model_->getEvent(idB);

                      if (!evtA || !evtB) return false;

                      return evtA->startDate < evtB->startDate;
                  });

        // Find the currently selected event ID (if any)
        QString currentSelectedId;
        int currentIndex = -1;

        QList<QGraphicsItem*> selectedItems = scene_->selectedItems();
        if (!selectedItems.isEmpty())
        {
            // Get the first selected TimelineItem
            for (QGraphicsItem* item : selectedItems)
            {
                TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(item);
                if (timelineItem)
                {
                    currentSelectedId = timelineItem->eventId();
                    break;
                }
            }
        }

        // Find the index of currently selected event in our sorted list
        if (!currentSelectedId.isEmpty())
        {
            currentIndex = eventIds.indexOf(currentSelectedId);
        }

        // Calculate next/previous index
        int nextIndex = -1;

        if (event->key() == Qt::Key_Tab && !(event->modifiers() & Qt::ShiftModifier))
        {
            // Tab: Move to next event
            if (currentIndex == -1)
                nextIndex = 0;
            else if (currentIndex < eventIds.size() - 1)
                nextIndex = currentIndex + 1;
            else
                nextIndex = 0;  // Wrap to first
        }
        else  // Backtab or Shift+Tab
        {
            // Shift+Tab: Move to previous event
            if (currentIndex == -1)
                nextIndex = eventIds.size() - 1;
            else if (currentIndex > 0)
                nextIndex = currentIndex - 1;
            else
                nextIndex = eventIds.size() - 1;  // Wrap to last
        }

        // Get the next event ID
        if (nextIndex >= 0 && nextIndex < eventIds.size())
        {
            QString nextEventId = eventIds[nextIndex];

            // Look up the actual TimelineItem by ID (right before use - never cached)
            TimelineItem* nextItem = scene_->findItemByEventId(nextEventId);

            if (!nextItem)
            {
                // Item doesn't exist in scene (shouldn't happen, but be safe)
                event->accept();
                return;
            }

            // Verify item is still valid and in our scene
            if (nextItem->scene() != scene_)
            {
                event->accept();
                return;
            }

            // Clear all selections
            scene_->clearSelection();

            // Re-verify item is still valid after clearing selection
            // (clearSelection could theoretically trigger side effects)
            nextItem = scene_->findItemByEventId(nextEventId);
            if (!nextItem || nextItem->scene() != scene_)
            {
                event->accept();
                return;
            }

            // Select the item
            nextItem->setSelected(true);

            // Scroll to make it visible (use sceneBoundingRect for safety)
            ensureVisible(nextItem->sceneBoundingRect(), 100, 50);

            // Emit signal to update detail panel
            emit scene_->itemClicked(nextEventId);
        }

        event->accept();
        return;
    }

    // Pass other key events to base class
    QGraphicsView::keyPressEvent(event);
}


double TimelineView::currentZoomLevel() const
{
    return mapper_->pixelsPerday();
}


void TimelineView::zoomToFitDateRange(const QDate& startDate, const QDate& endDate, bool animate)
{
    if (!startDate.isValid() || !endDate.isValid() || startDate > endDate)
    {
        qWarning() << "Invalid date range for zoom-to-fit:" << startDate << "to" << endDate;
        return;
    }

    // Calculate required pixels per day to fit the date range in viewport
    int viewportWidth = viewport()->width();
    if (viewportWidth <= 0)
    {
        qWarning() << "Invalid viewport width for zoom calculation";
        return;
    }

    // Calculate number of days in range (inclusive)
    int daysInRange = startDate.daysTo(endDate) + 1;

    // Calculate required pixels per day to exactly fit the range
    // Add small margin (5% on each side) for visual breathing room
    double requiredPixelsPerDay = (viewportWidth * 0.9) / static_cast<double>(daysInRange);

    // Clamp to valid zoom range
    requiredPixelsPerDay = qBound(
        mapper_->minPixelsPerDay(),
        requiredPixelsPerDay,
        TimelineCoordinateMapper::MAX_PIXELS_PER_DAY
        );

    // Store old zoom level
    double oldZoom = mapper_->pixelsPerday();

    // Apply new zoom level
    mapper_->setPixelsPerDay(requiredPixelsPerDay);

    // Rebuild scene with new zoom
    scene_->rebuildFromModel();

    // Calculate center point of the date range
    QDateTime rangeStart(startDate, QTime(0, 0, 0));
    QDateTime rangeEnd(endDate, QTime(23, 59, 59));
    qint64 centerMsecs = rangeStart.toMSecsSinceEpoch() +
                         (rangeEnd.toMSecsSinceEpoch() - rangeStart.toMSecsSinceEpoch()) / 2;
    QDateTime centerDateTime = QDateTime::fromMSecsSinceEpoch(centerMsecs);

    // Convert center datetime to scene X coordinate
    double centerX = mapper_->dateTimeToX(centerDateTime);

    // Center the view on the calculated position
    centerOn(centerX, sceneRect().center().y());

    qDebug() << "Zoom-to-fit applied:"
             << "Range:" << startDate << "to" << endDate
             << "| Days:" << daysInRange
             << "| Old zoom:" << oldZoom
             << "| New zoom:" << requiredPixelsPerDay
             << "| Center X:" << centerX;
}


void TimelineView::showContextMenu(const QPoint& pos)
{
    QMenu menu(this);

    // ========== EVENT OPERATIONS ==========
    QAction* addEventAction = menu.addAction("➕ Add Event");
    addEventAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A));
    connect(addEventAction, &QAction::triggered, this, &TimelineView::addEventRequested);

    editAction_ = menu.addAction("✏️ Edit");
    editAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    editAction_->setEnabled(hasSelection());
    connect(editAction_, &QAction::triggered, this, &TimelineView::editEventRequested);

    deleteAction_ = menu.addAction("🗑️ Delete");
    deleteAction_->setShortcut(QKeySequence::Delete);
    deleteAction_->setEnabled(hasSelection());
    connect(deleteAction_, &QAction::triggered, this, &TimelineView::deleteEventRequested);

    menu.addSeparator();

    // ========== ZOOM OPERATIONS ==========
    QAction* zoomInAction = menu.addAction("🔍 Zoom In");
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, &TimelineView::zoomInRequested);

    QAction* zoomOutAction = menu.addAction("🔍 Zoom Out");
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, &TimelineView::zoomOutRequested);

    QAction* resetZoomAction = menu.addAction("↺ Reset Zoom");
    resetZoomAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(resetZoomAction, &QAction::triggered, this, &TimelineView::resetZoomRequested);

    menu.addSeparator();

    // ========== NAVIGATION ==========
    QMenu* goToMenu = menu.addMenu("📅 Go to Date");

    QAction* goToTodayAction = goToMenu->addAction("📍 Today");
    goToTodayAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(goToTodayAction, &QAction::triggered, this, &TimelineView::goToTodayRequested);

    QAction* goToWeekAction = goToMenu->addAction("📆 Current Week");
    connect(goToWeekAction, &QAction::triggered, this, &TimelineView::goToCurrentWeekRequested);

    // Note: onGoToCurrentMonth doesn't exist in TimelineModule, so we'll skip it
    // If you want to add it, you'll need to implement it in TimelineModule first

    menu.addSeparator();

    // ========== VIEW TOGGLES ==========
    legendAction_ = menu.addAction("🎨 Legend");
    legendAction_->setCheckable(true);
    legendAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(legendAction_, &QAction::triggered, this, &TimelineView::legendToggleRequested);

    sidePanelAction_ = menu.addAction("◀ Hide Panel");
    sidePanelAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(sidePanelAction_, &QAction::triggered, this, &TimelineView::sidePanelToggleRequested);

    // Show menu at cursor position
    menu.exec(mapToGlobal(pos));
}


bool TimelineView::hasSelection() const
{
    return scene_ && !scene_->selectedItems().isEmpty();
}


void TimelineView::setLegendChecked(bool checked)
{
    if (legendAction_)
    {
        legendAction_->setChecked(checked);
    }
}


void TimelineView::setSidePanelVisible(bool visible)
{
    if (sidePanelAction_)
    {
        if (visible)
        {
            sidePanelAction_->setText("◀ Hide Panel");
            sidePanelAction_->setToolTip("Hide event list panel");
        }
        else
        {
            sidePanelAction_->setText("▶ Show Panel");
            sidePanelAction_->setToolTip("Show event list panel");
        }
    }
}
