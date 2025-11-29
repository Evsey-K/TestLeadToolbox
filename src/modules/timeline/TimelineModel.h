// TimelineModel.h


#pragma once
#include <QObject>
#include <QVector>
#include <QDate>
#include <QString>
#include <QColor>
#include <QMap>
#include "LaneAssigner.h"


/**
 * @class TimelineModel
 * @brief Data model with collision avoidance and lane tracking
 */
class TimelineModel : public QObject
{
    Q_OBJECT

public:
    /**
     * @enum EventType
     * @brief Categories of timeline events with distinct visual styling
     */
    enum class EventType
    {
        Meeting,
        Action,
        TestEvent,
        DueDate,
        Reminder
    };
    Q_ENUM(EventType)

    /**
    * @struct TimelineEvent
    * @brief Represents a single timeline event with all metadata
    */
    struct TimelineEvent
    {
        QString id;                 ///< Unuqie identifier for the event
        EventType type;             ///< Type of timeline event
        QDate startDate;            ///< Start date of the event
        QDate endDate;              ///< End date of the event (same as start for single-day events)
        QString title;              ///< Display title of the event
        QString description;        ///< Detailed description of the event
        QColor color;               ///< Visual color based on type
        int priority = 0;           ///< Prioty level (0-5, lower = more important)
        int lane = 0;               ///< Vertical lane for collision avoidance

        TimelineEvent() = default;

        TimelineEvent(const QString& id_, EventType type_, const QDate& start,
                      const QDate& end, const QString& title_, const QString& desc = "",
                      int prio = 0)
            : id(id_), type(type_), startDate(start), endDate(end),
            title(title_), description(desc), priority(prio)
        {
            color = colorForType(type_);
        }

        /**
         * @brief Returns display color based on event type
         */
        static QColor colorForType(EventType type)
        {
            switch (type)
            {
            case EventType::Meeting:   return QColor(66, 133, 244);  // Blue
            case EventType::Action:    return QColor(219, 68, 55);   // Red
            case EventType::TestEvent: return QColor(15, 157, 88);   // Green
            case EventType::DueDate:   return QColor(244, 160, 0);   // Orange
            case EventType::Reminder:  return QColor(171, 71, 188);  // Purple
            default:                   return QColor(100, 100, 100); // Gray
            }
        }

        /**
         * @brief Checks if this event overlaps with another in date range
         */
        bool overlaps(const TimelineEvent& other) const
        {
            return !(endDate < other.startDate || startDate > other.endDate);
        }

        /**
         * @brief Returns duration in days (inclusive)
         */
        int durationDays() const
        {
            return startDate.daysTo(endDate) + 1;
        }
    };

    /**
     * @brief Constructs a TimelineModel with version date range
     * @param parent Qt parent object
     */
    explicit TimelineModel(QObject* parent = nullptr);

    /**
     * @brief Sets the version's date range for the timeline
     */
    void setVersionDates(const QDate& start, const QDate& end);

    /**
     * @brief Gets the version start date
     */
    QDate versionStartDate() const { return versionStart_; }

    /**
     * @brief Gets the version end date
     */
    QDate versionEndDate() const { return versionEnd_; }

    /**
     * @brief Adds a new event to the timeline
     * @param event Event to add
     * @return Event ID (generated if empty), or empty QString on failure
     */
    QString addEvent(const TimelineEvent& event);

    /**
     * @brief Removes an event from the timeline
     * @param eventId Event ID to remove
     * @return true if removed successfully
     */
    bool removeEvent(const QString& eventId);

    /**
     * @brief Updates an existing event
     * @param eventId Event ID
     * @param updatedEvent New event data
     * @return true if updated successfully
     */
    bool updateEvent(const QString& eventId, const TimelineEvent& updatedEvent);

    /**
     * @brief Gets an event by ID
     * @param eventId Event ID
     * @return Pointer to event, or nullptr if not found
     */
    const TimelineEvent* getEvent(const QString& eventId) const;

    /**
     * @brief Gets all events (sorted by start date, then lane)
     */
    QVector<TimelineEvent> getAllEvents() const;

    /**
     * @brief Gets events in date range
     */
    QVector<TimelineEvent> getEventsInRange(const QDate& start, const QDate& end) const;

    /**
     * @brief Gets events for today
     */
    QVector<TimelineEvent> getEventsForToday() const;

    /**
     * @brief Gets events in next N days (lookahead)
     */
    QVector<TimelineEvent> getEventsLookahead(int days = 14) const;

    /**
     * @brief Returns total number of events
     */
    int eventCount() const { return events_.size(); }

    /**
     * @brief Returns maximum lane number used (for scene height calculation)
     */
    int maxLane() const { return maxLane_; }

    /**
     * @brief Clears all events
     */
    void clear();

    /**
     * @brief Force recalculation of all lanes
     */
    void recalculateLanes();

signals:
    /**
     * @brief Emitted when a new event is added
     */
    void eventAdded(const QString& eventId);

    /**
     * @brief Emitted when an event is removed
     */
    void eventRemoved(const QString& eventId);

    /**
     * @brief Emitted when an event's data changes
     */
    void eventUpdated(const QString& eventId);

    /**
     * @brief Emitted when all events are cleared
     */
    void eventsCleared();

    /**
     * @brief Emitted when lanes have been recalculated (Sprint 2)
     */
    void lanesRecalculated();

    /**
     * @brief Emitted when version date range changes
     */
    void versionDatesChanged(const QDate& start, const QDate& end);

private:
    /**
     * @brief Generates a unique event ID using UUID
     */
    QString generateEventId() const;

    /**
     * @brief Internal lane assignment using LaneAssigner algorithm
     */
    void assignLanesToEvents();

    QVector<TimelineEvent> events_;     ///< All timeline events
    QDate versionStart_;                ///< Start date of the software version
    QDate versionEnd_;                  ///< End date of the software version
    int nextEventIdCounter_ = 0;        ///< Counter for generating unique IDs
    int maxLane_ = 0;                   ///< Maximum lane number currently used
};


