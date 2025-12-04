// TimelineScene.h


#pragma once
#include <QGraphicsScene>
#include <QMap>


class TimelineModel;
class TimelineCoordinateMapper;
class TimelineItem;
class TimelineDateScale;
class CurrentDateMarker;
class VersionBoundaryMarker;


/**
 * @class TimelineScene
 * @brief Custom QGraphicsScene that manages timeline items and their layout w/ collision avoidance
 *
 * This scene is responsible for:
 * - Creating visual items from model data
 * - Positioning items using coordinate mapper
 * - Responding to model changes via signals/slots
 * - Rendering date scale and current date marker (Phase 1 & 3)
 * - Emitting selection events when items are clicked
 */
class TimelineScene : public QGraphicsScene
{
    Q_OBJECT

public:

    /**
     * @brief Constructs a TimelineScene connected to a model
     * @param model Pointer to the timeline data model
     * @param mapper Pointer to coordinate mapping utility
     * @param parent Optional QObject parent
     */
    explicit TimelineScene(TimelineModel* model,
                           TimelineCoordinateMapper* mapper,
                           QObject *parent = nullptr);

    TimelineCoordinateMapper* coordinateMapper() const { return mapper_; }      ///< @brief Get the coordinate mapper
    void rebuildFromModel();                                                    ///< @brief Rebuild all items from the model (useful after zoom or major changes)
    TimelineItem* findItemByEventId(const QString& eventId) const;              ///< @brief Find the TimelineItem associated with an event ID

signals:
    void itemClicked(const QString& eventId);                   ///< @brief Emitted when a timeline item is clicked
    void itemDragCompleted(const QString& eventId);             ///< @brief Emitted when a timeline item drag is completed
    void editEventRequested(const QString& eventId);            ///<
    void deleteEventRequested(const QString& eventId);          ///<
    void batchDeleteRequested(const QStringList& eventIds);     ///< @brief Emitted when multiple events should be deleted

public slots:
    void onEventAdded(const QString& eventId);              ///< @brief Handle a new event being added to the model
    void onEventRemoved(const QString& eventId);            ///< @brief Handle an event being removed from the model
    void onEventUpdated(const QString& eventId);            ///< @brief Handle an event being updated in the model
    void onVersionDatesChanged();                           ///< @brief Handle version dates changing (requires full rebuild)
    void onLanesRecalculated();                             ///< @brief Handle lanes being recalculated

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;         ///< @brief Override to detect item clicks
    void keyPressEvent(QKeyEvent* event) override;                          ///<

private:
    TimelineItem* createItemForEvent(const QString& eventId);               ///< @brief Create a single timeline item from event data
    void updateItemFromEvent(TimelineItem* item, const QString& eventId);   ///< @brief Update an existing item's visual representation
    void updateSceneHeight();                                               ///< @brief Update scene height based on current lane count
    void setupDateScale();                                                  ///< @brief Initialize date scale and current date marker
    void setupVersionBoundaryMarkers();                                     ///< @brief Setup version boundary markers
    void connectItemSignals(TimelineItem* item);                            ///<

    TimelineModel* model_;                              ///< Data model (not owned)
    TimelineCoordinateMapper* mapper_;                  ///< Coordinate mapper (not owned)
    QMap<QString, TimelineItem*> eventIdToItem_;        ///< Map event IDs to scene items

    TimelineDateScale* dateScale_;                      ///< Date scale renderer (owned by scene)
    CurrentDateMarker* currentDateMarker_;              ///< Today marker (owned by scene)
    VersionBoundaryMarker* versionStartMarker_;         ///< Version start marker (owned by scene)
    VersionBoundaryMarker* versionEndMarker_;           ///< Version end marker (owned by scene)

    // Lane-based layout constants
    static constexpr double ITEM_HEIGHT = 30.0;         ///< Default height of timeline bars
    static constexpr double LANE_SPACING = 5.0;         ///< Vertical spacing between lanes
    static constexpr double DATE_SCALE_OFFSET = 70.0;   ///< Y offset for events (below date scale)
};
