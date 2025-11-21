#include "TimelineScene.h"
#include "TimelineItem.h"

TimelineScene::TimelineScene(QObject *parent) : QGraphicsScene(parent) {
    // For starter: add a sample item so you see something
    auto item = new TimelineItem(QRectF(0,0,200,30));
    addItem(item);
    item->setPos(50,50);
}
