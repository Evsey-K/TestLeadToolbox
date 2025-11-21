#pragma once
#include <QMainWindow>

class TimelineModule;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    TimelineModule *timelineModule = nullptr;
};
