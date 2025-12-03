// TimelineItem.h


#pragma once
#include <QObject>
#include <QGraphicsRectItem>
#include <QBrush>
#include <QString>


class TimelineModel;
class TimelineCoordinateMapper;


/**
 * @class TimelineItem
 * @brief Represents a single draggable and resizable event item on the timeline
 *
 * This item:
 * - Displays an event as a colored rectangle
 * - Can be dragged horizontally to change dates
 * - Can be resized by dragging edges (for multi-day events)
 * - Updates the model when dragging or resizing completes
 * - Supports selection highlighting
 * - Shows appropriate cursors for resize handles
 */
class TimelineItem : public QGraphicsRectItem
{
    Q_OBJECT

public:

    /**
     * @brief Constructs a TimelineItem with the specified rectangle geometry
     * @param rect Initial rectangle (position and size)
     * @param parent Optional parent graphics item
     */
    explicit TimelineItem(const QRectF& rect, QGraphicsItem* parent = nullptr);

    void setEventId(const QString& eventId) { eventId_ = eventId; }                     ///< @brief Set the event ID this item represents
    QString eventId() const { return eventId_; }                                        ///< @brief Get the event ID
    void setModel(TimelineModel* model) { model_ = model; }                             ///< @brief Set the model reference (required for updates)
    void setCoordinateMapper(TimelineCoordinateMapper* mapper) { mapper_ = mapper; }    ///< @brief Set the coordinate mapper (required for date conversion)
    void setResizable(bool resizable) { resizable_ = resizable; }                       ///< @brief Enable or disable resize functionality
    bool isResizable() const { return resizable_; }                                     ///< @brief Check if item is resizable

signals:
    void editRequested(const QString& eventId);
    void deleteRequested(const QString& eventId);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;     ///< @brief Override to handle drag completion and update model
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;                     ///< @brief Track when drag starts
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;                      ///< @brief Handle mouse move for dragging and resizing
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;                   ///< @brief Track when drag ends
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;                      ///< @brief Update cursor based on mouse position (for resize handles)
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;                     ///< @brief Restore cursor when leaving item
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;              ///< @brief

private:
    void updateModelFromPosition();     ///< @brief Update the model with new dates based on current position
    void updateModelFromSize();         ///< @brief Update the model with new dates based on current size (after resize)

    enum ResizeHandle { None, LeftEdge, RightEdge };            ///< @brief Detect which edge/corner is under mouse cursor
    ResizeHandle getResizeHandle(const QPointF& pos) const;     ///< @brief Get resize handle at given position
    void updateCursor(ResizeHandle handle);                     ///< @brief Update cursor based on resize handle

    QString eventId_;                                       ///< ID of the event this item represents
    TimelineModel* model_ = nullptr;                        ///< Model reference (not owned)
    TimelineCoordinateMapper* mapper_ = nullptr;            ///< Coordinate mapper (not owned)
    QPointF dragStartPos_;                                  ///< Position when drag started
    QRectF resizeStartRect_;                                ///< Rectangle when resize started
    bool isDragging_ = false;                               ///< Whether currently being dragged
    bool isResizing_ = false;                               ///< Whether currently being resized
    bool resizable_ = true;                                 ///< Whether item can be resized
    ResizeHandle activeHandle_ = None;                      ///< Currently active resize handle

    static constexpr double RESIZE_HANDLE_WIDTH = 8.0;      ///< Width of resize hit area
};
