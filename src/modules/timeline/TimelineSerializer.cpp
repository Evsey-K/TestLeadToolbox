// TimelineSerializer.cpp - Updated with Type-Specific Field Support

#include "TimelineSerializer.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

bool TimelineSerializer::saveToFile(const TimelineModel* model, const QString& filePath)
{
    if (!model)
    {
        qWarning() << "TimelineSerializer::saveToFile - null model provided";
        return false;
    }

    // Serialize model to JSON
    QJsonObject jsonObj = serializeModel(model);
    QJsonDocument doc(jsonObj);

    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "Timeline saved to:" << filePath;
    return true;
}

bool TimelineSerializer::loadFromFile(TimelineModel* model, const QString& filePath)
{
    if (!model)
    {
        qWarning() << "TimelineSerializer::loadFromFile - null model provided";
        return false;
    }

    // Read file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open file for reading:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    // Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject())
    {
        qWarning() << "Invalid JSON format in file:" << filePath;
        return false;
    }

    // Deserialize into model
    return deserializeModel(model, doc.object());
}

QJsonObject TimelineSerializer::serializeModel(const TimelineModel* model)
{
    QJsonObject obj;

    // Serialize version dates
    obj["versionStart"] = model->versionStartDate().toString(Qt::ISODate);
    obj["versionEnd"] = model->versionEndDate().toString(Qt::ISODate);

    // Serialize active events
    QJsonArray eventsArray;
    QVector<TimelineEvent> events = model->getAllEvents();
    for (const TimelineEvent& event : events)
    {
        eventsArray.append(serializeEvent(event));
    }
    obj["events"] = eventsArray;

    // Serialize archived events
    QJsonArray archivedArray;
    QVector<TimelineEvent> archivedEvents = model->getAllArchivedEvents();
    for (const TimelineEvent& event : archivedEvents)
    {
        archivedArray.append(serializeEvent(event));
    }
    obj["archivedEvents"] = archivedArray;

    // Add version info for future migration support
    obj["serializerVersion"] = "2.0";  // Bumped version for new field support

    return obj;
}

bool TimelineSerializer::deserializeModel(TimelineModel* model, const QJsonObject& json)
{
    model->clear();

    // Deserialize version dates
    QString startDateStr = json["versionStart"].toString();
    QString endDateStr = json["versionEnd"].toString();

    QDate startDate = QDate::fromString(startDateStr, Qt::ISODate);
    QDate endDate = QDate::fromString(endDateStr, Qt::ISODate);

    if (startDate.isValid() && endDate.isValid())
    {
        model->setVersionDates(startDate, endDate);
    }

    // Deserialize active events
    QJsonArray eventsArray = json["events"].toArray();
    for (const QJsonValue& val : eventsArray)
    {
        TimelineEvent event = deserializeEvent(val.toObject());
        model->addEvent(event);
    }

    // Deserialize archived events (if present)
    if (json.contains("archivedEvents"))
    {
        QJsonArray archivedArray = json["archivedEvents"].toArray();
        for (const QJsonValue& val : archivedArray)
        {
            TimelineEvent event = deserializeEvent(val.toObject());
            event.archived = true;
            // Add and then archive (requires the event to exist first)
            QString eventId = model->addEvent(event);
            if (!eventId.isEmpty())
            {
                model->archiveEvent(eventId);
            }
        }
    }

    return true;
}

QString TimelineSerializer::getDefaultSaveLocation()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists())
    {
        dir.mkpath(".");
    }
    return dir.filePath("timeline.json");
}

bool TimelineSerializer::createBackup(const QString& filePath)
{
    if (!QFile::exists(filePath))
    {
        return false;  // Nothing to back up
    }

    QString backupPath = filePath + ".backup";

    // Remove old backup if exists
    if (QFile::exists(backupPath))
    {
        QFile::remove(backupPath);
    }

    return QFile::copy(filePath, backupPath);
}

// ============================================================================
// PRIVATE HELPER METHODS - Event Serialization
// ============================================================================

QJsonObject TimelineSerializer::serializeEvent(const TimelineEvent& event)
{
    QJsonObject obj;

    // Common fields (all event types)
    obj["id"] = event.id;
    obj["type"] = eventTypeToString(event.type);
    obj["title"] = event.title;
    obj["description"] = event.description;
    obj["priority"] = event.priority;
    obj["lane"] = event.lane;
    obj["archived"] = event.archived;
    obj["color"] = event.color.name();

    // Date/Time fields (used by most types)
    if (event.startDate.isValid())
        obj["startDate"] = event.startDate.toString(Qt::ISODate);
    if (event.endDate.isValid())
        obj["endDate"] = event.endDate.toString(Qt::ISODate);
    if (event.startTime.isValid())
        obj["startTime"] = event.startTime.toString("HH:mm");
    if (event.endTime.isValid())
        obj["endTime"] = event.endTime.toString("HH:mm");
    if (event.reminderDateTime.isValid())
        obj["reminderDateTime"] = event.reminderDateTime.toString(Qt::ISODate);
    if (event.dueDateTime.isValid())
        obj["dueDateTime"] = event.dueDateTime.toString(Qt::ISODate);

    // Meeting-specific fields
    if (!event.location.isEmpty())
        obj["location"] = event.location;
    if (!event.participants.isEmpty())
        obj["participants"] = event.participants;

    // Action-specific fields
    if (!event.status.isEmpty())
        obj["status"] = event.status;

    // Reminder-specific fields
    if (!event.recurringRule.isEmpty())
        obj["recurringRule"] = event.recurringRule;

    // Test Event-specific fields
    if (!event.testCategory.isEmpty())
        obj["testCategory"] = event.testCategory;

    if (!event.preparationChecklist.isEmpty())
    {
        QJsonObject checklistObj;
        for (auto it = event.preparationChecklist.begin(); it != event.preparationChecklist.end(); ++it)
        {
            checklistObj[it.key()] = it.value();
        }
        obj["preparationChecklist"] = checklistObj;
    }

    // Jira Ticket-specific fields
    if (!event.jiraKey.isEmpty())
        obj["jiraKey"] = event.jiraKey;
    if (!event.jiraSummary.isEmpty())
        obj["jiraSummary"] = event.jiraSummary;
    if (!event.jiraType.isEmpty())
        obj["jiraType"] = event.jiraType;
    if (!event.jiraStatus.isEmpty())
        obj["jiraStatus"] = event.jiraStatus;

    return obj;
}

TimelineEvent TimelineSerializer::deserializeEvent(const QJsonObject& json)
{
    TimelineEvent event;

    // Common fields
    event.id = json["id"].toString();
    event.type = stringToEventType(json["type"].toString());
    event.title = json["title"].toString();
    event.description = json["description"].toString();
    event.priority = json["priority"].toInt();
    event.lane = json["lane"].toInt();
    event.archived = json["archived"].toBool();

    // Parse color
    QString colorStr = json["color"].toString();
    if (!colorStr.isEmpty())
    {
        event.color = QColor(colorStr);
    }

    // Date/Time fields (with validity checks)
    if (json.contains("startDate"))
        event.startDate = QDate::fromString(json["startDate"].toString(), Qt::ISODate);
    if (json.contains("endDate"))
        event.endDate = QDate::fromString(json["endDate"].toString(), Qt::ISODate);
    if (json.contains("startTime"))
        event.startTime = QTime::fromString(json["startTime"].toString(), "HH:mm");
    if (json.contains("endTime"))
        event.endTime = QTime::fromString(json["endTime"].toString(), "HH:mm");
    if (json.contains("reminderDateTime"))
        event.reminderDateTime = QDateTime::fromString(json["reminderDateTime"].toString(), Qt::ISODate);
    if (json.contains("dueDateTime"))
        event.dueDateTime = QDateTime::fromString(json["dueDateTime"].toString(), Qt::ISODate);

    // Meeting-specific fields
    if (json.contains("location"))
        event.location = json["location"].toString();
    if (json.contains("participants"))
        event.participants = json["participants"].toString();

    // Action-specific fields
    if (json.contains("status"))
        event.status = json["status"].toString();

    // Reminder-specific fields
    if (json.contains("recurringRule"))
        event.recurringRule = json["recurringRule"].toString();

    // Test Event-specific fields
    if (json.contains("testCategory"))
        event.testCategory = json["testCategory"].toString();

    if (json.contains("preparationChecklist"))
    {
        QJsonObject checklistObj = json["preparationChecklist"].toObject();
        for (auto it = checklistObj.begin(); it != checklistObj.end(); ++it)
        {
            event.preparationChecklist[it.key()] = it.value().toBool();
        }
    }

    // Jira Ticket-specific fields
    if (json.contains("jiraKey"))
        event.jiraKey = json["jiraKey"].toString();
    if (json.contains("jiraSummary"))
        event.jiraSummary = json["jiraSummary"].toString();
    if (json.contains("jiraType"))
        event.jiraType = json["jiraType"].toString();
    if (json.contains("jiraStatus"))
        event.jiraStatus = json["jiraStatus"].toString();

    // Backward Compatibility: Convert old DueDate type to Action
    if (event.type == 3)  // Old TimelineEventType_DueDate value
    {
        qDebug() << "Migrating old DueDate event to Action type:" << event.title;
        event.type = TimelineEventType_Action;
        event.status = "Not Started";  // Default status for migrated events
    }

    return event;
}


QString TimelineSerializer::eventTypeToString(TimelineEventType type)
{
    switch (type)
    {
    case TimelineEventType_Meeting:
        return "Meeting";
    case TimelineEventType_Action:
        return "Action";
    case TimelineEventType_TestEvent:
        return "TestEvent";
    case TimelineEventType_Reminder:
        return "Reminder";
    case TimelineEventType_JiraTicket:
        return "JiraTicket";
    default:
        return "Unknown";
    }
}


TimelineEventType TimelineSerializer::stringToEventType(const QString& typeStr)
{
    if (typeStr == "Meeting")
        return TimelineEventType_Meeting;
    if (typeStr == "Action")
        return TimelineEventType_Action;
    if (typeStr == "TestEvent")
        return TimelineEventType_TestEvent;
    if (typeStr == "Reminder")
        return TimelineEventType_Reminder;
    if (typeStr == "JiraTicket")
        return TimelineEventType_JiraTicket;

    // Backward compatibility - migrate to Action
    if (typeStr == "DueDate")
    {
        qDebug() << "Encountered legacy DueDate type in deserialization";

        // Auto-migrate
        return TimelineEventType_Action;
    }

    qWarning() << "Unknown event type:" << typeStr << "- defaulting to Meeting";

    return TimelineEventType_Meeting;  // Safe fallback
}
