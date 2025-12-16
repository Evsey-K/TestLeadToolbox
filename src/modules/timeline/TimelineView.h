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

protected:
    void wheelEvent(QWheelEvent* event) override;                           ///< @brief Override to implement horizontal zoom on mouse wheel
    void mousePressEvent(QMouseEvent* event) override;                      ///< @brief Override to handle right-click panning start
    void mouseMoveEvent(QMouseEvent* event) override;                       ///< @brief Override to handle right-click panning movement
    void mouseReleaseEvent(QMouseEvent* event) override;                    ///< @brief Override to handle right-click panning end
    void resizeEvent(QResizeEvent* event) override;                         ///< @brief Override to recalculate minimum zoom on viewport resize

private:
    void updateMinimumZoom();                                               ///< @brief Calculate and set minimum zoom based on viewport and padding

    TimelineScene* scene_;
    TimelineModel* model_;
    TimelineCoordinateMapper* mapper_;
    bool isPanning_;
    QPoint lastPanPoint_;
    QPoint mousePressPos_;
    bool potentialClick_;
};
