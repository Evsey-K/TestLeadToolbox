// TimelineExporter.h


#pragma once
#include <QString>
#include <QPixmap>
#include "TimelineModel.h"


class TimelineView;
class QGraphicsScene;


/**
 * @class TimelineExporter
 * @brief Handles exporting timeline data to various formats
 *
 * Supports:
 * - Screenshot export (PNG, JPG)
 * - CSV export (event list)
 * - PDF export (formatted event list with timeline image)
 */
class TimelineExporter
{
public:
    /**
     * @brief Export timeline view as screenshot
     * @param view Timeline view to capture
     * @param filePath Output file path (extension determines format)
     * @param includeFullTimeline If true, captures entire scene; if false, captures visible area
     * @return true if export succeeded
     */
    static bool exportScreenshot(TimelineView* view,
                                 const QString& filePath,
                                 bool includeFullTimeline = true);

    /**
     * @brief Export event list to CSV file
     * @param model Timeline model containing events
     * @param filePath Output CSV file path
     * @return true if export succeeded
     */
    static bool exportToCSV(const TimelineModel* model, const QString& filePath);

    /**
     * @brief Export timeline to PDF document
     * @param model Timeline model containing events
     * @param view Timeline view for screenshot
     * @param filePath Output PDF file path
     * @param includeScreenshot If true, embeds timeline screenshot in PDF
     * @return true if export succeeded
     */
    static bool exportToPDF(const TimelineModel* model,
                            TimelineView* view,
                            const QString& filePath,
                            bool includeScreenshot = true);

    /**
     * @brief Render scene to pixmap
     * @param scene Graphics scene to render
     * @param fullScene If true, renders entire scene; if false, renders visible rect
     * @return Rendered pixmap
     */
    static QPixmap renderSceneToPixmap(QGraphicsScene* scene, bool fullScene = true);

private:
    /**
     * @brief Get CSV header row
     */
    static QString getCSVHeader();

    /**
     * @brief Convert event to CSV row
     */
    static QString eventToCSVRow(const TimelineEvent& event);

    /**
     * @brief Escape CSV field (handle quotes and commas)
     */
    static QString escapeCSVField(const QString& field);

    /**
     * @brief Convert event type to display string
     */
    static QString eventTypeToDisplayString(TimelineEventType type);
};
