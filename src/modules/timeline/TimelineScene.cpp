// TimelineScene.cpp
#include "TimelineScene.h"
#include "TimelineItem.h"

TimelineScene::TimelineScene(QObject *parent) : QGraphicsScene(parent) {
    // Add a sample TimelineItem to visually confirm the scene is rendering correctly
    // QRectF(0,0,200,30) specifies position (0,0) and size (width=200, height=30)
    auto item = new TimelineItem(QRectF(0, 0, 200, 30));
    addItem(item);

    // Position the sample item within the scene at coordinates (50, 50)
    item->setPos(50, 50);
}
