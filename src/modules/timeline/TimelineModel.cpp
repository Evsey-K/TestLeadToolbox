// TimelineModel.cpp


#include "TimelineModel.h"
#include "LaneAssigner.h"
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
    if (getEvent(newEvent.id) != nullptr)
    {
        qWarning() << "Event with ID" << newEvent.id << "already exists";

        return QString(); // Return empty QString on failure
    }

    // Assign color based on type if not set
    if(!newEvent.color.isValid())
    {
        newEvent.color = colorForType(newEvent.type);
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
            assignLanesToEvents();          // Recalculate lanes after removal
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

            assignLanesToEvents();          // Recalculate lanes after update
            emit eventUpdated(eventId);

            return true;
        }
    }
    return false;
}


TimelineEvent* TimelineModel::getEvent(const QString& eventId)
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


const TimelineEvent* TimelineModel::getEvent(const QString& eventId) const
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


QVector<TimelineEvent> TimelineModel::getEventsInRange(const QDate& start, const QDate& end) const
{
    QVector<TimelineEvent> result;

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


QVector<TimelineEvent> TimelineModel::getAllEvents() const
{
    return events_;
}


QVector<TimelineEvent> TimelineModel::getEventsForToday() const
{
    QDate today = QDate::currentDate();
    QVector<TimelineEvent> result;

    for (const auto& event : events_)
    {
        if (event.occursOnDate(today))
        {
            result.append(event);
        }
    }

    return result;
}


QVector<TimelineEvent> TimelineModel::getEventsLookahead(int days) const
{
    QDate today = QDate::currentDate();
    QDate endDate = today.addDays(days);

    QVector<TimelineEvent> result;

    for (const auto& event : events_)
    {
        // Include events that start within the lookahead window
        if (event.startDate >= today && event.startDate <= endDate)
        {
            result.append(event);
        }
    }

    return result;
}


void TimelineModel::clear()
{
    events_.clear();
    archivedEvents_.clear();
    maxLane_ = 0;
    emit eventsCleared();
}


QColor TimelineModel::colorForType(TimelineEventType type)
{
    switch (type)
    {
    case TimelineEventType_Meeting:
        return QColor(30, 144, 255);        // Dodger Blue (#1E90FF) - Calm, standard for scheduling
    case TimelineEventType_Action:
        return QColor(220, 20, 60);         // Crimson (#DC143C) - Strong signal for urgency
    case TimelineEventType_TestEvent:
        return QColor(138, 43, 226);        // Blue Violet (#8A2BE2) - Technical/specialized
    case TimelineEventType_Reminder:
        return QColor(255, 215, 0);         // Gold (#FFD700) - Visibility and attention
    case TimelineEventType_JiraTicket:
        return QColor(50, 205, 50);         // Lime Green (#32CD32) - Progress/tasks/neutral work item
    default:
        return QColor(128, 128, 128);       // Gray fallback for unknown types
    }
}


QString TimelineModel::generateEventId() const
{
    // Use QUuid for guarenteed uniqueness
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}


void TimelineModel::assignLanesToEvents()
{
    if (events_.isEmpty())
    {
        maxLane_ = 0;
        return;
    }

    // Separate events into manually-controlled and auto-assigned
    QVector<LaneAssigner::EventData> autoEvents;
    QVector<LaneAssigner::EventData> manualEvents;

    for (int i = 0; i < events_.size(); ++i)
    {
        TimelineEvent& event = events_[i];

        LaneAssigner::EventData data(
            event.id,
            event.startDate,
            event.endDate,
            &event
            );

        if (event.laneControlEnabled)
        {
            // For manual events, set the lane to the user-specified value
            data.lane = event.manualLane;
            event.lane = event.manualLane;
            manualEvents.append(data);
        }
        else
        {
            autoEvents.append(data);
        }
    }

    // Assign lanes to auto events, respecting manual events
    if (!autoEvents.isEmpty())
    {
        int maxLaneUsed = LaneAssigner::assignLanesWithReserved(autoEvents, manualEvents);

        // Apply assigned lanes back to events
        for (const auto& data : autoEvents)
        {
            if (data.userData)
            {
                TimelineEvent* event = static_cast<TimelineEvent*>(data.userData);
                event->lane = data.lane;
            }
        }

        // Update maxLane
        maxLane_ = maxLaneUsed;
    }
    else
    {
        // Only manual events, find max lane
        maxLane_ = 0;
        for (const auto& data : manualEvents)
        {
            if (data.lane > maxLane_)
            {
                maxLane_ = data.lane;
            }
        }
    }

    emit lanesRecalculated();
}


bool TimelineModel::archiveEvent(const QString& eventId)
{
    // Find event in active list
    for (int i = 0; i < events_.size(); ++i)
    {
        if (events_[i].id == eventId)
        {
            // Mark as archived
            events_[i].archived = true;

            // Move to archived list
            archivedEvents_.append(events_[i]);
            events_.removeAt(i);

            // Recalculate lanes after removal
            assignLanesToEvents();

            emit eventArchived(eventId);
            qDebug() << "Event archived:" << eventId;

            return true;
        }
    }

    qWarning() << "Cannot archive event - not found:" << eventId;

    return false;
}


bool TimelineModel::restoreEvent(const QString& eventId)
{
    // Find event in archived list
    for (int i = 0; i < archivedEvents_.size(); ++i)
    {
        if (archivedEvents_[i].id == eventId)
        {
            // Unmark archived flag
            archivedEvents_[i].archived = false;

            // Move back to active list
            events_.append(archivedEvents_[i]);
            archivedEvents_.removeAt(i);

            // Recalculate lanes after adding
            assignLanesToEvents();

            emit eventRestored(eventId);
            qDebug() << "Event restored:" << eventId;

            return true;
        }
    }

    qWarning() << "Cannot restore event - not found in archive:" << eventId;
    return false;
}


bool TimelineModel::permanentlyDeleteArchivedEvent(const QString& eventId)
{
    // Find event in archived list
    for (int i = 0; i < archivedEvents_.size(); ++i)
    {
        if (archivedEvents_[i].id == eventId)
        {
            archivedEvents_.removeAt(i);

            emit eventRemoved(eventId);
            qDebug() << "Archived event permanently deleted:" << eventId;

            return true;
        }
    }

    qWarning() << "Cannot permanently delete event - not found in archive:" << eventId;
    return false;
}


const TimelineEvent* TimelineModel::getArchivedEvent(const QString& eventId) const
{
    for (int i = 0; i < archivedEvents_.size(); ++i)
    {
        if (archivedEvents_[i].id == eventId)
        {
            return &archivedEvents_[i];
        }
    }

    return nullptr;
}


QVector<TimelineEvent> TimelineModel::getAllArchivedEvents() const
{
    return archivedEvents_;
}


bool TimelineModel::hasLaneConflict(const QDate& startDate, const QDate& endDate,
                                    int manualLane, const QString& excludeEventId) const
{
    // Check if any existing lane-controlled event occupies this lane and overlaps in time
    for (const auto& event : events_)
    {
        // Skip the event being updated
        if (!excludeEventId.isEmpty() && event.id == excludeEventId)
        {
            continue;
        }

        // Only check events with lane control enabled
        if (!event.laneControlEnabled)
        {
            continue;
        }

        // Check if same lane
        if (event.manualLane != manualLane)
        {
            continue;
        }

        // Check for date overlap
        // Events overlap if: startA <= endB AND endA >= startB
        if (startDate <= event.endDate && endDate >= event.startDate)
        {
            return true;  // Conflict found
        }
    }

    return false;  // No conflict
}
















