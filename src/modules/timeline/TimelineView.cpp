// TimelineView.cpp


#include "TimelineView.h"
#include "TimelineScene.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include <QWheelEvent>


TimelineView::TimelineView(TimelineModel* model,
                           TimelineCoordinateMapper* mapper,
                           QWidget* parent)
    : QGraphicsView(parent)
    , mapper_(mapper)
{
    // Create the scene with model and mapper
    scene_ = new TimelineScene(model, mapper, this);
    setScene(scene_);

    // Enable anti-aliasing for smooth rendering
    setRenderHint(QPainter::Antialiasing, true);

    // Disable vertical scroll bar
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Enable horizontal scroll bar
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Enable hand drag mode for panning
    setDragMode(QGraphicsView::ScrollHandDrag);

    // Set background color
    setBackgroundBrush(QBrush(QColor(250, 250, 250)));
}


void TimelineView::wheelEvent(QWheelEvent* event)
{
    // Check if Ctrl key is pressed for zoom
    if (event->modifiers() & Qt::ControlModifier)
    {
        // Define zoom factor
        const double zoomFactor = 1.15;

        // Determine zoom direction
        if (event->angleDelta().y() > 0)
        {
            // Zoom in
            mapper_->zoom(zoomFactor);
            scale(zoomFactor, 1.0);
        }
        else
        {
            // Zoom out
            mapper_->zoom(1.0 / zoomFactor);
            scale(1.0 / zoomFactor, 1.0);
        }

        // Rebuild scene with new scale
        scene_->rebuildFromModel();

        event->accept();
    }
    else
    {
        // Normal scroll behavior (horizontal pan)
        QGraphicsView::wheelEvent(event);
    }
}
