// TimelineModule.cpp
#include "TimelineModule.h"
#include "TimelineView.h"
#include <QVBoxLayout>

TimelineModule::TimelineModule(QWidget *parent) : QWidget(parent) {
    // Create vertical layout to stack widgets vertically inside TimelineModule
    auto layout = new QVBoxLayout(this);

    // Instantiate the TimelineView and add it to the layout
    view = new TimelineView(this);
    layout->addWidget(view);

    // Assign the layout to this widget to manage child widget positioning
    setLayout(layout);
}

TimelineModule::~TimelineModule() {
    // Destructor can remain empty as Qt parent-child hierarchy handles memory cleanup
}
