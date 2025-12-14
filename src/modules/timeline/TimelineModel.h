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
constexpr TimelineEventType TimelineEventType_Reminder = 4;
constexpr TimelineEventType TimelineEventType_JiraTicket = 5;

/**
 * @struct TimelineEvent
 * @brief Extended timeline event structure with type-specific fields
 *
 * NOTE: startDate and endDate are QDateTime objects that store both date and time.
 * For all-day events, the time component is set to 00:00:00 for start and 23:59:59 for end.
 * For events with hour precision, the exact time is stored.
 *
 * Field Usage by Type:
 *
 * Meeting:
 *   - title, startDate (QDateTime), endDate (QDateTime), location,
 *     participants, priority, description
 *
 * Action:
 *   - title, startDate (QDateTime), dueDateTime, status, priority, description
 *     Note: endDate is set to dueDateTime.date() with time 23:59:59 for rendering
 *
 * Test Event:
 *   - title, startDate (QDateTime), endDate (QDateTime), testCategory,
 *     preparationChecklist, priority, description
 *
 * Reminder:
 *   - title, reminderDateTime, recurringRule, description
 *     Note: startDate and endDate are derived from reminderDateTime for rendering
 *
 * Jira Ticket:
 *   - title, jiraKey, jiraSummary, jiraType, jiraStatus, startDate (QDateTime),
 *     endDate (QDateTime), priority, description
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

    // ========== LANE CONTROL FIELDS ==========
    bool laneControlEnabled = false;    ///< If true, user has manually set the lane
    int manualLane = 0;                 ///< User-specified lane number (when laneControlEnabled is true)

    // ========== DATE/TIME FIELDS ==========
    // CHANGED: These are now QDateTime instead of QDate
    QDateTime startDate;        ///< Start date/time (used by most types) - includes hour precision
    QDateTime endDate;          ///< End date/time (used by multi-day events) - includes hour precision

    // DEPRECATED: These fields are kept for backward compatibility but are now derived from startDate/endDate
    QTime startTime;            ///< Legacy: Use startDate.time() instead
    QTime endTime;              ///< Legacy: Use endDate.time() instead

    QDateTime reminderDateTime; ///< Reminder datetime (Reminder type)
    QDateTime dueDateTime;      ///< Due datetime (Action type)

    // ========== MEETING-SPECIFIC FIELDS ==========
    QString location;           ///< Physical location or virtual link
    QString participants;       ///< Comma-separated participant list

    // ========== ACTION-SPECIFIC FIELDS ==========
    QString status;             ///< Status: Not Started, In Progress, Blocked, Completed

    // ========== TEST EVENT-SPECIFIC FIELDS ==========
    QString testCategory;       ///< Category: Dry Run, Preliminary, Formal
    QMap<QString, bool> preparationChecklist;  ///< Checklist items with completion status

    // ========== REMINDER-SPECIFIC FIELDS ==========
    QString recurringRule;      ///< Recurrence rule: Daily, Weekly, Monthly

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
        return date >= startDate.date() && date <= endDate.date();
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
        return startDate.date().daysTo(endDate.date()) + 1;
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
            QString start = startDate.isValid() ? startDate.toString("yyyy-MM-dd HH:mm") : "Not Set";
            return QString("%1 → Due: %2").arg(start).arg(dueDateTime.toString("yyyy-MM-dd HH:mm"));
        }
        else if (startDate.date() == endDate.date())
        {
            // Same day - show time if not midnight
            if (startDate.time() == QTime(0, 0, 0) && endDate.time() == QTime(23, 59, 59))
            {
                return startDate.date().toString("yyyy-MM-dd");  // All-day event
            }
            else
            {
                return QString("%1 %2-%3")
                .arg(startDate.date().toString("yyyy-MM-dd"))
                    .arg(startDate.time().toString("HH:mm"))
                    .arg(endDate.time().toString("HH:mm"));
            }
        }
        else
        {
            return QString("%1 to %2")
            .arg(startDate.toString("yyyy-MM-dd HH:mm"))
                .arg(endDate.toString("yyyy-MM-dd HH:mm"));
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
    explicit TimelineModel(QObject* parent = nullptr);

    void setVersionDates(const QDate& start, const QDate& end);
    QDate versionStartDate() const { return versionStart_; }
    QDate versionEndDate() const { return versionEnd_; }
    QString addEvent(const TimelineEvent& event);
    bool removeEvent(const QString& eventId);
    bool updateEvent(const QString& eventId, const TimelineEvent& updatedEvent);

    TimelineEvent* getEvent(const QString& eventId);
    const TimelineEvent* getEvent(const QString& eventId) const;
    QVector<TimelineEvent> getAllEvents() const;
    QVector<TimelineEvent> getEventsInRange(const QDate& start, const QDate& end) const;
    QVector<TimelineEvent> getEventsForToday() const;
    QVector<TimelineEvent> getEventsLookahead(int days = 14) const;
    int eventCount() const { return events_.size(); }
    int maxLane() const { return maxLane_; }
    void clear();
    void recalculateLanes();
    static QColor colorForType(TimelineEventType type);
    bool archiveEvent(const QString& eventId);
    bool restoreEvent(const QString& eventId);
    bool permanentlyDeleteArchivedEvent(const QString& eventId);
    const TimelineEvent* getArchivedEvent(const QString& eventId) const;
    QVector<TimelineEvent> getAllArchivedEvents() const;

    bool hasLaneConflict(const QDateTime& startDateTime, const QDateTime& endDateTime, int manualLane, const QString& excludeEventId = QString()) const;

signals:
    void versionDatesChanged(const QDate& start, const QDate& end);
    void eventAdded(const QString& eventId);
    void eventRemoved(const QString& eventId);
    void eventUpdated(const QString& eventId);
    void eventArchived(const QString& eventId);
    void eventRestored(const QString& eventId);
    void lanesRecalculated();
    void eventsCleared();

private:
    void assignLanesToEvents();
    QString generateEventId() const;

    QDate versionStart_;
    QDate versionEnd_;
    QVector<TimelineEvent> events_;
    QVector<TimelineEvent> archivedEvents_;
    int maxLane_ = 0;
};
