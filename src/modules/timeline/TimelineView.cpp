// TimelineView.cpp
#include "TimelineView.h"
#include "TimelineScene.h"
#include <QWheelEvent>

TimelineView::TimelineView(QWidget *parent) : QGraphicsView(parent) {
    // Instantiate the TimelineScene and assign it to this view
    scene_ = new TimelineScene(this);
    setScene(scene_);

    // Enable anti-aliasing for smooth graphical rendering
    setRenderHint(QPainter::Antialiasing, true);

    // Disable vertical scroll bar; prefer horizontal scrolling only
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Enable hand drag mode for intuitive panning with mouse drag
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void TimelineView::wheelEvent(QWheelEvent *event) {
    // Define zoom scaling factor for horizontal zoom
    const double scaleFactor = 1.15;

    // Zoom in or out horizontally based on wheel scroll direction
    if (event->angleDelta().y() > 0) {
        // Scroll up → zoom in horizontally
        scale(scaleFactor, 1.0);
    } else {
        // Scroll down → zoom out horizontally
        scale(1.0 / scaleFactor, 1.0);
    }

    // Note: Vertical scaling remains fixed at 1.0 to prevent vertical zoom
}
