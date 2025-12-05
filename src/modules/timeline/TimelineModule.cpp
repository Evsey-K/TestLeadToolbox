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
    setupAutoSave();

    // Try to load existing timeline data
    loadTimelineData();

    //
    setupUndoStack();
}


TimelineModule::~TimelineModule()
{
    // Save before exit
    if (autoSaveManager_)
    {
        autoSaveManager_->saveNow();
    }

    delete mapper_;     // Model and widgets are owned by Qt parent hierarchy
}


void TimelineModule::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Create toolbar
    auto toolbar = createToolbar();
    mainLayout->addWidget(toolbar);

    // Splitter for timeline view and side panel
    auto splitter = new QSplitter(Qt::Horizontal);

    // Prevent side panel from collapsing when dragging splitter
    splitter->setChildrenCollapsible(false);

    // Create timeline view (which creates the scene internally)
    view_ = new TimelineView(model_, mapper_, this);
    splitter->addWidget(view_);

    // Create scroll animator
    scrollAnimator_ = new TimelineScrollAnimator(view_, mapper_, this);

    // Create side panel
    sidePanel_ = new TimelineSidePanel(model_, this);
    sidePanel_->setMinimumWidth(300);
    sidePanel_->setMaximumWidth(500);
    splitter->addWidget(sidePanel_);

    // Set initial splitter sizes (70% timeline, 30% panel)
    splitter->setStretchFactor(0, 7);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter, 1); // Give splitter all remaining space

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
}


QToolBar* TimelineModule::createToolbar()
{
    auto toolbar = new QToolBar();
    toolbar->setIconSize(QSize(24, 24));

    // File operations
    auto saveAction = toolbar->addAction("💾 Save");
    saveAction->setToolTip("Save timeline data");
    connect(saveAction, &QAction::triggered, this, &TimelineModule::onSaveClicked);

    auto loadAction = toolbar->addAction("📂 Load");
    loadAction->setToolTip("Load timeline data");
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

    // TIER 2: Connect selection change signals to update delete button state
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
    QString saveFilePath = TimelineSerializer::getDefaultSaveLocation();

    autoSaveManager_ = new AutoSaveManager(model_, saveFilePath, this);

    // Start auto-save with 5-minute interval
    autoSaveManager_->startAutoSave(300000); // 5 minutes in milliseconds

    // Connect to auto-save signals
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


void TimelineModule::onSaveClicked()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Timeline",
        TimelineSerializer::getDefaultSaveLocation(),
        "JSON Files (*.json);;All Files (*)"
        );

    if (!filePath.isEmpty())
    {
        bool success = TimelineSerializer::saveToFile(model_, filePath);

        if (success)
        {
            statusLabel_->setText("Timeline saved to: " + filePath);
            autoSaveManager_->markClean();
            QMessageBox::information(this, "Success", "Timeline saved successfully!");
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to save timeline.");
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
            autoSaveManager_->saveNow();
        }
        else if (result == QMessageBox::Cancel)
        {
            return;
        }
    }

    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load Timeline",
        TimelineSerializer::getDefaultSaveLocation(),
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
    EditEventDialog dialog(eventId, model_,
                           model_->versionStartDate(),
                           model_->versionEndDate(),
                           this);

    int result = dialog.exec();

    if (result == QDialog::Accepted)
    {
        TimelineEvent updatedEvent = dialog.getEvent();
        // NEW: Use command instead of direct model call
        undoStack_->push(new UpdateEventCommand(model_, eventId, updatedEvent));
        statusLabel_->setText(QString("Event '%1' updated").arg(updatedEvent.title));
    }
    else if (result == EditEventDialog::DeleteRequested)
    {
        deleteEvent(eventId);
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
    QDate startDate = event->startDate;
    QDate endDate = event->endDate;

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
