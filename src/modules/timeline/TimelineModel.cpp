// TimelineModel.cpp


#include "TimelineModel.h"
#include <QUuid>
#include <algorithm>


TimelineModel::TimelineModel(QObject* parent)
    : QObject(parent)
    , versionStart_(QDate::currentDate())
    , versionEnd_(QDate::currentDate().addMonths(3))
{
    // Default to a 3-month version cycle starting today
}


void TimelineModel::setVersionDates(const QDate& start, const QDate& end)
{
    if (start > end)
    {
        qWarning() << "Invalid version range: start > end";
        return;
    }

    versionStart_ = start;
    versionEnd_ = end;
    emit versionDatesChanged(start, end);
}


QString TimelineModel::addEvent(const TimelineEvent& event)
{
    // Validate event dates are within Version range
    if(event.startDate < versionStart_ || event.endDate > versionEnd_)
    {
        qWarning() << "Event dates outside version range:"
                   << "Event:" << event.startDate << "to" << event.endDate
                   << "Version:" << versionStart_ << "to" << versionEnd_;
    }

    // Create a copy with generated Id if empty
    TimelineEvent newEvent = event;
    if(newEvent.id.isEmpty())
    {
        newEvent.id = generateEventId();
    }

    // Check for duplicate ID
    if (events_.contains(newEvent.id))
    {
        qWarning() << "Event with ID" << newEvent.id << "already exists";

        return QString();   // Return empty QString on failure
    }

    // Assign color based on type if not set
    if(!newEvent.color.isValid())
    {
        newEvent.color = TimelineEvent::colorForType(newEvent.type);
    }

    // Add to internal storage
    events_.append(newEvent);

    // Recalculate lanes after adding new event (Sprint 2 enhancement)
    assignLanesToEvents();

    // Emit signal
    emit eventAdded(newEvent.id);

    // Return the event ID
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


bool TimelineModel::updateEvent(const QString& eventId, const TimelineEvent& updatedEvent)
{
    for(int i = 0; i < events_.size(); ++i)
    {
        if(events_[i].id == eventId)
        {
            // Preserve the Id
            TimelineEvent updated = updatedEvent;
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
        case TimelineObjectType::Reminder:
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












