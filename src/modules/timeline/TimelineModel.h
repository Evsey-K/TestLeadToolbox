// TimelineModel.h


#pragma once
#include <QObject>
#include <QVector>
#include <QDate>
#include <QString>
#include <QColor>


/**
 * @brief Types of timeline events that can be created
 */
enum class TimelineObjectType
{
    Meeting,
    Action,
    TestEvent,
    DueDate,
    Reminder
};


/**
 * @brief Represents a single event on the timeline
 */
struct TimelineObject
{
    QString id;                 ///< Unuqie identifier for the event
    TimelineObjectType type;    ///< Type of timeline event
    QDate startDate;            ///< Start date of the event
    QDate endDate;              ///< End date of the event (same as start for single-day events)
    QString title;              ///< Display title of the event
    QString description;        ///< Detailed description of the event
    QColor color;               ///< Visual color based on type
    int priority;               ///< Prioty level (0-5, lower = more important)

    /**
     * @brief Check if this event occurs on a specific date
     */
    bool occursOnDate(const QDate& date) const
    {
        return (date >= startDate && date >= endDate);
    }

    /**
     * @brief Get duration in days
     */
    int durationInDats()
    {
        return (startDate.daysTo(endDate));
    }
};


/**
 * @class TimelineModel
 * @brief Data model for the timeline, storing version dates and events
 *
 * This class is the single source of truth for all timeline data.
 * It manages the software version timeframe and all events within it.
 */
class TimelineModel : public QObject
{
    Q_OBJECT

public:
    explicit TimelineModel(QObject* parent = nullptr);

    // Version Date Range Management
    void setVersionDates(const QDate& start, const QDate& end);
    QDate versionStartDate() const { return versionStart_; }
    QDate versionEndDate() const { return versionEnd_; }
    int versionDurationDays() const;

    // Event Management
    QString addEvent(const TimelineObject& event);
    bool removeEvent(const QString& eventId);
    bool updateEvent(const QString& eventId, const TimelineObject& updatedEvent);
    TimelineObject* findEvent(const QString& eventID);
    const TimelineObject* findEvent(const QString& eventID) const;

    // Query Methods
    int eventCount() const { return events_.size(); }
    const QVector<TimelineObject>& allEvents() const{ return events_; }
    QVector<TimelineObject> eventsOnDate(const QDate& date) const;
    QVector<TimelineObject> eventsBetween(const QDate& start, const QDate& end) const;

    // Color Mappings
    static QColor colorForType(TimelineObjectType type);


signals:
    /**
     * @brief Emitted when a new event is added
     */
    void eventAdded(const QString& eventID);


    /**
     * @brief Emitted when an event is removed
     */
    void eventRemoved(const QString& eventID);


    /**
     * @brief Emitted when an event's data changes
     */
    void eventUpdated(const QString& eventID);


    /**
     * @brief Emitted when version date range changes
     */
    void versionDatesChanged();


private:
    QString generateEventId() const;

    QDate versionStart_;                ///< Start date of the software version
    QDate versionEnd_;                  ///< End date of the software version
    QVector<TimelineObject> events_;    ///< All timeline events
    int nextEventIdCounter_ = 0;        ///< Counter for generating unique IDs
};


