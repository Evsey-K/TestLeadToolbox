#pragma once
#include <QWidget>

class TimelineView;

class TimelineModule : public QWidget {
    Q_OBJECT
public:
    explicit TimelineModule(QWidget *parent = nullptr);
    ~TimelineModule();
private:
    TimelineView *view = nullptr;
};
