// TimelineModel.cpp


#include "TimelineModel.h"
#include <QUuid>


TimelineModel::TimelineModel(QObject* parent)
    : QObject(parent)
    , versionStart_(QDate::currentDate())
    , versionEnd_(QDate::currentDate().addMonths(3))
{
    // Default to a 3-month version cycle starting today
}


void TimelineModel::setVersionDates(const QDate& start, const QDate& end)
{
    if(start.isValid() && end.isValid() && start <= end)
    {
        versionStart_ = start;
        versionEnd_ = end;
        emit versionDatesChanged();
    }
}


int TimelineModel::versionDurationDays() const
{
    return versionStart_.daysTo(versionEnd_) + 1;
}


QString TimelineModel::addEvent(const TimelineObject& event)
{
    // Validate event dates are within range
    if(event.startDate < versionStart_ || event.endDate > versionEnd_)
    {
        // ACTION: Add Error notification here for invalid date range of event for the current version (event timeframe lies outside the version time frame)
    }

    // Create a copy with generated Id if empty
    TimelineObject newEvent = event;
    if(newEvent.id.isEmpty())
    {
        newEvent.id = generateEventID();
    }

    // Assign color based on type if not set
    if(!newEvent.color.isValid())
    {
        newEvent.color = colorForType(newEvent.type);
    }

    events_.append(newEvent);
    emit eventAdded(newEvent.id);

    return newEvent.id;
}


bool TimelineModel::removeEvent(const QString& eventId)
{
    for(int i = 0; i < events_.size(); ++i)
    {
        if(events_[i].id == eventId)
        {
            events_.removeAt(i);
            emit eventRemoved(eventId);
            return true;
        }
    }
    return false;
}


bool TimelineModel::updateEvent(const QString& eventId, const TimelineObject& updatedEvent)
{
    for(int i = 0; i < events_.size(); ++i)
    {
        if(events_[i].id == eventId)
        {
            // Preserve the Id
            TimelineObject updated = updatedEvent;
            updated.id = eventId;
            events_[i] = updated;
            emit eventUpdated(eventId);
            return true;
        }
    }
    return false;
}


TimelineObject* TimelineModel::findEvent(const QString& eventId)
{
    for(int i = 0; i < events_.size(); ++i)
    {
        if(events_[i].id == eventId)
        {
            return &events_[i];
        }
    }
    return nullptr;
}


const TimelineObject* TimelineModel::findEvent(const QString& eventId) const
{
    for(int i = 0; i < events_.size(); ++i)
    {
        if(events_[i].id == eventId)
        {
            return &events_[i];
        }
    }
    return nullptr;
}


QVector<TimelineObject> TimelineModel::eventsOnDate(const QDate& date) const
{
    QVector<TimelineObject> result;

    for(const auto& event : events_)
    {
        if(event.occursOnDate(date))
        {
            result.append(event);
        }
    }
    return result;
}


QVector<TimelineObject> TimelineModel::eventsBetween(const QDate& start, const QDate& end) const
{
    QVector<TimelineObject> result;

    for(const auto& event : events_)
    {
        // Check if event overlaps with the range
        if(event.startDate <= end && event.startDate >= start)
        {
            result.append(event);
        }
    }
    return result;
}


QColor TimelineModel::colorForType(TimelineObjectType type)
{
    switch (type)
    {
        case TimelineObjectType::Meeting:
            return QColor(100, 149, 237);       // Cornflower Blue
        case TimelineObjectType::Action:
            return QColor(255, 165, 0);         // Orange
        case TimelineObjectType::TestEvent:
            return QColor(50, 205, 50);         // Lime Green
        case TimelineObjectType::DueDate:
            return QColor(220, 20, 60);         // Crimson
        case TimelineObjectType::Reminer:
            return QColor(255, 215, 0);         // Gold
        default:
            return QColor();                    // Gray fallback
    }
}


QString TimelineModel::generateEventId() const
{
    // Use QUuid for guarenteed uniqueness
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}












