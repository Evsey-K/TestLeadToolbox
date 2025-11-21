#pragma once
#include <QGraphicsView>

class TimelineScene;

class TimelineView : public QGraphicsView {
    Q_OBJECT
public:
    explicit TimelineView(QWidget *parent = nullptr);
protected:
    void wheelEvent(QWheelEvent *event) override; // zoom horizontally
private:
    TimelineScene *scene_ = nullptr;
};
