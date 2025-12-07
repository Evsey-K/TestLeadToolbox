// TimelineScene.cpp


#include "TimelineScene.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineItem.h"
#include "LaneAssigner.h"
#include "TimelineDateScale.h"
#include "CurrentDateMarker.h"
#include "VersionBoundaryMarker.h"
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QKeyEvent>


TimelineScene::TimelineScene(TimelineModel* model, TimelineCoordinateMapper* mapper, QObject* parent)
    : QGraphicsScene(parent)
    , model_(model)
    , mapper_(mapper)
    , dateScale_(nullptr)
    , currentDateMarker_(nullptr)
    , versionStartMarker_(nullptr)
    , versionEndMarker_(nullptr)
{
    // Connect to model signals
    connect(model_, &TimelineModel::eventAdded, this, &TimelineScene::onEventAdded);
    connect(model_, &TimelineModel::eventRemoved, this, &TimelineScene::onEventRemoved);
    connect(model_, &TimelineModel::eventUpdated, this, &TimelineScene::onEventUpdated);
    connect(model_, &TimelineModel::versionDatesChanged, this, &TimelineScene::onVersionDatesChanged);
    connect(model_, &TimelineModel::lanesRecalculated, this, &TimelineScene::onLanesRecalculated);          // Connect to lane recalculation signal
    connect(model_, &TimelineModel::eventArchived, this, &TimelineScene::onEventRemoved);                   // Handle archiving (soft delete) - should remove from view like hard delete
    connect(model_, &TimelineModel::eventRestored, this, &TimelineScene::onEventAdded);                     // Handle restoration - should add back to view

    // Setup date scale and current date marker
    setupDateScale();

    // Setup version boundary markers (Feature 3a)
    setupVersionBoundaryMarkers();

    // Build initial scene from existing model data
    rebuildFromModel();
}


void TimelineScene::setupDateScale()
{
    // Create date scale
    dateScale_ = new TimelineDateScale(mapper_);
    addItem(dateScale_);

    // Create current date marker
    currentDateMarker_ = new CurrentDateMarker(mapper_);
    addItem(currentDateMarker_);
}


void TimelineScene::setupVersionBoundaryMarkers()
{
    // Create version start marker
    versionStartMarker_ = new VersionBoundaryMarker(mapper_, VersionBoundaryMarker::VersionStart);
    addItem(versionStartMarker_);

    // Create version end marker
    versionEndMarker_ = new VersionBoundaryMarker(mapper_, VersionBoundaryMarker::VersionEnd);
    addItem(versionEndMarker_);
}


void TimelineScene::rebuildFromModel()
{
    // Clear all existing event items (but keep date scale and markers)
    for (auto* item : eventIdToItem_.values())
    {
        removeItem(item);
        delete item;
    }
    eventIdToItem_.clear();

    // Create items for all events in the model
    const auto& events = model_->getAllEvents();

    for (const auto& event : events)
    {
        createItemForEvent(event.id);
    }

    // Feature 3b: Calculate scene rect with +1 month padding before/after version dates
    QDate paddedStart = model_->versionStartDate().addMonths(-1);
    QDate paddedEnd = model_->versionEndDate().addMonths(1);

    double startX = mapper_->dateToX(paddedStart);
    double endX = mapper_->dateToX(paddedEnd);
    double width = endX - startX;

    double height = DATE_SCALE_OFFSET + LaneAssigner::calculateSceneHeight(model_->maxLane(), ITEM_HEIGHT, LANE_SPACING);

    // Set scene rect with padding
    setSceneRect(startX, 0, width, height);

    // Update date scale and current date marker heights
    if (dateScale_)
    {
        dateScale_->setPaddedDateRange(paddedStart, paddedEnd);
        dateScale_->setTimelineHeight(height);
        dateScale_->updateScale();
    }

    if (currentDateMarker_)
    {
        currentDateMarker_->setTimelineHeight(height);
        currentDateMarker_->updatePosition();
    }

    // Update version boundary markers
    if (versionStartMarker_)
    {
        versionStartMarker_->setTimelineHeight(height);
        versionStartMarker_->updatePosition();
    }

    if (versionEndMarker_)
    {
        versionEndMarker_->setTimelineHeight(height);
        versionEndMarker_->updatePosition();
    }
}


TimelineItem* TimelineScene::findItemByEventId(const QString& eventId) const
{
    return eventIdToItem_.value(eventId, nullptr);
}


void TimelineScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    // Just call base implementation - items will emit their own clicked signals
    QGraphicsScene::mousePressEvent(event);
}


void TimelineScene::onEventAdded(const QString& eventId)
{
    createItemForEvent(eventId);
    updateSceneHeight();            // Adjust scene height for new lane
}


void TimelineScene::onEventRemoved(const QString& eventId)
{
    TimelineItem* item = findItemByEventId(eventId);

    if (item)
    {
        eventIdToItem_.remove(eventId);
        removeItem(item);
        delete item;
    }
    updateSceneHeight(); // Adjust scene height after removal
}


void TimelineScene::onEventUpdated(const QString& eventId)
{
    TimelineItem* item = findItemByEventId(eventId);

    if (item)
    {
        updateItemFromEvent(item, eventId);

        // Emit drag completed signal when event is updated
        // (This will be triggered after TimelineItem updates the model)
        emit itemDragCompleted(eventId);
    }
}


void TimelineScene::onVersionDatesChanged()
{
    // Version dates changed - need to recalculate all positions
    rebuildFromModel();
}


void TimelineScene::onLanesRecalculated()
{
    // When lanes are recalculated, update all item positions
    const auto& events = model_->getAllEvents();

    for (const auto& event : events)
    {
        TimelineItem* item = findItemByEventId(event.id);

        if (item)
        {
            updateItemFromEvent(item, event.id);
        }
    }
    updateSceneHeight();
}


TimelineItem* TimelineScene::createItemForEvent(const QString& eventId)
{
    const TimelineEvent* event = model_->getEvent(eventId);

    if (!event)
    {
        return nullptr;
    }

    // Calculate Y position based on lane (offset by date scale height)
    double yPos = DATE_SCALE_OFFSET + LaneAssigner::laneToY(event->lane, ITEM_HEIGHT, LANE_SPACING);

    // Calculate rectangle using coordinate mapper
    QRectF rect = mapper_->dateRangeToRect(event->startDate,
                                           event->endDate,
                                           yPos,
                                           ITEM_HEIGHT);


    // Create the item
    TimelineItem* item = new TimelineItem(rect);
    item->setEventId(eventId);
    item->setModel(model_);
    item->setCoordinateMapper(mapper_);
    item->setUndoStack(undoStack_);

    // Set visual properties
    item->setBrush(QBrush(event->color));
    item->setPen(QPen(Qt::black, 1));
    item->setToolTip(QString("%1\n%2 to %3\nLane: %4")
                         .arg(event->title)
                         .arg(event->startDate.toString(Qt::ISODate))
                         .arg(event->endDate.toString(Qt::ISODate))
                         .arg(event->lane));

    // Add to scene and tracking map
    addItem(item);
    eventIdToItem_[eventId] = item;

    connectItemSignals(item);

    return item;
}


void TimelineScene::updateItemFromEvent(TimelineItem* item, const QString& eventId)
{
    const TimelineEvent* event = model_->getEvent(eventId);

    if (!event)
    {
        return;
    }

    // Calculate Y position based on lane (offset by date scale height)
    double yPos = DATE_SCALE_OFFSET + LaneAssigner::laneToY(event->lane, ITEM_HEIGHT, LANE_SPACING);

    // Recalculate position and size
    QRectF newRect = mapper_->dateRangeToRect(event->startDate,
                                              event->endDate,
                                              yPos,
                                              ITEM_HEIGHT);

    item->setRect(newRect);
    item->setPos(0, 0);     // Reset position since rect includes the position
    item->setBrush(QBrush(event->color));
    item->setToolTip(QString("%1\n%2 to %3\nLane: %4")
                         .arg(event->title)
                         .arg(event->startDate.toString(Qt::ISODate))
                         .arg(event->endDate.toString(Qt::ISODate))
                         .arg(event->lane));
}


void TimelineScene::updateSceneHeight()
{
    // Calculate scene width with +1 month padding
    QDate paddedStart = model_->versionStartDate().addMonths(-1);
    QDate paddedEnd = model_->versionEndDate().addMonths(1);

    double startX = mapper_->dateToX(paddedStart);
    double endX = mapper_->dateToX(paddedEnd);
    double width = endX - startX;

    // Dynamically adjust scene height based on lane count
    double height = DATE_SCALE_OFFSET + LaneAssigner::calculateSceneHeight(model_->maxLane(), ITEM_HEIGHT, LANE_SPACING);

    setSceneRect(startX, 0, width, height);

    // Update date scale and marker heights
    if (dateScale_)
    {
        dateScale_->setPaddedDateRange(paddedStart, paddedEnd);
        dateScale_->setTimelineHeight(height);
        dateScale_->updateScale();
    }

    if (currentDateMarker_)
    {
        currentDateMarker_->setTimelineHeight(height);
        currentDateMarker_->updatePosition();
    }

    if (versionStartMarker_)
    {
        versionStartMarker_->setTimelineHeight(height);
        versionStartMarker_->updatePosition();
    }

    if (versionEndMarker_)
    {
        versionEndMarker_->setTimelineHeight(height);
        versionEndMarker_->updatePosition();
    }
}


void TimelineScene::keyPressEvent(QKeyEvent* event)
{
    // Handle Delete key with batch deletion support
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        QList<QGraphicsItem*> selected = selectedItems();

        if (!selected.isEmpty())
        {
            QStringList eventIds;

            // Collect all selected event IDs
            for (QGraphicsItem* item : selected)
            {
                TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(item);

                if (timelineItem && !timelineItem->eventId().isEmpty())
                {
                    eventIds.append(timelineItem->eventId());
                }
            }

            if (!eventIds.isEmpty())
            {
                // Emit appropriate signal based on count
                if (eventIds.size() == 1)
                {
                    // Single item - use regular delete signal
                    emit deleteEventRequested(eventIds.first());
                }
                else
                {
                    // Multiple items - use batch delete signal
                    emit batchDeleteRequested(eventIds);
                }
            }

            event->accept();

            return;
        }
    }

    QGraphicsScene::keyPressEvent(event);
}


void TimelineScene::connectItemSignals(TimelineItem* item)
{
    connect(item, &TimelineItem::editRequested, this, &TimelineScene::editEventRequested);
    connect(item, &TimelineItem::deleteRequested, this, &TimelineScene::deleteEventRequested);
    connect(item, &TimelineItem::clicked, this, &TimelineScene::itemClicked);
}
