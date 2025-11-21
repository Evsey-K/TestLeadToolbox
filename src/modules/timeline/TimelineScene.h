#pragma once
#include <QGraphicsScene>

class TimelineScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit TimelineScene(QObject *parent = nullptr);
};
