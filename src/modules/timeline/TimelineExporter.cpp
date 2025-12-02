// TimelineExporter.cpp


#include "TimelineExporter.h"
#include "TimelineModel.h"
#include "TimelineView.h"
#include <QGraphicsScene>
#include <QPainter>
#include <QFile>
#include <QTextStream>
#include <QPrinter>
#include <QPageSize>
#include <QPageLayout>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
#include <QTextTableFormat>
#include <QDebug>
#include <QImage>


bool TimelineExporter::exportScreenshot(TimelineView* view,
                                        const QString& filePath,
                                        bool includeFullTimeline)
{
    if (!view || !view->scene())
    {
        qWarning() << "Invalid view or scene for screenshot export";
        return false;
    }

    QPixmap pixmap = renderSceneToPixmap(view->scene(), includeFullTimeline);

    if (pixmap.isNull())
    {
        qWarning() << "Failed to render scene to pixmap";
        return false;
    }

    // Determine format from file extension
    QString format = "PNG"; // Default
    if (filePath.endsWith(".jpg", Qt::CaseInsensitive) ||
        filePath.endsWith(".jpeg", Qt::CaseInsensitive))
    {
        format = "JPG";
    }

    bool success = pixmap.save(filePath, format.toUtf8().constData(), 95); // 95% quality

    if (success)
    {
        qDebug() << "Screenshot exported to:" << filePath;
    }
    else
    {
        qWarning() << "Failed to save screenshot to:" << filePath;
    }

    return success;
}


bool TimelineExporter::exportToCSV(const TimelineModel* model, const QString& filePath)
{
    if (!model)
    {
        qWarning() << "Cannot export null model to CSV";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open CSV file for writing:" << filePath;
        return false;
    }

    QTextStream out(&file);

    // Write UTF-8 BOM for Excel compatibility
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";

    // Write header
    out << getCSVHeader() << "\n";

    // Write all events
    const auto& events = model->getAllEvents();

    for (const auto& event : events)
    {
        out << eventToCSVRow(event) << "\n";
    }

    file.close();

    qDebug() << "CSV exported to:" << filePath << "(" << events.size() << "events)";

    return true;
}


bool TimelineExporter::exportToPDF(const TimelineModel* model,
                                   TimelineView* view,
                                   const QString& filePath,
                                   bool includeScreenshot)
{
    if (!model)
    {
        qWarning() << "Cannot export null model to PDF";
        return false;
    }

    // Create PDF printer
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize::A4);
    printer.setPageOrientation(QPageLayout::Portrait);

    // Create document
    QTextDocument document;
    QTextCursor cursor(&document);

    // Set document-wide font
    QFont baseFont("Arial", 10);
    document.setDefaultFont(baseFont);

    // Title
    QTextCharFormat titleFormat;
    titleFormat.setFontPointSize(18);
    titleFormat.setFontWeight(QFont::Bold);
    cursor.insertText("Timeline Report\n\n", titleFormat);

    // Metadata
    QTextCharFormat metaFormat;
    metaFormat.setFontPointSize(10);
    cursor.insertText(QString("Generated: %1\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")), metaFormat);
    cursor.insertText(QString("Version Period: %1 to %2\n")
                          .arg(model->versionStartDate().toString("yyyy-MM-dd"))
                          .arg(model->versionEndDate().toString("yyyy-MM-dd")), metaFormat);
    cursor.insertText(QString("Total Events: %1\n\n").arg(model->eventCount()), metaFormat);

    // Optional: Include screenshot
    if (includeScreenshot && view)
    {
        QTextCharFormat sectionFormat;
        sectionFormat.setFontPointSize(14);
        sectionFormat.setFontWeight(QFont::Bold);
        cursor.insertText("Timeline Visualization\n\n", sectionFormat);

        QPixmap screenshot = renderSceneToPixmap(view->scene(), true);
        if (!screenshot.isNull())
        {
            // Scale screenshot to fit page width
            QImage scaledImage = screenshot.toImage().scaledToWidth(
                700, Qt::SmoothTransformation);

            cursor.insertImage(scaledImage);
            cursor.insertText("\n\n");
        }
    }

    // Event list section
    QTextCharFormat sectionFormat;
    sectionFormat.setFontPointSize(14);
    sectionFormat.setFontWeight(QFont::Bold);
    cursor.insertText("Event List\n\n", sectionFormat);

    // Create table
    QTextTableFormat tableFormat;
    tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    tableFormat.setCellPadding(4);
    tableFormat.setCellSpacing(0);

    const auto& events = model->getAllEvents();
    QTextTable* table = cursor.insertTable(events.size() + 1, 5, tableFormat); // +1 for header

    // Table header
    QTextCharFormat headerFormat;
    headerFormat.setFontWeight(QFont::Bold);
    headerFormat.setBackground(QBrush(QColor(200, 200, 200)));

    QStringList headers = {"Title", "Type", "Start Date", "End Date", "Priority"};

    for (int col = 0; col < headers.size(); ++col)
    {
        QTextTableCell cell = table->cellAt(0, col);
        QTextCursor cellCursor = cell.firstCursorPosition();
        cellCursor.setCharFormat(headerFormat);
        cellCursor.insertText(headers[col]);
    }

    // Table rows
    for (int row = 0; row < events.size(); ++row)
    {
        const TimelineEvent& event = events[row];

        // Title
        table->cellAt(row + 1, 0).firstCursorPosition().insertText(event.title);

        // Type
        table->cellAt(row + 1, 1).firstCursorPosition().insertText(
            eventTypeToDisplayString(event.type));

        // Start Date
        table->cellAt(row + 1, 2).firstCursorPosition().insertText(
            event.startDate.toString("yyyy-MM-dd"));

        // End Date
        table->cellAt(row + 1, 3).firstCursorPosition().insertText(
            event.endDate.toString("yyyy-MM-dd"));

        // Priority
        table->cellAt(row + 1, 4).firstCursorPosition().insertText(
            QString::number(event.priority));
    }

    // Print document to PDF
    document.print(&printer);

    qDebug() << "PDF exported to:" << filePath;

    return true;
}


QPixmap TimelineExporter::renderSceneToPixmap(QGraphicsScene* scene, bool fullScene)
{
    if (!scene)
    {
        return QPixmap();
    }

    QRectF renderRect = fullScene ? scene->sceneRect() : scene->itemsBoundingRect();

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
    return "ID,Title,Type,Start Date,End Date,Duration (days),Priority,Lane,Description";
}


QString TimelineExporter::eventToCSVRow(const TimelineEvent& event)
{
    QStringList fields;

    fields << escapeCSVField(event.id);
    fields << escapeCSVField(event.title);
    fields << escapeCSVField(eventTypeToDisplayString(event.type));
    fields << event.startDate.toString("yyyy-MM-dd");
    fields << event.endDate.toString("yyyy-MM-dd");
    fields << QString::number(event.startDate.daysTo(event.endDate) + 1);
    fields << QString::number(event.priority);
    fields << QString::number(event.lane);
    fields << escapeCSVField(event.description);

    return fields.join(",");
}


QString TimelineExporter::escapeCSVField(const QString& field)
{
    // If field contains comma, quote, or newline, wrap in quotes and escape quotes
    if (field.contains(',') || field.contains('"') || field.contains('\n'))
    {
        QString escaped = field;
        escaped.replace("\"", "\"\""); // Escape quotes by doubling them
        return "\"" + escaped + "\"";
    }

    return field;
}


QString TimelineExporter::eventTypeToDisplayString(TimelineEventType type)
{
    switch (type)
    {
    case TimelineEventType_Meeting: return "Meeting";
    case TimelineEventType_Action: return "Action";
    case TimelineEventType_TestEvent: return "Test Event";
    case TimelineEventType_DueDate: return "Due Date";
    case TimelineEventType_Reminder: return "Reminder";

    default: return "Unknown";
    }
}
