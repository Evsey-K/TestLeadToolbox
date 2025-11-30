// TimelineScene.h


#pragma once
#include <QGraphicsScene>
#include <QMap>


class TimelineModel;
class TimelineCoordinateMapper;
class TimelineItem;


/**
 * @class TimelineScene
 * @brief Custom QGraphicsScene that manages timeline items and their layout w/ collision avoidance
 *
 * This scene is responsible for:
 * - Creating visual items from model data
 * - Positioning items using coordinate mapper
 * - Responding to model changes via signals/slots
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

    /**
     * @brief Get the coordinate mapper
     */
    TimelineCoordinateMapper* coordinateMapper() const
    {
        return mapper_;
    }

    /**
     * @brief Rebuild all items from the model (useful after zoom or major changes)
     */
    void rebuildFromModel();

    /**
     * @brief Find the TimelineItem associated with an event ID
     */
    TimelineItem* findItemByEventId(const QString& eventId) const;

public slots:
    /**
     * @brief Handle a new event being added to the model
     */
    void onEventAdded(const QString& eventId);

    /**
     * @brief Handle an event being removed from the model
     */
    void onEventRemoved(const QString& eventId);

    /**
     * @brief Handle an event being updated in the model
     */
    void onEventUpdated(const QString& eventId);

    /**
     * @brief Handle version dates changing (requires full rebuild)
     */
    void onVersionDatesChanged();

    /**
     * @brief Handle lanes being recalculated (Sprint 2)
     */
    void onLanesRecalculated();

private:
    /**
     * @brief Create a single timeline item from event data
     */
    TimelineItem* createItemForEvent(const QString& eventId);

    /**
     * @brief Update an existing item's visual representation
     */
    void updateItemFromEvent(TimelineItem* item, const QString& eventId);

    /**
     * @brief Update scene height based on current lane count (Sprint 2)
     */
    void updateSceneHeight();

    TimelineModel* model_;                              ///< Data model (not owned)
    TimelineCoordinateMapper* mapper_;                  ///< Coordinate mapper (not owned)
    QMap<QString, TimelineItem*> eventIdToItem_;        ///< Map event IDs to scene items

    // Lane-based layout constants
    static constexpr double ITEM_HEIGHT = 30.0;         ///< Default height of timeline bars
    static constexpr double LANE_SPACING = 5.0;         ///< Vertical spacing between lanes
};
