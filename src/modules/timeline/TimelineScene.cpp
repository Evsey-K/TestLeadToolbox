// TimelineScene.cpp


#include "TimelineScene.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineItem.h"
#include "LaneAssigner.h"
#include <QPen>


TimelineScene::TimelineScene(TimelineModel* model, TimelineCoordinateMapper* mapper, QObject* parent)
    : QGraphicsScene(parent)
    , model_(model)
    , mapper_(mapper)
{
    // Connect to model signals
    connect(model_, &TimelineModel::eventAdded, this, &TimelineScene::onEventAdded);
    connect(model_, &TimelineModel::eventRemoved, this, &TimelineScene::onEventRemoved);
    connect(model_, &TimelineModel::eventUpdated, this, &TimelineScene::onEventUpdated);
    connect(model_, &TimelineModel::versionDatesChanged, this, &TimelineScene::onVersionDatesChanged);

    // Connect to lane recalculation signal
    connect(model_, &TimelineModel::lanesRecalculated, this, &TimelineScene::onLanesRecalculated);

    // Build initial scene from existing model data
    rebuildFromModel();
}


void TimelineScene::rebuildFromModel()
{
    // Clear all existing items
    clear();
    eventIdToItem_.clear();

    // Create items for all events in the model
    const auto& events = model_->getAllEvents();
    for(const auto& event : events)
    {
        createItemForEvent(event.id);
    }

    // Update scene rect to include all lanes
    double width = mapper_->totalWidth();
    double height = LaneAssigner::calculateSceneHeight(model_->maxLane(), ITEM_HEIGHT, LANE_SPACING);
    setSceneRect(0, 0, width, height);
}


TimelineItem* TimelineScene::findItemByEventId(const QString& eventId) const
{
    return eventIdToItem_.value(eventId, nullptr);
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

    // Calculate Y position based on lane
    double yPos = LaneAssigner::laneToY(event->lane, ITEM_HEIGHT, LANE_SPACING);

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

    // Set visual properties
    item->setBrush(QBrush(event->color));
    item->setPen(QPen(Qt::black, 1));
    item->setToolTip(QString("%1\n%2 to %3")
                         .arg(event->title)
                         .arg(event->startDate.toString(Qt::ISODate)
                         .arg(event->endDate.toString(Qt::ISODate))
                         .arg(event->lane)));

    // Add to scene and tracking map
    addItem(item);
    eventIdToItem_[eventId] = item;

    return item;
}


void TimelineScene::updateItemFromEvent(TimelineItem* item, const QString& eventId)
{
    const TimelineEvent* event = model_->getEvent(eventId);

    if (!event)
    {
        return;
    }

    // Calculate Y position based on lane
    double yPos = LaneAssigner::laneToY(event->lane, ITEM_HEIGHT, LANE_SPACING);

    // Recalculate position and size
    QRectF newRect = mapper_->dateRangeToRect(event->startDate,
                                              event->endDate,
                                              yPos,
                                              ITEM_HEIGHT);

    item->setRect(newRect);
    item->setPos(0, 0);     // Reset position since rect includes the position
    item->setBrush(QBrush(event->color));
    item->setToolTip(QString("%1\n%2 to %3")
                         .arg(event->title)
                         .arg(event->startDate.toString(Qt::ISODate))
                         .arg(event->endDate.toString(Qt::ISODate))
                         .arg(event->lane));
}


void TimelineScene::updateSceneHeight()
{
    // SPRINT 2: Dynamically adjust scene height based on lane count
    double width = mapper_->totalWidth();
    double height = LaneAssigner::calculateSceneHeight(model_->maxLane(), ITEM_HEIGHT, LANE_SPACING);
    setSceneRect(0, 0, width, height);
}

