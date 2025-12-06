// TimelineExporter.cpp - Updated with Type-Specific Field Support

#include "TimelineExporter.h"
#include "TimelineScene.h"
#include "TimelineView.h"
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

bool TimelineExporter::exportToCSV(const TimelineModel* model, const QString& filePath)
{
    if (!model)
    {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&file);

    // Write CSV header
    out << getCSVHeader() << "\n";

    // Write event rows
    QVector<TimelineEvent> events = model->getAllEvents();
    for (const TimelineEvent& event : events)
    {
        out << eventToCSVRow(event) << "\n";
    }

    file.close();
    return true;
}

QPixmap TimelineExporter::renderSceneToPixmap(QGraphicsScene* scene, bool fullScene)
{
    if (!scene)
    {
        return QPixmap();
    }

    // Determine render area
    QRectF renderRect = fullScene ?
                            scene->sceneRect() : scene->itemsBoundingRect();

    // Create pixmap with appropriate size
    QPixmap pixmap(renderRect.size().toSize());
    pixmap.fill(Qt::white);

    // Render scene to pixmap
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    scene->render(&painter, QRectF(), renderRect);
    painter.end();

    return pixmap;
}

QString TimelineExporter::getCSVHeader()
{
    // Extended CSV header to support type-specific fields
    return "ID,Title,Type,Start Date,End Date,Duration (days),Priority,Lane,"
           "Start Time,End Time,Location,Participants,"  // Meeting
           "Due DateTime,Status,"                          // Action
           "Test Category,Checklist Progress,"            // Test Event
           "Reminder DateTime,Recurring Rule,"            // Reminder
           "Jira Key,Jira Type,Jira Status,"              // Jira Ticket
           "Description";
}

QString TimelineExporter::eventToCSVRow(const TimelineEvent& event)
{
    QStringList fields;

    // === COMMON FIELDS ===
    fields << escapeCSVField(event.id);
    fields << escapeCSVField(event.title);
    fields << escapeCSVField(eventTypeToDisplayString(event.type));
    fields << event.startDate.toString("yyyy-MM-dd");
    fields << event.endDate.toString("yyyy-MM-dd");
    fields << QString::number(event.startDate.daysTo(event.endDate) + 1);
    fields << QString::number(event.priority);
    fields << QString::number(event.lane);

    // === TYPE-SPECIFIC FIELDS (in order matching header) ===

    // Meeting-specific fields
    if (event.type == TimelineEventType_Meeting)
    {
        fields << event.startTime.toString("HH:mm");
        fields << event.endTime.toString("HH:mm");
        fields << escapeCSVField(event.location);
        fields << escapeCSVField(event.participants);
    }
    else
    {
        fields << "" << "" << "" << "";  // Empty Meeting fields
    }

    // Action-specific fields
    if (event.type == TimelineEventType_Action)
    {
        fields << event.dueDateTime.toString("yyyy-MM-dd HH:mm");
        fields << escapeCSVField(event.status);
    }
    else
    {
        fields << "" << "";  // Empty Action fields
    }

    // Test Event-specific fields
    if (event.type == TimelineEventType_TestEvent)
    {
        fields << escapeCSVField(event.testCategory);

        // Calculate checklist progress
        if (!event.preparationChecklist.isEmpty())
        {
            int completed = 0;
            for (auto it = event.preparationChecklist.begin();
                 it != event.preparationChecklist.end(); ++it)
            {
                if (it.value())
                    completed++;
            }
            int total = event.preparationChecklist.size();
            QString progress = QString("%1/%2 (%3%)")
                                   .arg(completed)
                                   .arg(total)
                                   .arg((completed * 100) / total);
            fields << escapeCSVField(progress);
        }
        else
        {
            fields << "";
        }
    }
    else
    {
        fields << "" << "";  // Empty Test Event fields
    }

    // Reminder-specific fields
    if (event.type == TimelineEventType_Reminder)
    {
        fields << event.reminderDateTime.toString("yyyy-MM-dd HH:mm");
        fields << escapeCSVField(event.recurringRule);
    }
    else
    {
        fields << "" << "";  // Empty Reminder fields
    }

    // Jira Ticket-specific fields
    if (event.type == TimelineEventType_JiraTicket)
    {
        fields << escapeCSVField(event.jiraKey);
        fields << escapeCSVField(event.jiraType);
        fields << escapeCSVField(event.jiraStatus);
    }
    else
    {
        fields << "" << "" << "";  // Empty Jira fields
    }

    // === DESCRIPTION (Common) ===
    fields << escapeCSVField(event.description);

    return fields.join(",");
}

QString TimelineExporter::escapeCSVField(const QString& field)
{
    // If field contains comma, quote, or newline, wrap in quotes and escape quotes
    if (field.contains(',') || field.contains('"') || field.contains('\n'))
    {
        QString escaped = field;
        escaped.replace("\"", "\"\"");  // Escape quotes by doubling them
        return "\"" + escaped + "\"";
    }

    return field;
}

QString TimelineExporter::eventTypeToDisplayString(TimelineEventType type)
{
    switch (type)
    {
    case TimelineEventType_Meeting:
        return "Meeting";
    case TimelineEventType_Action:
        return "Action";
    case TimelineEventType_TestEvent:
        return "Test Event";
    case TimelineEventType_Reminder:
        return "Reminder";
    case TimelineEventType_JiraTicket:
        return "Jira Ticket";
    default:
        return "Unknown";
    }
}
