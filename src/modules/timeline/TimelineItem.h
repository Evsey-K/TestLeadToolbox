// TimelineItem.h


#pragma once
#include <QGraphicsObject>
#include <QBrush>
#include <QPen>
#include <QString>
#include <QRectF>
#include <QMap>


class TimelineModel;
class TimelineCoordinateMapper;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;
class QMenu;
class QUndoStack;


/**
 * @class TimelineItem
 * @brief A custom graphics item representing a timeline event
 *
 * Supports:
 * - Dragging horizontally to change dates
 * - Dragging vertically when lane control is enabled to change lane
 * - Multi-item selection and dragging with individual lane control per item
 * - Resizing via edge handles
 * - Context menu for edit/delete actions
 * - Automatic overlap prevention via model's lane assignment system
 */
class TimelineItem : public QGraphicsObject
{
    Q_OBJECT

public:

    /**
     * @brief Constructs a TimelineItem with the specified rectangle geometry
     * @param rect Initial rectangle (position and size)
     * @param parent Optional parent graphics item
     */
    explicit TimelineItem(const QRectF& rect, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;                                                                           ///< @brief Required by QGraphicsItem - returns bounding rectangle
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;      ///< @brief Required by QGraphicsItem - paints the item

    // Rectangle management (replacing QGraphicsRectItem methods)
    void setRect(const QRectF& rect);
    QRectF rect() const { return rect_; }

    // Brush and pen (for styling)
    void setBrush(const QBrush& brush) { brush_ = brush; update(); }
    QBrush brush() const { return brush_; }
    void setPen(const QPen& pen) { pen_ = pen; update(); }
    QPen pen() const { return pen_; }

    void setEventId(const QString& eventId) { eventId_ = eventId; }                     ///< @brief Set the event ID this item represents
    QString eventId() const { return eventId_; }                                        ///< @brief Get the event ID
    void setModel(TimelineModel* model) { model_ = model; }                             ///< @brief Set the model reference (required for updates)
    void setCoordinateMapper(TimelineCoordinateMapper* mapper) { mapper_ = mapper; }    ///< @brief Set the coordinate mapper (required for date conversion)
    void setUndoStack(QUndoStack* undoStack) { undoStack_ = undoStack; }                ///< @brief Set the undo stack (required for undo/redo support)
    void setResizable(bool resizable) { resizable_ = resizable; }                       ///< @brief Enable or disable resize functionality
    bool isResizable() const { return resizable_; }                                     ///< @brief Check if item is resizable
    void setSkipNextUpdate(bool skip) { skipNextUpdate_ = skip; }                       ///< @brief Set whether to skip the next model update (used during undo/redo)
    bool shouldSkipNextUpdate() const { return skipNextUpdate_; }                       ///< @brief Check if next update should be skipped

    void setAttachmentCount(int count);                                                 ///<
    int attachmentCount() const { return attachmentCount_; }                            ///<
    bool hasAttachments() const { return attachmentCount_ > 0; }                        ///<

signals:
    void editRequested(const QString& eventId);
    void deleteRequested(const QString& eventId);
    void clicked(const QString& eventId);
    void filesDropped(const QString& eventId, const QStringList& filePaths);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;     ///< @brief Override to handle drag completion and update model

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;                     ///< @brief Track when drag starts
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;                      ///< @brief Handle mouse move for dragging and resizing
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;                   ///< @brief Track when drag ends
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;                      ///< @brief Update cursor based on mouse position (for resize handles)
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;                     ///< @brief Restore cursor when leaving item

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;              ///< @brief Show context menu

    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;                   ///<
    void dragLeaveEvent(QGraphicsSceneDragDropEvent* event) override;                   ///<
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;                        ///<

private:
    void updateModelFromPosition();                 ///< @brief Update the model with new dates based on current position
    void updateModelFromSize();                     ///< @brief Update the model with new dates based on current size (after resize)
    int calculateLaneFromYPosition() const;         ///< @brief Calculate lane number from current Y position

    enum ResizeHandle { None, LeftEdge, RightEdge };            ///< @brief Detect which edge/corner is under mouse cursor
    ResizeHandle getResizeHandle(const QPointF& pos) const;     ///< @brief Get resize handle at given position
    void updateCursor(ResizeHandle handle);                     ///< @brief Update cursor based on resize handle
    void drawAttachmentIndicator(QPainter* painter);            ///<
    void drawDragOverlay(QPainter* painter);                    ///<

    TimelineModel* model_ = nullptr;                        ///< Model reference (not owned)
    TimelineCoordinateMapper* mapper_ = nullptr;            ///< Coordinate mapper (not owned)
    QUndoStack* undoStack_ = nullptr;                       ///< Undo stack reference (not owned)

    bool isDragging_ = false;                               ///< Whether currently being dragged
    bool isMultiDragging_ = false;                          ///< Whether performing multi-item drag
    bool isBeingMultiDragged_ = false;                      ///< Whether being moved by another item's multi-drag
    bool isResizing_ = false;                               ///< Whether currently being resized
    bool resizable_ = true;                                 ///< Whether item can be resized
    bool skipNextUpdate_ = false;                           ///< Whether to skip next model update (used during undo/redo)
    bool isDragHover_ = false;                              ///<
    qreal originalZValue_ = 10.0;                           ///< Original z-value before drag (for restoration)

    QRectF rect_;                                           ///< Item's rectangle
    QBrush brush_;                                          ///< Fill brush
    QPen pen_;                                              ///< Border pen
    QString eventId_;                                       ///< ID of the event this item represents

    QPointF dragStartPos_;                                  ///< Position when drag started
    QRectF resizeStartRect_;                                ///< Rectangle when resize started
    ResizeHandle activeHandle_ = None;                      ///< Currently active resize handle
    QMap<QGraphicsItem*, QPointF> multiDragStartPositions_; ///< Starting positions of all selected items

    int attachmentCount_ = 0;                               ///<
    bool showAttachmentIndicator_ = false;                  ///<

    static constexpr double RESIZE_HANDLE_WIDTH = 8.0;      ///< Width of resize hit area
};
