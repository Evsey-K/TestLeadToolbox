// TimelineModule.cpp


#include "TimelineModule.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineView.h"
#include "TimelineScene.h"
#include "TimelineSidePanel.h"
#include "AddEventDialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QMessageBox>


TimelineModule::TimelineModule(QWidget* parent)
    : QWidget(parent)
{
    // Create model
    model_ = new TimelineModel(this);

    // Set default version dates (3 months from now)
    QDate today = QDate::currentDate();
    model_->setVersionDates(today, today.addMonths(3));

    // Create coordinate mapper
    mapper_ = new TimelineCoordinateMapper(model_->versionStartDate(),
                                           model_->versionEndDate(),
                                           TimelineCoordinateMapper::DEFAULT_PIXELS_PER_DAY);

    setupUi();
    setupConnections();

    // Add some sample data for testing
    createSampleData();
}


TimelineModel::~TimelineModule()
{
    delete mapper_;     // Model and widgets are owned by Qt parent hierarchy
}
