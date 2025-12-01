// TimelineModule.cpp


#include "TimelineModule.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineView.h"
#include "TimelineItem.h"
#include "TimelineScene.h"
#include "TimelineSidePanel.h"
#include "AddEventDialog.h"
#include "VersionSettingsDialog.h"
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
    mapper_ = new TimelineCoordinateMapper(
        model_->versionStartDate(),
        model_->versionEndDate(),
        TimelineCoordinateMapper::DEFAULT_PIXELS_PER_DAY);

    setupUi();
    setupConnections();

    // Temporary - Add some sample data for testing
    createSampleData();
}


TimelineModule::~TimelineModule()
{
    delete mapper_;     // Model and widgets are owned by Qt parent hierarchy
}


void TimelineModule::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Toolbar with Add Event and Version Settings buttons
    auto toolbar = new QWidget();
    auto toolbarLayout = new QHBoxLayout(toolbar);

    // Add Event button
    addEventButton_ = new QPushButton("Add Event");
    addEventButton_->setIcon(QIcon::fromTheme("list-add"));
    toolbarLayout->addWidget(addEventButton_);

    // Version Settings button
    versionSettingsButton_ = new QPushButton("Version Settings");
    versionSettingsButton_->setIcon(QIcon::fromTheme("document-properties"));
    toolbarLayout->addWidget(versionSettingsButton_);

    toolbarLayout->addStretch();

    mainLayout->addWidget(toolbar);

    // Splitter for timeline view and side panel
    auto splitter = new QSplitter(Qt::Horizontal);

    // Create timeline view (which creates the scene internally)
    view_ = new TimelineView(model_, mapper_, this);
    splitter->addWidget(view_);

    // Create side panel
    sidePanel_ = new TimelineSidePanel(model_, this);
    sidePanel_->setMinimumWidth(300);
    sidePanel_->setMaximumWidth(500);
    splitter->addWidget(sidePanel_);

    // Set initial splitter sizes (70% timeline, 30% panel)
    splitter->setStretchFactor(0, 7);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter, 1); // Give splitter all remaining space
}


void TimelineModule::setupConnections()
{
    // Add Event button
    connect(addEventButton_, &QPushButton::clicked, this, &TimelineModule::onAddEventClicked);

    // Version Settings button
    connect(versionSettingsButton_, &QPushButton::clicked, this, &TimelineModule::onVersionSettingsClicked);

    // Side panel event selection
    connect(sidePanel_, &TimelineSidePanel::eventSelected, this, &TimelineModule::onEventSelectedInPanel);

    // Timeline item clicks update side panel
    connect(view_->timelineScene(), &TimelineScene::itemClicked, sidePanel_, &TimelineSidePanel::displayEventDetails);

    // Drag completion updates side panel
    connect(view_->timelineScene(), &TimelineScene::itemDragCompleted, sidePanel_, &TimelineSidePanel::displayEventDetails);

    // Update mapper when version dates change
    connect(model_, &TimelineModel::versionDatesChanged, [this]()
            {
                mapper_->setVersionDates(model_->versionStartDate(), model_->versionEndDate());
            });
}


void TimelineModule::onAddEventClicked()
{
    // Show add event dialog
    AddEventDialog dialog(model_->versionStartDate(), model_->versionEndDate(), this);

    if (dialog.exec() == QDialog::Accepted)
    {
        TimelineEvent newEvent = dialog.getEvent();
        QString eventId = model_->addEvent(newEvent);

        // Optional: Show confirmation
        QMessageBox::information(this, "Event Added", QString("Event '%1' has been added.")
                                 .arg(newEvent.title));
    }
}


void TimelineModule::onVersionSettingsClicked()
{
    // Show version settings dialog
    VersionSettingsDialog dialog(model_->versionStartDate(),
                                 model_->versionEndDate(),
                                 this);

    if (dialog.exec() == QDialog::Accepted)
    {
        QDate newStart = dialog.startDate();
        QDate newEnd = dialog.endDate();

        // Update model (this will trigger versionDatesChanged signal)
        model_->setVersionDates(newStart, newEnd);

        // Update mapper
        mapper_->setVersionDates(newStart, newEnd);

        // Rebuild the entire scene with new date range
        view_->timelineScene()->rebuildFromModel();

        // Show confirmation
        QString versionName = dialog.versionName();
        QString message = versionName.isEmpty()
                              ? QString("Version dates updated:\n%1 to %2")
                                    .arg(newStart.toString(Qt::ISODate))
                                    .arg(newEnd.toString(Qt::ISODate))
                              : QString("Version '%1' dates updated:\n%2 to %3")
                                    .arg(versionName)
                                    .arg(newStart.toString(Qt::ISODate))
                                    .arg(newEnd.toString(Qt::ISODate));

        QMessageBox::information(this, "Version Settings Updated", message);
    }
}


void TimelineModule::onEventSelectedInPanel(const QString& eventId)
{
    // Find the corresponding scene item and highlight/center it
    TimelineScene* scene = qobject_cast<TimelineScene*>(view_->scene());

    if (scene)
    {
        TimelineItem* item = scene->findItemByEventId(eventId);
        if (item)
        {
            // Clear previous selection
            scene->clearSelection();

            // Select the item
            item->setSelected(true);

            // Center the view on the item
            view_->centerOn(item);
        }
    }
}

void TimelineModule::createSampleData()
{
    // Add a few sample events for demonstration
    QDate start = model_->versionStartDate();

    TimelineEvent event1;
    event1.type = TimelineEventType_Meeting;
    event1.title = "Sprint Planning";
    event1.startDate = start.addDays(5);
    event1.endDate = start.addDays(5);
    event1.description = "Plan sprint activities";
    event1.priority = 4;
    model_->addEvent(event1);

    TimelineEvent event2;
    event2.type = TimelineEventType_TestEvent;
    event2.title = "Integration Testing";
    event2.startDate = start.addDays(20);
    event2.endDate = start.addDays(25);
    event2.description = "Run full integration test suite";
    event2.priority = 5;
    model_->addEvent(event2);

    TimelineEvent event3;
    event3.type = TimelineEventType_DueDate;
    event3.title = "Release Candidate";
    event3.startDate = start.addDays(60);
    event3.endDate = start.addDays(60);
    event3.description = "RC build deadline";
    event3.priority = 5;
    model_->addEvent(event3);

    TimelineEvent event4;
    event4.type = TimelineEventType_Action;
    event4.title = "Code Freeze";
    event4.startDate = start.addDays(55);
    event4.endDate = start.addDays(59);
    event4.description = "No new features after this";
    event4.priority = 4;
    model_->addEvent(event4);
}
