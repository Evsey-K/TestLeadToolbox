#include "MainWindow.h"
#include "modules/timeline/TimelineModule.h"
#include <QDockWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Test Lead Toolbox");

    // Create timeline module and set as central widget
    timelineModule = new TimelineModule(this);
    setCentralWidget(timelineModule);
}

MainWindow::~MainWindow() {}
