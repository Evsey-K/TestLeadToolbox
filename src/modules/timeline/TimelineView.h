// TimelineView.h


#pragma once
#include <QGraphicsView>
#include <QPoint>


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

    /**
     * @brief Get the timeline scene
     */
    TimelineScene* timelineScene() const { return scene_; }

protected:
    /**
     * @brief Override to implement horizontal zoom on mouse wheel
     */
    void wheelEvent(QWheelEvent* event) override;

    /**
     * @brief Override to handle right-click panning start
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief Override to handle right-click panning movement
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief Override to handle right-click panning end
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    TimelineScene* scene_;
    TimelineCoordinateMapper* mapper_;
    bool isPanning_;
    QPoint lastPanPoint_;
};
