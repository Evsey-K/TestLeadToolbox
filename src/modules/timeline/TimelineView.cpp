// TimelineView.cpp


#include "TimelineView.h"
#include "TimelineScene.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>


TimelineView::TimelineView(TimelineModel* model,
                           TimelineCoordinateMapper* mapper,
                           QWidget* parent)
    : QGraphicsView(parent)
    , mapper_(mapper)
    , isPanning_(false)
{
    // Create the scene with model and mapper
    scene_ = new TimelineScene(model, mapper, this);
    setScene(scene_);

    // Enable anti-aliasing for smooth rendering
    setRenderHint(QPainter::Antialiasing, true);

    // Disable vertical scroll bar
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Enable horizontal scroll bar
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Enable rubber band selection for left-click drag
    setDragMode(QGraphicsView::RubberBandDrag);

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

        // Get the mouse position in widget coordinates
        QPointF widgetPos = event->position();

        // Convert to scene coordinates before zoom
        QPointF scenePosBefore = mapToScene(widgetPos.toPoint());

        // Determine zoom direction and apply zoom
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

        // Convert same widget position to scene coordinates after zoom
        QPointF scenePosAfter = mapToScene(widgetPos.toPoint());

        // Calculate the difference and adjust the scroll bars
        QPointF delta = scenePosAfter - scenePosBefore;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() + delta.y());

        event->accept();
    }
    else if (event->modifiers() & Qt::ShiftModifier)
    {
        // Shift + Mouse wheel: Vertical scrolling
        int delta = event->angleDelta().y();
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        event->accept();
    }
    else
    {
        // Default: Horizontal scrolling (no modifiers)
        int delta = event->angleDelta().y();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta);
        event->accept();
    }
}

void TimelineView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        // Start right-click panning
        isPanning_ = true;
        lastPanPoint_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    // For left-click, use default behavior (rubber band selection)
    QGraphicsView::mousePressEvent(event);
}

void TimelineView::mouseMoveEvent(QMouseEvent* event)
{
    if (isPanning_)
    {
        // Calculate the delta movement
        QPoint delta = event->pos() - lastPanPoint_;
        lastPanPoint_ = event->pos();

        // Pan the view by adjusting scroll bars
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());

        event->accept();
        return;
    }

    // For left-click, use default behavior (rubber band selection)
    QGraphicsView::mouseMoveEvent(event);
}

void TimelineView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton && isPanning_)
    {
        // End right-click panning
        isPanning_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }

    // For left-click, use default behavior (rubber band selection)
    QGraphicsView::mouseReleaseEvent(event);
}
