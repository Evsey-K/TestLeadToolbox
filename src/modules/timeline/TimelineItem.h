#pragma once
#include <QGraphicsRectItem>
#include <QBrush>

class TimelineItem : public QGraphicsRectItem {
public:
    TimelineItem(const QRectF &rect) : QGraphicsRectItem(rect) {
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(QGraphicsItem::ItemIsSelectable);
        setBrush(QBrush(Qt::blue));
    }
};
