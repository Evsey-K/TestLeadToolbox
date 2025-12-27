// TimelineView.h

#pragma once
#include <QGraphicsView>
#include <QPoint>
#include <QDate>

class TimelineScene;
class TimelineModel;
class TimelineCoordinateMapper;

/**
 * @class TimelineView
 * @brief Custom QGraphicsView for displaying and interacting with the timeline
 *
 * Provides:
 * - Viewport for the timeline scene
 * - Horizontal zoom via mouse wheel
 * - Left-click rubber band selection for multi-select
 * - Right-click drag-based panning
 * - Right-click context menu (when not panning)
 * - Scene management
 * - Dynamic minimum zoom calculation based on viewport width
 * - Programmatic zoom-to-fit for specific date ranges
 */
class TimelineView : public QGraphicsView {
    Q_OBJECT

public:
    /**
     * @brief Constructs a TimelineView with model and coordinate mapper
     * @param model Timeline data model
     * @param mapper Coordinate mapping utility
     * @param parent Optional parent widget
     */
    explicit TimelineView(TimelineModel* model,
                          TimelineCoordinateMapper* mapper,
                          QWidget* parent = nullptr);

    TimelineScene* timelineScene() const { return scene_; }                 ///< @brief Get the timeline scene

    double currentZoomLevel() const;                                                                ///< @brief Get the current zoom level (pixels per day)
    void zoomToFitDateRange(const QDate& startDate, const QDate& endDate, bool animate = true);     ///< @brief Zoom and center to fit a specific date range in the viewport

signals:
    void addEventRequested();                           ///< @brief User requested to add a new event via context menu
    void editEventRequested();                          ///< @brief User requested to edit selected event via context menu
    void deleteEventRequested();                        ///< @brief User requested to delete selected event(s) via context menu
    void zoomInRequested();                             ///< @brief User requested zoom in via context menu
    void zoomOutRequested();                            ///< @brief User requested zoom out via context menu
    void resetZoomRequested();                          ///< @brief User requested reset zoom via context menu
    void goToTodayRequested();                          ///< @brief User requested to jump to today via context menu
    void goToCurrentWeekRequested();                    ///< @brief User requested to jump to current week via context menu
    void goToCurrentMonthRequested();                   ///< @brief User requested to jump to current month via context menu
    void legendToggleRequested();                       ///< @brief User requested to toggle legend visibility via context menu
    void sidePanelToggleRequested();                    ///< @brief User requested to toggle side panel via context menu

public slots:
    void setLegendChecked(bool checked);                ///< @brief Update legend action checked state (called by TimelineModule)
    void setSidePanelVisible(bool visible);             ///< @brief Update side panel action text (called by TimelineModule)

protected:
    void wheelEvent(QWheelEvent* event) override;                           ///< @brief Override to implement horizontal zoom on mouse wheel
    void mousePressEvent(QMouseEvent* event) override;                      ///< @brief Override to handle right-click panning start
    void mouseMoveEvent(QMouseEvent* event) override;                       ///< @brief Override to handle right-click panning movement
    void mouseReleaseEvent(QMouseEvent* event) override;                    ///< @brief Override to handle right-click panning end and context menu
    void resizeEvent(QResizeEvent* event) override;                         ///< @brief Override to recalculate minimum zoom on viewport resize
    void scrollContentsBy(int dx, int dy) override;                         ///< @brief Override to update version name position on scroll

private:
    void updateMinimumZoom();                                               ///< @brief Calculate and set minimum zoom based on viewport and padding
    void showContextMenu(const QPoint& pos);                                ///< @brief Show right-click context menu at given position
    bool hasSelection() const;                                              ///< @brief Check if any timeline items are currently selected

    TimelineScene* scene_;
    TimelineModel* model_;
    TimelineCoordinateMapper* mapper_;
    bool isPanning_;
    QPoint lastPanPoint_;
    QPoint mousePressPos_;
    bool potentialClick_;

    // Context menu actions (stored for state updates)
    QAction* editAction_;
    QAction* deleteAction_;
    QAction* legendAction_;
    QAction* sidePanelAction_;
};
