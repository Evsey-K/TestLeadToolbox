// TimelineScene.cpp


#include "TimelineScene.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineItem.h"


TimelineScene::TimelineScene(TimelineModel* model, TimelineCoordinateMapper* mapper, QObject* parent)
    : QGraphicsScene(parent)
    , model_(model)
    , mapper_(mapper)
{
    // Connect to model signals
    connect(model_, &TimelineModel::eventAdded, this, &TimelineScene::onEventAdded);
    connect(model_, &TimelineModel::eventRemoved, this, &TimelineScene::onEventRemoved);
    connect(model_, &TimelineModel::eventUpdated, this, &TimelineScene::onEventUpdated);

    // Build initial scene from existing model data
    rebuildFromModel();
}


void TimelineScene::rebuildFromModel()
{
    // Clear all existing items
    clear();
    eventIdToItem_.clear();

    // Create items for all events in the model
    const auto& events = model_->allEvents();
    for(const auto& event : events)
    {
        createItemForEvent(event.id);
    }

    // Update scene rect to encompass the entire timeline
    double width = mapper_->totalWidth();
    setSceneRect(0, 0, width, 200);
}


TimelineItem* TimelineScene::findItemByEventId(const QString& eventId) const
{
    return eventIdToItem_.value(eventId, nullptr);
}


void TimelineScene::onEventAdded(const QString& eventId)
{
    createItemForEvent(eventId);
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
}


void TimelineScene::onEventUpdated(const QString& eventId)
{
    TimelineItem* item = findItemByEventId(eventId);

    if(item)
    {
        updateItemFromEvent(item, eventId);
    }
}


void TimelineScene::onVersionDatesChanged()
{
    // Version dates changed - need to recalculate all positions
    rebuildFromModel();
}


TimelineItem* TimelineScene::createItemForEvent(const QString& eventId)
{
    const TimelineObject* event = model_->findEvent(eventId);

    if(!event)
    {
        return nullptr;
    }

    // Calculate rectangle using coordinate mapper
    QRectF rect = mapper_->dateRangeToRect(event->startDate,
                                           event->endDate,
                                           ITEM_Y_POSITION,
                                           ITEM_HEIGHT);

    // Create the item
    TimelineItem* item = new TimelineItem(rect);
    item->setEventId(eventId);
    item->setModel(model_);
    item->setCoordinateMapper(mapper_);

    // Set visual properties
    item->setBrush(QBrush(event->color));
    item->setToolTip(QString("%1\n%2 to %3")
                         .arg(event->title)
                         .arg(event->startDate.toString(Qt::ISODate)
                         .arg(event->endDate.toString(Qt::ISODate))));

    // Add to scene and tracking map
    addItem(item);
    eventIdToItem_[eventId] = item;

    return item;
}


void TimelineScene::updateItemFromEvent(TimelineItem* item, const QString& eventId)
{
    const TimelineObject* event = model_->findEvent(eventId);
    if(!event)
    {
        return;
    }


    // Recalculate position and size
    QRectF newRect = mapper_->dateRangeToRect(event->startDate,
                                              event->endDate,
                                              ITEM_Y_POSITION,
                                              ITEM_HEIGHT);

    item->setRect(newRect);
    item->setBrush(QBrush(event->color));
    item->setToolTip(QString("%1\n%2 to %3")
                         .arg(event->title)
                         .arg(event->startDate.toString(Qt::ISODate)
                         .arg(event->endDate.toString(Qt::ISODate))));
}


























