#include "TimelineModule.h"
#include "TimelineView.h"
#include <QVBoxLayout>

TimelineModule::TimelineModule(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    view = new TimelineView(this);
    layout->addWidget(view);
    setLayout(layout);
}

TimelineModule::~TimelineModule() {}
