// TimelineModel.h

#pragma once
#include <QObject>
#include <QVector>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QString>
#include <QColor>
#include <QMap>

// Forward declarations
using TimelineEventType = int;
constexpr TimelineEventType TimelineEventType_Meeting = 0;
constexpr TimelineEventType TimelineEventType_Action = 1;
constexpr TimelineEventType TimelineEventType_TestEvent = 2;
// TimelineEventType_DueDate = 3;  // REMOVED in favor of unified Action type
constexpr TimelineEventType TimelineEventType_Reminder = 4;
constexpr TimelineEventType TimelineEventType_JiraTicket = 5;  // NEW

/**
 * @struct TimelineEvent
 * @brief Extended timeline event structure with type-specific fields
 *
 * This structure supports multiple event types with dynamic field usage:
 * - Common fields: Used by all event types
 * - Type-specific fields: Only populated for relevant event types
 *
 * Field Usage by Type:
 *
 * Meeting:
 *   - title, startDate, endDate, startTime, endTime, location,
 *     participants, priority, description
 *
 * Action:
 *   - title, startDate, startTime, dueDateTime, status, priority, description
 *
 * Test Event:
 *   - title, startDate, endDate, testCategory, preparationChecklist,
 *     priority, description
 *
 * Reminder:
 *   - title, reminderDateTime, recurringRule, description
 *
 * Jira Ticket:
 *   - title, jiraKey, jiraSummary, jiraType, jiraStatus, startDate,
 *     endDate (as due date), priority, description
 */
struct TimelineEvent
{
    // ========== COMMON FIELDS (All Event Types) ==========
    QString id;                 ///< Unique identifier for the event
    TimelineEventType type;     ///< Type of timeline event
    QString title;              ///< Display title of the event
    QString description;        ///< Detailed description of the event
    QColor color;               ///< Visual color based on type
    int priority = 0;           ///< Priority level (0-5, lower = more important)
    int lane = 0;               ///< Vertical lane for collision avoidance
    bool archived = false;      ///< Soft-delete flag

    // ========== DATE/TIME FIELDS ==========
    QDate startDate;            ///< Start date (used by most types)
    QDate endDate;              ///< End date (used by multi-day events)
    QTime startTime;            ///< Start time (Meeting, Action)
    QTime endTime;              ///< End time (Meeting)
    QDateTime reminderDateTime; ///< Reminder datetime (Reminder)
    QDateTime dueDateTime;      ///< Due datetime (Action)

    // ========== MEETING-SPECIFIC FIELDS ==========
    QString location;           ///< Physical location or virtual link
    QString participants;       ///< Comma-separated participant list

    // ========== ACTION-SPECIFIC FIELDS ==========
    QString status;             ///< Status: Not Started, In Progress, Blocked, Completed

    // ========== REMINDER-SPECIFIC FIELDS ==========
    QString recurringRule;      ///< Recurrence rule: Daily, Weekly, Monthly

    // ========== TEST EVENT-SPECIFIC FIELDS ==========
    QString testCategory;       ///< Category: Dry Run, Preliminary, Formal
    QMap<QString, bool> preparationChecklist;  ///< Checklist items with completion status

    // ========== JIRA TICKET-SPECIFIC FIELDS ==========
    QString jiraKey;            ///< Jira ticket key (e.g., ABC-123)
    QString jiraSummary;        ///< Brief summary from Jira
    QString jiraType;           ///< Type: Story, Bug, Task, Sub-task, Epic
    QString jiraStatus;         ///< Status: To Do, In Progress, Done

    TimelineEvent() = default;

    /**
     * @brief Checks if this event occurs on a specific date
     */
    bool occursOnDate(const QDate& date) const
    {
        return date >= startDate && date <= endDate;
    }

    /**
     * @brief Checks if this event overlaps with another
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

    /**
     * @brief Get the primary display date range string
     */
    QString displayDateRange() const
    {
        if (type == TimelineEventType_Reminder)
        {
            return reminderDateTime.toString("yyyy-MM-dd HH:mm");
        }
        else if (type == TimelineEventType_Action && dueDateTime.isValid())
        {
            QString start = startDate.isValid() ? startDate.toString("yyyy-MM-dd") : "Not Set";
            return QString("%1 → Due: %2").arg(start).arg(dueDateTime.toString("yyyy-MM-dd HH:mm"));
        }
        else if (startDate == endDate)
        {
            return startDate.toString("yyyy-MM-dd");
        }
        else
        {
            return QString("%1 to %2").arg(startDate.toString("yyyy-MM-dd"))
            .arg(endDate.toString("yyyy-MM-dd"));
        }
    }
};

/**
 * @class TimelineModel
 * @brief Data model with collision avoidance and lane tracking
 */
class TimelineModel : public QObject
{
    Q_OBJECT

public:
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
     * @brief Gets an event by ID (mutable)
     */
    TimelineEvent* getEvent(const QString& eventId);

    /**
     * @brief Gets an event by ID (const)
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

    /**
     * @brief Returns color for a given event type
     */
    static QColor colorForType(TimelineEventType type);

    /**
     * @brief Archive an event (soft delete)
     */
    bool archiveEvent(const QString& eventId);

    /**
     * @brief Restore an archived event
     */
    bool restoreEvent(const QString& eventId);

    /**
     * @brief Permanently delete an archived event
     */
    bool permanentlyDeleteArchivedEvent(const QString& eventId);

    /**
     * @brief Get an archived event by ID
     */
    const TimelineEvent* getArchivedEvent(const QString& eventId) const;

    /**
     * @brief Get all archived events
     */
    QVector<TimelineEvent> getAllArchivedEvents() const;

signals:
    /**
     * @brief Emitted when an event is added
     */
    void eventAdded(const QString& eventId);

    /**
     * @brief Emitted when an event is removed
     */
    void eventRemoved(const QString& eventId);

    /**
     * @brief Emitted when an event is updated
     */
    void eventUpdated(const QString& eventId);

    /**
     * @brief Emitted when lanes are recalculated
     */
    void lanesRecalculated();

    /**
     * @brief Emitted when version dates change
     * @param start New version start date
     * @param end New version end date
     */
    void versionDatesChanged(const QDate& start, const QDate& end);

    /**
     * @brief Emitted when all events are cleared from the model
     */
    void eventsCleared();

    /**
     * @brief Emitted when an event is archived (soft delete)
     * @param eventId ID of the archived event
     */
    void eventArchived(const QString& eventId);

    /**
     * @brief Emitted when an archived event is restored
     * @param eventId ID of the restored event
     */
    void eventRestored(const QString& eventId);

private:
    /**
     * @brief Assigns lanes to all events to prevent visual overlap
     */
    void assignLanesToEvents();

    /**
     * @brief Generates a unique event ID
     */
    QString generateEventId() const;

    QDate versionStart_;                        ///< Version start date boundary
    QDate versionEnd_;                          ///< Version end date boundary
    QVector<TimelineEvent> events_;             ///< Active events
    QVector<TimelineEvent> archivedEvents_;     ///< Archived events
    int maxLane_ = 0;                           ///< Maximum lane number in use
};
