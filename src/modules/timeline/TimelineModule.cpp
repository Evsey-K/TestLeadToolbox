// TimelineModule.cpp


#include "TimelineModule.h"
#include "TimelineModel.h"
#include "TimelineCoordinateMapper.h"
#include "TimelineView.h"
#include "TimelineItem.h"
#include "TimelineScene.h"
#include "TimelineSidePanel.h"
#include "TimelineLegend.h"
#include "AddEventDialog.h"
#include "VersionSettingsDialog.h"
#include "TimelineSerializer.h"
#include "AutoSaveManager.h"
#include "TimelineExporter.h"
#include "ScrollToDateDialog.h"
#include "TimelineScrollAnimator.h"
#include "EditEventDialog.h"
#include "TimelineCommands.h"
#include "TimelineSettings.h"
#include "ConfirmationDialog.h"
#include "ArchivedEventsDialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QLabel>


TimelineModule::TimelineModule(QWidget* parent)
    : QWidget(parent)
    , autoSaveManager_(nullptr)
    , scrollAnimator_(nullptr)
    , deleteAction_(nullptr)
    , undoStack_(nullptr)
    , legend_(nullptr)
    , legendCheckbox_(nullptr)
    , splitter_(nullptr)
    , currentFilePath_("")
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

    // Create undo stack BEFORE setupUi()
    setupUndoStack();

    setupUi();
    setupConnections();
    setupAutoSave();

    // Try to load existing timeline data
    loadTimelineData();

    // Create the legend (initially hidden)
    createLegend();
}


TimelineModule::~TimelineModule()
{
    // Only save on exit if we have a file path (user has saved at least once)
    if (autoSaveManager_ && !currentFilePath_.isEmpty())
    {
        autoSaveManager_->saveNow();
    }

    delete mapper_;
}


void TimelineModule::save() { onSaveClicked(); }        ///<
void TimelineModule::saveAs() { onSaveAsClicked(); }    ///<
void TimelineModule::load() { onLoadClicked(); }        ///<


void TimelineModule::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Create toolbar
    auto toolbar = createToolbar();
    mainLayout->addWidget(toolbar);

    // Splitter for timeline view and side panel
    splitter_ = new QSplitter(Qt::Horizontal);

    // Prevent side panel from collapsing when dragging splitter
    splitter_->setChildrenCollapsible(false);

    // Create timeline view FIRST (before side panel needs it)
    view_ = new TimelineView(model_, mapper_, this);
    view_->timelineScene()->setUndoStack(undoStack_);
    splitter_->addWidget(view_);

    // Connect undo stack to scene (must be after setupUndoStack() is called)
    if (view_->timelineScene())
    {
        view_->timelineScene()->setUndoStack(undoStack_);
    }

    // Create scroll animator
    scrollAnimator_ = new TimelineScrollAnimator(view_, mapper_, this);

    // ✅ NOW create side panel WITH view_ parameter
    sidePanel_ = new TimelineSidePanel(model_, view_, this);
    sidePanel_->setMinimumWidth(300);
    sidePanel_->setMaximumWidth(800);
    splitter_->addWidget(sidePanel_);

    // Set initial splitter sizes (70% timeline, 30% panel)
    splitter_->setStretchFactor(0, 7);
    splitter_->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter_, 1); // Give splitter all remaining space

    // Status bar for feedback
    statusLabel_ = new QLabel("Ready");
    auto statusLayout = new QHBoxLayout();
    statusLayout->addWidget(statusLabel_);
    statusLayout->addStretch();

    unsavedIndicator_ = new QLabel();
    statusLayout->addWidget(unsavedIndicator_);

    mainLayout->addLayout(statusLayout);

    // Restore side panel visibility from settings
    restoreSidePanelState();

    // Connect splitter moved signal to update legend position
    connect(splitter_, &QSplitter::splitterMoved, this, &TimelineModule::onSplitterMoved);
}


QToolBar* TimelineModule::createToolbar()
{
    auto toolbar = new QToolBar();
    toolbar->setIconSize(QSize(24, 24));

    // File operations
    auto saveAction = toolbar->addAction("💾 Save");
    saveAction->setToolTip("Save timeline to current file");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &TimelineModule::onSaveClicked);

    auto saveAsAction = toolbar->addAction("💾 Save As");
    saveAsAction->setToolTip("Save timeline to new file");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &TimelineModule::onSaveAsClicked);

    auto loadAction = toolbar->addAction("📂 Load");
    loadAction->setToolTip("Load timeline data");
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, this, &TimelineModule::onLoadClicked);


    toolbar->addSeparator();


    // Add the 'Version Settings' button
    versionSettingsButton_ = new QPushButton("⚙️ Version Settings");
    toolbar->addWidget(versionSettingsButton_);

    // Add the 'Add' event button Event operations
    addEventButton_ = new QPushButton("➕ Add Event");
    toolbar->addWidget(addEventButton_);

    // Add the 'delete' event button (context-sensitive, disabled when no selection)
    deleteAction_ = toolbar->addAction("🗑️ Delete");
    deleteAction_->setToolTip("Delete selected event(s)");
    deleteAction_->setEnabled(false);  // Initially disabled
    connect(deleteAction_, &QAction::triggered, this, &TimelineModule::onDeleteActionTriggered);

    auto archiveAction = toolbar->addAction("📦 Archive");
    archiveAction->setToolTip("View archived events");
    connect(archiveAction, &QAction::triggered, this, &TimelineModule::onShowArchivedEvents);


    toolbar->addSeparator();


    // Export operations
    auto exportMenu = new QMenu();
    auto exportScreenshotAction = exportMenu->addAction("Export Screenshot (PNG)");
    auto exportCSVAction = exportMenu->addAction("Export to CSV");
    auto exportPDFAction = exportMenu->addAction("Export to PDF");

    connect(exportScreenshotAction, &QAction::triggered, this, &TimelineModule::onExportScreenshot);
    connect(exportCSVAction, &QAction::triggered, this, &TimelineModule::onExportCSV);
    connect(exportPDFAction, &QAction::triggered, this, &TimelineModule::onExportPDF);

    auto exportButton = new QPushButton("📤 Export");
    exportButton->setMenu(exportMenu);
    toolbar->addWidget(exportButton);


    toolbar->addSeparator();


    // Navigation
    auto scrollToDateAction = toolbar->addAction("📅 Go to Date");
    scrollToDateAction->setToolTip("Scroll to specific date");
    connect(scrollToDateAction, &QAction::triggered, this, &TimelineModule::onScrollToDate);

    // Add legend checkbox after "Go to Date"
    legendCheckbox_ = new QCheckBox("Legend");
    legendCheckbox_->setToolTip("Show/Hide event type color legend");
    legendCheckbox_->setChecked(false);  // Initially unchecked (legend hidden)
    toolbar->addWidget(legendCheckbox_);
    connect(legendCheckbox_, &QCheckBox::toggled, this, &TimelineModule::onLegendToggled);

    // Add spacer to push toggle button to the right
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // Add side panel toggle button at the right end
    toggleSidePanelAction_ = toolbar->addAction("◀ Hide Panel");
    toggleSidePanelAction_->setToolTip("Show/Hide event list panel");
    toggleSidePanelAction_->setShortcut(QKeySequence("Ctrl+P"));
    connect(toggleSidePanelAction_, &QAction::triggered, this, &TimelineModule::onToggleSidePanelClicked);

    return toolbar;
}


void TimelineModule::setupConnections()
{
    connect(addEventButton_, &QPushButton::clicked, this, &TimelineModule::onAddEventClicked);                                      // Add Event button
    connect(versionSettingsButton_, &QPushButton::clicked, this, &TimelineModule::onVersionSettingsClicked);                        // Version Settings button
    connect(sidePanel_, &TimelineSidePanel::eventSelected, this, &TimelineModule::onEventSelectedInPanel);                          // Side panel event selection
    connect(view_->timelineScene(), &TimelineScene::itemClicked, sidePanel_, &TimelineSidePanel::displayEventDetails);              // Timeline item clicks update side panel
    connect(view_->timelineScene(), &TimelineScene::itemDragCompleted, sidePanel_, &TimelineSidePanel::displayEventDetails);        // Drag completion updates side panel
    connect(view_->timelineScene(), &TimelineScene::editEventRequested, this, &TimelineModule::onEditEventRequested);               //
    connect(view_->timelineScene(), &TimelineScene::deleteEventRequested, this, &TimelineModule::onDeleteEventRequested);           //
    connect(view_->timelineScene(), &TimelineScene::batchDeleteRequested, this, &TimelineModule::onBatchDeleteRequested);           //
    connect(sidePanel_, &TimelineSidePanel::editEventRequested, this, &TimelineModule::onEditEventRequested);                       //
    connect(sidePanel_, &TimelineSidePanel::deleteEventRequested, this, &TimelineModule::onDeleteEventRequested);                   //
    connect(model_, &TimelineModel::versionDatesChanged, [this]()                                                                   // Update mapper when version dates change
            {
                mapper_->setVersionDates(model_->versionStartDate(), model_->versionEndDate());
            });

    // Connect selection change signals to update delete button state
    connect(view_->timelineScene(), &QGraphicsScene::selectionChanged,
            this, &TimelineModule::updateDeleteActionState);
    connect(sidePanel_, &TimelineSidePanel::selectionChanged,
            this, &TimelineModule::updateDeleteActionState);

    // Scroll animator completion
    connect(scrollAnimator_, &TimelineScrollAnimator::scrollCompleted, [this](const QDate& date)
            {
                statusLabel_->setText(QString("Scrolled to: %1").arg(date.toString("yyyy-MM-dd")));
            });

    // Timeline item clicks also update status bar
    connect(view_->timelineScene(), &TimelineScene::itemClicked, this, [this](const QString& eventId)
            {
                const TimelineEvent* event = model_->getEvent(eventId);

                if (event)
                {
                    statusLabel_->setText(QString("Selected: %1").arg(event->title));
                }
            });
}


void TimelineModule::setupAutoSave()
{
    // Create auto-save manager but don't set file path or start it yet
    // It will be configured after the first manual save
    autoSaveManager_ = new AutoSaveManager(model_, "", this);

    // Connect to auto-save signals (unchanged)
    connect(autoSaveManager_, &AutoSaveManager::autoSaveCompleted, [this](const QString& filePath)
            {
                statusLabel_->setText("Auto-saved to: " + filePath);
            });

    connect(autoSaveManager_, &AutoSaveManager::autoSaveFailed, [this](const QString& error)
            {
                statusLabel_->setText("Auto-save failed: " + error);
            });

    connect(autoSaveManager_, &AutoSaveManager::unsavedChangesChanged, [this](bool hasUnsavedChanges)
            {
                unsavedIndicator_->setText(hasUnsavedChanges ? "● Unsaved changes" : "");
            });
}


void TimelineModule::loadTimelineData()
{
    QString defaultPath = TimelineSerializer::getDefaultSaveLocation();

    if (QFile::exists(defaultPath))
    {
        bool success = TimelineSerializer::loadFromFile(model_, defaultPath);
        if (success)
        {
            // Rebuild view
            mapper_->setVersionDates(model_->versionStartDate(), model_->versionEndDate());
            view_->timelineScene()->rebuildFromModel();

            setCurrentFilePath(defaultPath);  // Enable auto-save to this file
            statusLabel_->setText("Timeline loaded from: " + defaultPath);
        }
    }
}


void TimelineModule::onAddEventClicked()
{
    // Show add event dialog
    AddEventDialog dialog(model_->versionStartDate(), model_->versionEndDate(), this);

    if (dialog.exec() == QDialog::Accepted)
    {
        TimelineEvent newEvent = dialog.getEvent();

        // Validate lane conflicts if lane control is enabled
        if (newEvent.laneControlEnabled)
        {
            bool hasConflict = model_->hasLaneConflict(
                newEvent.startDate,
                newEvent.endDate,
                newEvent.manualLane
                );

            if (hasConflict)
            {
                QMessageBox::warning(
                    this,
                    "Lane Conflict",
                    QString("Another manually-controlled event already occupies lane %1 "
                            "during this time period.\n\n"
                            "Please choose a different lane number or adjust the event dates.")
                        .arg(newEvent.manualLane)
                    );
                // Re-open the dialog to let user fix the conflict
                onAddEventClicked();
                return;
            }
        }

        // No conflict, proceed with adding
        undoStack_->push(new AddEventCommand(model_, newEvent));
        statusLabel_->setText(QString("Event '%1' added").arg(newEvent.title));
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

        statusLabel_->setText(QString("Version dates updated: %1 to %2")
                                  .arg(newStart.toString("yyyy-MM-dd"))
                                  .arg(newEnd.toString("yyyy-MM-dd")));
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

            const TimelineEvent* event = model_->getEvent(eventId);
            if (event)
            {
                statusLabel_->setText(QString("Selected: %1").arg(event->title));
            }
        }
    }
}


bool TimelineModule::saveToFile(const QString& filePath)
{
    bool success = TimelineSerializer::saveToFile(model_, filePath);

    if (success)
    {
        setCurrentFilePath(filePath);
        autoSaveManager_->markClean();
        statusLabel_->setText("Timeline saved to: " + filePath);
        return true;
    }
    else
    {
        QMessageBox::warning(this, "Error", "Failed to save timeline.");
        return false;
    }
}


void TimelineModule::setCurrentFilePath(const QString& filePath)
{
    currentFilePath_ = filePath;

    // Update auto-save manager with new path
    autoSaveManager_->setSaveFilePath(filePath);

    // Enable auto-save now that we have a file path (if user has it enabled)
    if (!filePath.isEmpty() && TimelineSettings::instance().autoSaveEnabled())
    {
        if (!autoSaveManager_->isAutoSaveEnabled())
        {
            autoSaveManager_->startAutoSave(300000);  // 5 minutes
        }
    }
    else
    {
        autoSaveManager_->stopAutoSave();
    }
}


void TimelineModule::onSaveClicked()
{
    // If we have a current file path, save directly to it
    if (!currentFilePath_.isEmpty())
    {
        if (saveToFile(currentFilePath_))
        {
            QMessageBox::information(this, "Success", "Timeline saved successfully!");
        }
    }
    else
    {
        // No current file path - this is the first save, show dialog
        onSaveAsClicked();
    }
}


void TimelineModule::onSaveAsClicked()
{
    QString initialPath = currentFilePath_.isEmpty()
    ? TimelineSerializer::getDefaultSaveLocation()
    : currentFilePath_;

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Timeline As",
        initialPath,
        "JSON Files (*.json);;All Files (*)"
        );

    if (!filePath.isEmpty())
    {
        if (saveToFile(filePath))
        {
            QMessageBox::information(this, "Success", "Timeline saved successfully!");
        }
    }
}


void TimelineModule::onLoadClicked()
{
    // Warn if unsaved changes
    if (autoSaveManager_->hasUnsavedChanges())
    {
        auto result = QMessageBox::question(
            this,
            "Unsaved Changes",
            "You have unsaved changes. Do you want to save before loading?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
            );

        if (result == QMessageBox::Save)
        {
            onSaveClicked();
        }
        else if (result == QMessageBox::Cancel)
        {
            return;
        }
    }

    // Use current path's directory if available
    QString initialPath = currentFilePath_.isEmpty()
                              ? TimelineSerializer::getDefaultSaveLocation()
                              : QFileInfo(currentFilePath_).absolutePath();

    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load Timeline",
        initialPath,
        "JSON Files (*.json);;All Files (*)"
        );

    if (!filePath.isEmpty())
    {
        bool success = TimelineSerializer::loadFromFile(model_, filePath);

        if (success)
        {
            // Rebuild view
            mapper_->setVersionDates(model_->versionStartDate(), model_->versionEndDate());
            view_->timelineScene()->rebuildFromModel();

            // Set current file path and enable auto-save
            setCurrentFilePath(filePath);
            statusLabel_->setText("Timeline loaded from: " + filePath);
            autoSaveManager_->markClean();
            QMessageBox::information(this, "Success", "Timeline loaded successfully!");
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to load timeline.");
        }
    }
}


void TimelineModule::onExportScreenshot()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Screenshot",
        "timeline_screenshot.png",
        "PNG Images (*.png);;JPEG Images (*.jpg *.jpeg);;All Files (*)"
        );

    if (!filePath.isEmpty())
    {
        bool success = TimelineExporter::exportScreenshot(view_, filePath, true);

        if (success)
        {
            statusLabel_->setText("Screenshot exported to: " + filePath);
            QMessageBox::information(this, "Success", "Screenshot exported successfully!");
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to export screenshot.");
        }
    }
}


void TimelineModule::onExportCSV()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export to CSV",
        "timeline_events.csv",
        "CSV Files (*.csv);;All Files (*)"
        );

    if (!filePath.isEmpty())
    {
        bool success = TimelineExporter::exportToCSV(model_, filePath);

        if (success)
        {
            statusLabel_->setText("CSV exported to: " + filePath);
            QMessageBox::information(this, "Success",
                                     QString("Exported %1 events to CSV successfully!")
                                         .arg(model_->eventCount()));
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to export to CSV.");
        }
    }
}


void TimelineModule::onExportPDF()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export to PDF",
        "timeline_report.pdf",
        "PDF Files (*.pdf);;All Files (*)"
        );

    if (!filePath.isEmpty())
    {
        bool success = TimelineExporter::exportToPDF(model_, view_, filePath, true);

        if (success)
        {
            statusLabel_->setText("PDF exported to: " + filePath);
            QMessageBox::information(this, "Success", "PDF report generated successfully!");
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to export to PDF.");
        }
    }
}


void TimelineModule::onScrollToDate()
{
    ScrollToDateDialog dialog(
        QDate::currentDate(),
        model_->versionStartDate(),
        model_->versionEndDate(),
        this
        );

    if (dialog.exec() == QDialog::Accepted)
    {
        QDate targetDate = dialog.targetDate();
        bool animate = dialog.shouldAnimate();
        bool highlight = dialog.shouldHighlight();
        int rangeDays = dialog.highlightRangeDays();

        // Scroll to target date
        scrollAnimator_->scrollToDate(targetDate, animate, 750);

        // Create highlight overlay if requested
        if (highlight)
        {
            // Get current scene height
            double sceneHeight = view_->scene()->sceneRect().height();

            // Create highlight overlay
            DateRangeHighlight* highlightOverlay = new DateRangeHighlight(
                targetDate,
                rangeDays,
                mapper_,
                sceneHeight
                );

            // Add to scene
            view_->scene()->addItem(highlightOverlay);

            // Start fade-out after 2 seconds
            highlightOverlay->fadeOut(2000, 1000);

            // Update status
            statusLabel_->setText(QString("Scrolling to %1 (highlighting ±%2 days)")
                                      .arg(targetDate.toString("yyyy-MM-dd"))
                                      .arg(rangeDays));
        }
        else
        {
            statusLabel_->setText(QString("Scrolling to: %1")
                                      .arg(targetDate.toString("yyyy-MM-dd")));
        }
    }
}


void TimelineModule::onEditEventRequested(const QString& eventId)
{
    const TimelineEvent* event = model_->getEvent(eventId);

    if (!event)
    {
        qWarning() << "Cannot edit event: event not found" << eventId;
        return;
    }

    EditEventDialog dialog(eventId, model_,
                           model_->versionStartDate(),
                           model_->versionEndDate(),
                           this);

    bool deleteRequested = false;

    connect(&dialog, &EditEventDialog::deleteRequested, [&deleteRequested]() {
        deleteRequested = true;
    });

    int result = dialog.exec();

    if (deleteRequested)
    {
        // User clicked Delete button in the dialog
        onDeleteEventRequested(eventId);
        return;
    }

    if (result == QDialog::Accepted)
    {
        // User clicked Save - update the event
        TimelineEvent updatedEvent = dialog.getEvent();

        // Validate lane conflicts if lane control is enabled
        if (updatedEvent.laneControlEnabled)
        {
            bool hasConflict = model_->hasLaneConflict(
                updatedEvent.startDate,
                updatedEvent.endDate,
                updatedEvent.manualLane,
                eventId  // Exclude current event from conflict check
                );

            if (hasConflict)
            {
                QMessageBox::warning(
                    this,
                    "Lane Conflict",
                    QString("Another manually-controlled event already occupies lane %1 "
                            "during this time period.\n\n"
                            "Please choose a different lane number or adjust the event dates.")
                        .arg(updatedEvent.manualLane)
                    );
                // Re-open the dialog to let user fix the conflict
                onEditEventRequested(eventId);
                return;
            }
        }

        // No conflict, proceed with update
        undoStack_->push(new UpdateEventCommand(model_, eventId, updatedEvent));
        statusLabel_->setText(QString("Event '%1' updated").arg(updatedEvent.title));
    }
}


void TimelineModule::onDeleteEventRequested(const QString& eventId)
{
    deleteEvent(eventId);
}


bool TimelineModule::confirmDeletion(const QString& eventId)
{
    const TimelineEvent* event = model_->getEvent(eventId);

    if (!event) return false;

    auto result = QMessageBox::question(
        this,
        "Confirm Deletion",
        QString("Are you sure you want to delete this event?\n\n"
                "Title: %1\n"
                "Dates: %2 to %3\n\n"
                "This action cannot be undone.")
            .arg(event->title)
            .arg(event->startDate.toString("yyyy-MM-dd"))
            .arg(event->endDate.toString("yyyy-MM-dd")),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    return (result == QMessageBox::Yes);
}


bool TimelineModule::deleteEvent(const QString& eventId)
{
    const TimelineEvent* event = model_->getEvent(eventId);

    if (!event)
    {
        QMessageBox::warning(this, "Deletion Failed", "Event not found.");
        statusLabel_->setText("Event deletion failed - event not found");
        return false;
    }

    QString eventTitle = event->title;
    QDate startDate = event->startDate.date();
    QDate endDate = event->endDate.date();

    // NEW: Check soft delete preference
    bool useSoftDelete = TimelineSettings::instance().useSoftDelete();

    // NEW: Show confirmation with "don't ask again"
    if (!ConfirmationDialog::confirmDeletion(this, eventTitle, startDate, endDate, useSoftDelete))
    {
        statusLabel_->setText("Deletion cancelled");
        return false;
    }

    // NEW: Use command for undo support
    undoStack_->push(new DeleteEventCommand(model_, eventId, useSoftDelete));

    QString action = useSoftDelete ? "archived" : "deleted";
    statusLabel_->setText(QString("Event '%1' %2 successfully").arg(eventTitle, action));

    return true;
}


void TimelineModule::onBatchDeleteRequested(const QStringList& eventIds)
{
    deleteBatchEvents(eventIds);
}


bool TimelineModule::confirmBatchDeletion(const QStringList& eventIds)
{
    if (eventIds.isEmpty())
    {
        return false;
    }

    // Build list of events to delete
    QStringList eventDetails;
    for (const QString& eventId : eventIds)
    {
        const TimelineEvent* event = model_->getEvent(eventId);
        if (event)
        {
            eventDetails.append(QString("• %1 (%2 to %3)")
                                    .arg(event->title)
                                    .arg(event->startDate.toString("yyyy-MM-dd"))
                                    .arg(event->endDate.toString("yyyy-MM-dd")));
        }
    }

    QString message;

    if (eventIds.size() == 1)
    {
        message = QString("Are you sure you want to delete this event?\n\n%1\n\n"
                          "This action cannot be undone.")
                      .arg(eventDetails.join("\n"));
    }
    else
    {
        message = QString("Are you sure you want to delete these %1 events?\n\n%2\n\n"
                          "This action cannot be undone.")
                      .arg(eventIds.size())
                      .arg(eventDetails.join("\n"));
    }

    auto result = QMessageBox::question(
        this,
        "Confirm Deletion",
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    return (result == QMessageBox::Yes);
}


bool TimelineModule::deleteEventWithoutConfirmation(const QString& eventId)
{
    const TimelineEvent* event = model_->getEvent(eventId);

    if (!event)
    {
        QMessageBox::warning(this, "Deletion Failed", "Event not found.");
        statusLabel_->setText("Event deletion failed - event not found");

        return false;
    }

    QString eventTitle = event->title;

    bool success = model_->removeEvent(eventId);

    if (success)
    {
        statusLabel_->setText(QString("Event '%1' deleted successfully").arg(eventTitle));

        return true;
    }
    else
    {
        QMessageBox::critical(this, "Deletion Failed", "An error occurred while deleting the event.");
        statusLabel_->setText("Event deletion failed");

        return false;
    }
}


bool TimelineModule::deleteBatchEvents(const QStringList& eventIds)
{
    if (eventIds.isEmpty())
    {
        return false;
    }

    QStringList eventTitles;
    for (const QString& eventId : eventIds)
    {
        const TimelineEvent* event = model_->getEvent(eventId);
        if (event)
        {
            eventTitles.append(event->title);
        }
    }

    // NEW: Check soft delete preference
    bool useSoftDelete = TimelineSettings::instance().useSoftDelete();

    // NEW: Show confirmation with "don't ask again"
    if (!ConfirmationDialog::confirmBatchDeletion(this, eventTitles, useSoftDelete))
    {
        statusLabel_->setText("Deletion cancelled");
        return false;
    }

    // NEW: Use batch command for undo support
    undoStack_->push(new BatchDeleteCommand(model_, eventIds, useSoftDelete));

    QString action = useSoftDelete ? "archived" : "deleted";
    statusLabel_->setText(QString("%1 event(s) %2 successfully").arg(eventIds.size()).arg(action));

    return true;
}


// TIER 2 IMPLEMENTATION: Toolbar delete button handler
void TimelineModule::onDeleteActionTriggered()
{
    // Get all selected event IDs from both scene and side panel
    QStringList selectedEventIds = getAllSelectedEventIds();

    if (selectedEventIds.isEmpty())
    {
        statusLabel_->setText("No events selected for deletion");
        return;
    }

    // Delete all selected events
    deleteBatchEvents(selectedEventIds);
}


// TIER 2 IMPLEMENTATION: Update delete button state based on selection
void TimelineModule::updateDeleteActionState()
{
    if (!deleteAction_)
    {
        return;
    }

    // Get all selected event IDs
    QStringList selectedEventIds = getAllSelectedEventIds();

    // Enable delete button only if there's a selection
    bool hasSelection = !selectedEventIds.isEmpty();
    deleteAction_->setEnabled(hasSelection);

    // Update tooltip based on selection count
    if (hasSelection)
    {
        if (selectedEventIds.size() == 1)
        {
            deleteAction_->setToolTip("Delete selected event");
        }
        else
        {
            deleteAction_->setToolTip(QString("Delete %1 selected events").arg(selectedEventIds.size()));
        }
    }
    else
    {
        deleteAction_->setToolTip("Delete selected event(s)");
    }
}


// TIER 2 IMPLEMENTATION: Get all selected event IDs from scene and side panel
QStringList TimelineModule::getAllSelectedEventIds() const
{
    QStringList eventIds;

    // Get selection from timeline scene
    TimelineScene* scene = qobject_cast<TimelineScene*>(view_->scene());
    if (scene)
    {
        QList<QGraphicsItem*> selectedItems = scene->selectedItems();
        for (QGraphicsItem* item : selectedItems)
        {
            TimelineItem* timelineItem = qgraphicsitem_cast<TimelineItem*>(item);
            if (timelineItem && !timelineItem->eventId().isEmpty())
            {
                eventIds.append(timelineItem->eventId());
            }
        }
    }

    // Get selection from side panel (through its public method)
    QStringList panelSelection = sidePanel_->getSelectedEventIds();
    for (const QString& eventId : panelSelection)
    {
        if (!eventIds.contains(eventId))
        {
            eventIds.append(eventId);
        }
    }

    return eventIds;
}


void TimelineModule::setupUndoStack()
{
    undoStack_ = new QUndoStack(this);
    undoStack_->setUndoLimit(50);

    connect(undoStack_, &QUndoStack::indexChanged, [this]() {
        QString status;
        if (undoStack_->canUndo())
        {
            status = QString("Can undo: %1").arg(undoStack_->undoText());
        }
        else if (undoStack_->canRedo())
        {
            status = QString("Can redo: %1").arg(undoStack_->redoText());
        }
        else
        {
            status = "Ready";
        }

        if (statusLabel_)
        {
            statusLabel_->setText(status);
        }
    });
}


void TimelineModule::onShowArchivedEvents()
{
    ArchivedEventsDialog dialog(model_, undoStack_, this);

    dialog.exec();

    // Refresh the timeline view after dialog closes (in case events were restored)
    view_->timelineScene()->rebuildFromModel();
    sidePanel_->refreshAllTabs();
}


// Toggle side panel visibility
void TimelineModule::onToggleSidePanelClicked()
{
    bool isVisible = sidePanel_->isVisible();

    // Toggle visibility
    sidePanel_->setVisible(!isVisible);

    // Save state to settings
    TimelineSettings::instance().setSidePanelVisible(!isVisible);

    // Update button text and icon
    updateToggleButtonState();

    // Update status
    QString status = sidePanel_->isVisible() ? "Side panel shown" : "Side panel hidden";
    statusLabel_->setText(status);
}


// Update toggle button based on panel visibility
void TimelineModule::updateToggleButtonState()
{
    if (!toggleSidePanelAction_)
    {
        return;
    }

    bool isVisible = sidePanel_->isVisible();

    if (isVisible)
    {
        toggleSidePanelAction_->setText("◀ Hide Panel");
        toggleSidePanelAction_->setToolTip("Hide event list panel");
    }
    else
    {
        toggleSidePanelAction_->setText("▶ Show Panel");
        toggleSidePanelAction_->setToolTip("Show event list panel");
    }
}


// Restore side panel state from settings
void TimelineModule::restoreSidePanelState()
{
    bool visible = TimelineSettings::instance().sidePanelVisible();
    sidePanel_->setVisible(visible);
    updateToggleButtonState();

    qDebug() << "Side panel restored to:" << (visible ? "visible" : "hidden");
}


void TimelineModule::createLegend()
{
    // Create legend as a widget overlay on top of the view
    legend_ = new TimelineLegend(view_);

    // Build legend entries from event types
    QVector<TimelineLegend::LegendEntry> entries;

    entries.append({ "Meeting", TimelineModel::colorForType(TimelineEventType_Meeting) });
    entries.append({ "Action", TimelineModel::colorForType(TimelineEventType_Action) });
    entries.append({ "Test Event", TimelineModel::colorForType(TimelineEventType_TestEvent) });
    entries.append({ "Reminder", TimelineModel::colorForType(TimelineEventType_Reminder) });
    entries.append({ "Jira Ticket", TimelineModel::colorForType(TimelineEventType_JiraTicket) });

    legend_->setEntries(entries);

    // Position the legend (initially hidden)
    updateLegendPosition();

    // Keep legend on top
    legend_->raise();
}


void TimelineModule::setLegendVisible(bool visible)
{
    if (legend_)
    {
        if (visible)
        {
            legend_->show();
            updateLegendPosition();
            legend_->raise();  // Ensure it stays on top
        }
        else
        {
            legend_->hide();
        }
    }
}


void TimelineModule::updateLegendPosition()
{
    if (!legend_ || !legend_->isVisible())
    {
        return;
    }

    // Get splitter sizes to determine where the side panel begins
    QList<int> sizes = splitter_->sizes();
    if (sizes.size() < 2)
    {
        return;
    }

    // The timeline view width in pixels
    int viewWidth = sizes[0];

    // Position legend in the top-right corner of the view, with margin
    int legendX = viewWidth - legend_->width() - 10;
    int legendY = 10;

    // Move the widget to this position
    legend_->move(legendX, legendY);

    // Ensure legend stays on top
    legend_->raise();
}


void TimelineModule::onLegendToggled(bool checked)
{
    setLegendVisible(checked);

    QString status = checked ? "Legend shown" : "Legend hidden";
    statusLabel_->setText(status);
}


void TimelineModule::onSplitterMoved(int pos, int index)
{
    // When the splitter moves, update the legend position
    // Only update if legend is visible
    if (legend_ && legend_->isVisible())
    {
        updateLegendPosition();
    }
}


void TimelineModule::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    // Update legend position when window is resized
    if (legend_ && legend_->isVisible())
    {
        updateLegendPosition();
    }
}
