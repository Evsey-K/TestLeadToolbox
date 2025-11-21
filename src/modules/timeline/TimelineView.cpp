#include "TimelineView.h"
#include "TimelineScene.h"
#include <QWheelEvent>

TimelineView::TimelineView(QWidget *parent) : QGraphicsView(parent) {
    scene_ = new TimelineScene(this);
    setScene(scene_);

    // Enable anti-aliasing for nicer rendering
    setRenderHint(QPainter::Antialiasing, true);

    // Horizontal scroll only preferred
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void TimelineView::wheelEvent(QWheelEvent *event) {
    // Zoom horizontally only
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, 1.0);
    } else {
        scale(1.0/scaleFactor, 1.0);
    }
}
