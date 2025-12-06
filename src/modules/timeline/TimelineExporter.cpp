// TimelineExporter.cpp


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
#include <QPrinter>
#include <QPageLayout>
#include <QPageSize>
#include <QDateTime>


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


bool TimelineExporter::exportScreenshot(TimelineView* view,
                                        const QString& filePath,
                                        bool includeFullTimeline)
{
    if (!view || !view->scene())
    {
        return false;
    }

    QPixmap pixmap = renderSceneToPixmap(view->scene(), includeFullTimeline);

    if (pixmap.isNull())
    {
        return false;
    }

    return pixmap.save(filePath);
}


bool TimelineExporter::exportEventsToCSV(const QVector<TimelineEvent>& events,
                                         const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&file);

    // Write CSV header
    out << getCSVHeader() << "\n";

    // Write event rows
    for (const TimelineEvent& event : events)
    {
        out << eventToCSVRow(event) << "\n";
    }

    file.close();

    return true;
}


bool TimelineExporter::exportToPDF(const TimelineModel* model,
                                   TimelineView* view,
                                   const QString& filePath,
                                   bool includeScreenshot)
{
    if (!model)
    {
        return false;
    }

    QVector<TimelineEvent> events = model->getAllEvents();
    QString title = QString("Timeline Report (%1 events)").arg(events.size());

    return exportEventsToPDF(events, view, filePath, includeScreenshot, title);
}


bool TimelineExporter::exportEventsToPDF(const QVector<TimelineEvent>& events,
                                         TimelineView* view,
                                         const QString& filePath,
                                         bool includeScreenshot,
                                         const QString& reportTitle)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize::A4);
    printer.setPageOrientation(QPageLayout::Portrait);

    QPainter painter;
    if (!painter.begin(&printer))
    {
        return false;
    }

    QFont titleFont("Arial", 16, QFont::Bold);
    QFont headerFont("Arial", 10, QFont::Bold);
    QFont bodyFont("Arial", 9);

    int y = 100;
    const int margin = 100;

    // Qt 6: Use pageLayout() to get page dimensions
    QRectF pageRect = printer.pageLayout().paintRectPixels(printer.resolution());
    const int pageWidth = pageRect.width() - 2 * margin;
    const int pageHeight = pageRect.height();

    // Draw title
    painter.setFont(titleFont);
    painter.drawText(margin, y, reportTitle);
    y += 80;

    // Draw generation date
    painter.setFont(bodyFont);
    QString dateStr = QString("Generated: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    painter.drawText(margin, y, dateStr);
    y += 60;

    // Include screenshot if requested
    if (includeScreenshot && view && view->scene())
    {
        QPixmap screenshot = renderSceneToPixmap(view->scene(), true);
        if (!screenshot.isNull())
        {
            // Scale screenshot to fit page width
            QPixmap scaled = screenshot.scaledToWidth(pageWidth, Qt::SmoothTransformation);

            // Check if we need a new page
            if (y + scaled.height() > pageHeight - margin)
            {
                printer.newPage();
                y = margin;
            }

            painter.drawPixmap(margin, y, scaled);
            y += scaled.height() + 60;
        }
    }

    // Draw events table
    if (!events.isEmpty())
    {
        // Check if we need a new page
        if (y + 200 > pageHeight - margin)
        {
            printer.newPage();
            y = margin;
        }

        painter.setFont(headerFont);
        painter.drawText(margin, y, QString("Events Summary (%1 total)").arg(events.size()));
        y += 40;

        // Draw table header
        painter.drawLine(margin, y, margin + pageWidth, y);
        y += 5;

        painter.setFont(headerFont);
        painter.drawText(margin, y, "Title");
        painter.drawText(margin + 250, y, "Type");
        painter.drawText(margin + 380, y, "Date Range");
        painter.drawText(margin + 530, y, "Priority");
        y += 25;
        painter.drawLine(margin, y, margin + pageWidth, y);
        y += 15;

        // Draw events
        painter.setFont(bodyFont);
        for (const TimelineEvent& event : events)
        {
            // Check if we need a new page
            if (y + 30 > pageHeight - margin)
            {
                printer.newPage();
                y = margin;

                // Redraw header on new page
                painter.setFont(headerFont);
                painter.drawLine(margin, y, margin + pageWidth, y);
                y += 5;
                painter.drawText(margin, y, "Title");
                painter.drawText(margin + 250, y, "Type");
                painter.drawText(margin + 380, y, "Date Range");
                painter.drawText(margin + 530, y, "Priority");
                y += 25;
                painter.drawLine(margin, y, margin + pageWidth, y);
                y += 15;
                painter.setFont(bodyFont);
            }

            // Draw event row
            QString title = event.title;
            if (title.length() > 30)
            {
                title = title.left(27) + "...";
            }

            painter.drawText(margin, y, title);
            painter.drawText(margin + 250, y, eventTypeToDisplayString(event.type));

            QString dateRange;
            if (event.startDate == event.endDate)
            {
                dateRange = event.startDate.toString("MMM dd");
            }
            else
            {
                dateRange = QString("%1 - %2")
                .arg(event.startDate.toString("MMM dd"))
                    .arg(event.endDate.toString("MMM dd"));
            }
            painter.drawText(margin + 380, y, dateRange);
            painter.drawText(margin + 530, y, QString::number(event.priority));

            y += 25;
        }
    }

    painter.end();
    return true;
}
