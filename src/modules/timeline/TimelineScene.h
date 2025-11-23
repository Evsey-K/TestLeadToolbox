// TimelineScene.h
#pragma once
#include <QGraphicsScene>

/**
 * @class TimelineScene
 * @brief Custom QGraphicsScene subclass that manages timeline items and their layout.
 *
 * TimelineScene holds the visual elements of the timeline such as timeline event items.
 */

class TimelineScene : public QGraphicsScene {
    Q_OBJECT
public:

    /**
     * @brief Constructs a TimelineScene with optional parent.
     * @param parent Optional QObject parent pointer.
     */

    explicit TimelineScene(QObject *parent = nullptr);
};
