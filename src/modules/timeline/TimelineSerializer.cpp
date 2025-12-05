// TimelineSerializer.cpp


#include "TimelineSerializer.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDebug>


bool TimelineSerializer::saveToFile(const TimelineModel* model, const QString& filePath)
{
    if (!model)
    {
        qWarning() << "Cannot save null model";

        return false;
    }

    // Create backup of existing file
    if (QFile::exists(filePath))
    {
        createBackup(filePath);
    }

    // Serialize model to JSON
    QJsonObject jsonObj = serializeModel(model);
    QJsonDocument doc(jsonObj);

    // Write to file
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Failed to open file for writing:" << filePath;

        return false;
    }

    qint64 bytesWritten = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (bytesWritten == -1)
    {
        qWarning() << "Failed to write JSON to file";

        return false;
    }

    qDebug() << "Timeline saved successfully to" << filePath;

    return true;
}


bool TimelineSerializer::loadFromFile(TimelineModel* model, const QString& filePath)
{
    if (!model)
    {
        qWarning() << "Cannot load into null model";

        return false;
    }

    // Read file
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open file for reading:" << filePath;

        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "JSON parse error:" << parseError.errorString();

        return false;
    }

    if (!doc.isObject())
    {
        qWarning() << "JSON root is not an object";

        return false;
    }

    // Deserialize into model
    bool success = deserializeModel(model, doc.object());

    if (success)
    {
        qDebug() << "Timeline loaded successfully from" << filePath;
    }

    return success;
}


QJsonObject TimelineSerializer::serializeModel(const TimelineModel* model)
{
    QJsonObject root;

    // Metadata
    root["version"] = "1.0";
    root["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Version dates
    root["versionStartDate"] = model->versionStartDate().toString(Qt::ISODate);
    root["versionEndDate"] = model->versionEndDate().toString(Qt::ISODate);

    // Serialize active events
    QJsonArray eventsArray;
    const auto& events = model->getAllEvents();

    for (const auto& event : events)
    {
        eventsArray.append(serializeEvent(event));
    }

    root["events"] = eventsArray;

    // Serialize archived events
    QJsonArray archivedArray;
    const auto& archivedEvents = model->getAllArchivedEvents();

    for (const auto& event : archivedEvents)
    {
        archivedArray.append(serializeEvent(event));
    }

    root["archivedEvents"] = archivedArray;

    return root;
}


bool TimelineSerializer::deserializeModel(TimelineModel* model, const QJsonObject& json)
{
    // Clear existing model data
    model->clear();

    // Version check
    QString version = json["version"].toString();
    if (version != "1.0")
    {
        qWarning() << "Unsupported timeline file version:" << version;
    }

    // Load version dates
    QString startDateStr = json["versionStartDate"].toString();
    QString endDateStr = json["versionEndDate"].toString();

    QDate startDate = QDate::fromString(startDateStr, Qt::ISODate);
    QDate endDate = QDate::fromString(endDateStr, Qt::ISODate);

    if (!startDate.isValid() || !endDate.isValid())
    {
        qWarning() << "Invalid version dates in JSON";
        return false;
    }

    model->setVersionDates(startDate, endDate);

    // Load active events
    QJsonArray eventsArray = json["events"].toArray();

    for (const QJsonValue& eventValue : eventsArray)
    {
        if (!eventValue.isObject())
        {
            qWarning() << "Skipping invalid event entry";
            continue;
        }

        TimelineEvent event = deserializeEvent(eventValue.toObject());

        if (!event.id.isEmpty())
        {
            model->addEvent(event);
        }
    }

    // NEW: Load archived events
    QJsonArray archivedArray = json["archivedEvents"].toArray();

    for (const QJsonValue& eventValue : archivedArray)
    {
        if (!eventValue.isObject())
        {
            qWarning() << "Skipping invalid archived event entry";
            continue;
        }

        TimelineEvent event = deserializeEvent(eventValue.toObject());

        if (!event.id.isEmpty())
        {
            // Add as active event first
            QString addedId = model->addEvent(event);

            // Then archive it
            if (!addedId.isEmpty())
            {
                model->archiveEvent(addedId);
            }
        }
    }

    return true;
}


QString TimelineSerializer::getDefaultSaveLocation()
{
    // Use application data directory
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // Create directory if it doesn't exist
    QDir dir;

    if (!dir.exists(appDataPath))
    {
        dir.mkpath(appDataPath);
    }

    return appDataPath + "/timeline_data.json";
}


bool TimelineSerializer::createBackup(const QString& filePath)
{
    if (!QFile::exists(filePath))
    {
        return true;    // Nothing to backup
    }

    // Create backup filename with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd__HHmmss");
    QString backupPath = filePath + ".backup_" + timestamp;

    // Copy file
    bool success = QFile::copy(filePath, backupPath);

    if (success)
    {
        qDebug() << "Backup created:" << backupPath;
    }
    else
    {
        qWarning() << "Failed to create backup";
    }

    return success;
}


QJsonObject TimelineSerializer::serializeEvent(const TimelineEvent& event)
{
    QJsonObject obj;

    obj["id"] = event.id;
    obj["type"] = eventTypeToString(event.type);
    obj["title"] = event.title;
    obj["description"] = event.description;
    obj["startDate"] = event.startDate.toString(Qt::ISODate);
    obj["endDate"] = event.endDate.toString(Qt::ISODate);
    obj["priority"] = event.priority;
    obj["lane"] = event.lane;
    obj["archived"] = event.archived;

    // Store color as hex string
    obj["color"] = event.color.name();

    return obj;
}


TimelineEvent TimelineSerializer::deserializeEvent(const QJsonObject& json)
{
    TimelineEvent event;

    event.id = json["id"].toString();
    event.type = stringToEventType(json["type"].toString());
    event.title = json["title"].toString();
    event.description = json["description"].toString();

    QString startDateStr = json["startDate"].toString();
    QString endDateStr = json["endDate"].toString();

    event.startDate = QDate::fromString(startDateStr, Qt::ISODate);
    event.endDate = QDate::fromString(endDateStr, Qt::ISODate);

    event.priority = json["priority"].toInt();
    event.lane = json["lane"].toInt();
    event.archived = json["archived"].toBool();

    // Parse color
    QString colorStr = json["color"].toString();

    if (!colorStr.isEmpty())
    {
        event.color = QColor(colorStr);
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
    case TimelineEventType_DueDate:
        return "DueDate";
    case TimelineEventType_Reminder:
        return "Reminder";

    default: return "Unknown";
    }
}


TimelineEventType TimelineSerializer::stringToEventType(const QString& typeStr)
{
    if (typeStr == "Meeting")
    {
        return TimelineEventType_Meeting;
    }
    if (typeStr == "Action")
    {
        return TimelineEventType_Action;
    }
    if (typeStr == "TestEvent")
    {
        return TimelineEventType_TestEvent;
    }
    if (typeStr == "DueDate")
    {
        return TimelineEventType_DueDate;
    }
    if (typeStr == "Reminder")
    {
        return TimelineEventType_Reminder;
    }

    qWarning() << "Unknown event type:" << typeStr;

    return TimelineEventType_Meeting; // Default fallback
}















