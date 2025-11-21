// TimelineView.h
#pragma once
#include <QGraphicsView>

// Forward declaration to reduce header dependencies
class TimelineScene;

/**
 * @class TimelineView
 * @brief Custom QGraphicsView subclass to display and interact with the timeline scene.
 *
 * TimelineView provides the viewport for the timeline graphics scene (TimelineScene),
 * enabling features such as horizontal zooming and drag-based panning.
 */
class TimelineView : public QGraphicsView {
    Q_OBJECT
public:
    /**
     * @brief Constructs a TimelineView instance with an optional parent widget.
     * @param parent Pointer to parent widget, defaults to nullptr.
     */
    explicit TimelineView(QWidget *parent = nullptr);

protected:
    /**
     * @brief Overridden to implement horizontal zoom behavior on mouse wheel events.
     * @param event Pointer to the wheel event.
     */
    void wheelEvent(QWheelEvent *event) override;

private:
    TimelineScene *scene_ = nullptr; ///< The custom graphics scene displayed in this view.
};
