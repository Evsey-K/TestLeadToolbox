// TimelineSidePanel.cpp


#include "TimelineSidePanel.h"
#include "ui_TimelineSidePanel.h"
#include "TimelineCommands.h"
#include "AttachmentListWidget.h"
#include "TimelineModel.h"
#include "TimelineView.h"
#include "TimelineSettings.h"
#include "TimelineExporter.h"
#include "SetTodayDateDialog.h"
#include "SetLookaheadRangeDialog.h"
#include <functional>
#include <QListWidget>
#include <QPixmap>
#include <QPainter>
#include <QDate>
#include <QDateTimeEdit>
#include <QMenu>
#include <QKeyEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTabBar>
#include <algorithm>
#include <QClipboard>
#include <QGuiApplication>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QFormLayout>
#include <QStackedWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QToolTip>
#include <QScrollArea>
#include <QShortcut>


TimelineSidePanel::TimelineSidePanel(TimelineModel* model, TimelineView* view, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TimelineSidePanel)
    , model_(model)
    , view_(view)
    , currentSortMode_(TimelineSettings::SortMode::ByDate)
{
    ui->setupUi(this);

    // Robust edit-mode shortcuts (work even when child widgets have focus)
    saveShortcut_ = new QShortcut(QKeySequence::Save, this);
    saveShortcut_->setContext(Qt::WidgetWithChildrenShortcut);

    connect(saveShortcut_, &QShortcut::activated, this, [this]() {
        if (isEditingDetails_)
        {
            onDetailsSaveClicked();
        }
    });

    cancelShortcut_ = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    cancelShortcut_->setContext(Qt::WidgetWithChildrenShortcut);

    connect(cancelShortcut_, &QShortcut::activated, this, [this]() {
        if (isEditingDetails_)
        {
            onDetailsCancelClicked();
        }
    });

    ui->saveDetailsButton->setToolTip("Save changes (Ctrl+S)");
    ui->cancelDetailsButton->setToolTip("Cancel editing (Escape)");

    // Details pane actions (inline edit)
    connect(ui->editDetailsButton, &QPushButton::clicked, this, &TimelineSidePanel::onDetailsEditClicked);
    connect(ui->saveDetailsButton, &QPushButton::clicked, this, &TimelineSidePanel::onDetailsSaveClicked);
    connect(ui->cancelDetailsButton, &QPushButton::clicked, this, &TimelineSidePanel::onDetailsCancelClicked);

    // Delete remains a direct request (kept consistent with existing behavior)
    connect(ui->deleteDetailsButton, &QPushButton::clicked, this, [this]() {
        if (!currentEventId_.isEmpty())
        {
            emit deleteEventRequested(currentEventId_);
        }
    });

    // Ensure correct initial button state
    ui->saveDetailsButton->setVisible(false);
    ui->cancelDetailsButton->setVisible(false);

    // Load saved sort/filter preferences
    currentSortMode_ = TimelineSettings::instance().sidePanelSortMode();
    activeFilterTypes_ = TimelineSettings::instance().sidePanelFilterTypes();

    // Splitter behavior
    ui->splitter->setStretchFactor(0, 7);
    ui->splitter->setStretchFactor(1, 3);
    ui->splitter->setChildrenCollapsible(false);

    // Let the details pane shrink naturally when sections are collapsed
    ui->eventDetailsGroupBox->setMinimumHeight(0);
    ui->eventDetailsGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    if (ui->eventDetailsGroupBox->layout())
    {
        ui->eventDetailsGroupBox->layout()->setAlignment(Qt::AlignTop);
        ui->eventDetailsGroupBox->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    }

    ui->eventDetailsGroupBox->setVisible(false);

    // Enable multi-selection on all list widgets
    enableMultiSelection(ui->allEventsList);
    enableMultiSelection(ui->lookaheadList);
    enableMultiSelection(ui->todayList);

    // Add attachments UI under details pane
    setupAttachmentsSection();

    // Inline editor (Edit → Save/Cancel)
    setupInlineEditor();

    connectSignals();
    refreshAllTabs();
    adjustWidthToFitTabs();

    setupListWidgetContextMenu(ui->allEventsList);
    setupListWidgetContextMenu(ui->lookaheadList);
    setupListWidgetContextMenu(ui->todayList);

    // Setup tab bar context menu
    setupTabBarContextMenu();

    // Set focus policy to receive key events
    setFocusPolicy(Qt::StrongFocus);
    ui->allEventsList->setFocusPolicy(Qt::StrongFocus);
    ui->lookaheadList->setFocusPolicy(Qt::StrongFocus);
    ui->todayList->setFocusPolicy(Qt::StrongFocus);
}


TimelineSidePanel::~TimelineSidePanel()
{
    delete ui;
}


void TimelineSidePanel::setupAttachmentsSection()
{
    if (!ui->eventDetailsGroupBox || attachmentsSection_)
    {
        return;
    }

    QVBoxLayout* rootLayout = qobject_cast<QVBoxLayout*>(ui->eventDetailsGroupBox->layout());
    if (!rootLayout)
    {
        return;
    }

    attachmentsSection_ = new QWidget(ui->eventDetailsGroupBox);
    QVBoxLayout* attachRoot = new QVBoxLayout(attachmentsSection_);
    attachRoot->setContentsMargins(5, 4, 5, 5);
    attachRoot->setSpacing(6);

    // Lets the attachments section shrink when children are hidden
    attachRoot->setSizeConstraint(QLayout::SetMinimumSize);

    // ---- Header row (NO Add button anymore) ----
    attachmentsHeader_ = new QWidget(attachmentsSection_);
    QHBoxLayout* headerLayout = new QHBoxLayout(attachmentsHeader_);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    // Toggle button
    toggleAttachmentsButton_ = new QPushButton("▼ Attachments", attachmentsHeader_);
    toggleAttachmentsButton_->setFlat(true);
    toggleAttachmentsButton_->setStyleSheet("QPushButton { text-align: left; font-weight: bold; padding: 4px; }");
    toggleAttachmentsButton_->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(toggleAttachmentsButton_);

    // Count badge
    attachmentsCountBadge_ = new QLabel("0", attachmentsHeader_);
    attachmentsCountBadge_->setAlignment(Qt::AlignCenter);
    attachmentsCountBadge_->setMinimumWidth(22);
    attachmentsCountBadge_->setStyleSheet(
        "QLabel {"
        "  background: #E6E6E6;"
        "  color: #333;"
        "  border-radius: 9px;"
        "  padding: 2px 7px;"
        "  font-size: 8.5pt;"
        "  font-weight: 600;"
        "}"
        );

    headerLayout->addWidget(attachmentsCountBadge_);
    headerLayout->addStretch();

    // ---- Attachment widget ----
    attachmentsWidget_ = new AttachmentListWidget(attachmentsSection_);
    attachmentsWidget_->setAddButtonVisible(true);
    attachmentsWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    attachRoot->addWidget(attachmentsHeader_);

    // Best-effort: find the internal list and make it expand vertically
    if (auto innerList = attachmentsWidget_->findChild<QListWidget*>())
    {
        innerList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    // ---- Empty state ----
    attachmentsEmptyStateLabel_ = new QLabel("No attachments yet. Drag & drop files here, or click +.", attachmentsSection_);
    attachmentsEmptyStateLabel_->setWordWrap(true);
    attachmentsEmptyStateLabel_->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    attachmentsEmptyStateLabel_->setVisible(true);
    attachRoot->addWidget(attachmentsEmptyStateLabel_);

    // Show the internal Add button (as "+") now
    attachmentsWidget_->setAddButtonVisible(true);

    attachRoot->addWidget(attachmentsWidget_);

    // Insert at bottom of details pane (below scroll area)
    rootLayout->addWidget(attachmentsSection_);
    attachmentsSection_->setVisible(true);

    // Keep header badge + empty state updated when attachments change
    connect(attachmentsWidget_, &AttachmentListWidget::attachmentsChanged, this, &TimelineSidePanel::refreshAttachmentsForCurrentEvent);

    // Connect toggle button
    connect(toggleAttachmentsButton_, &QPushButton::clicked, this, &TimelineSidePanel::onToggleAttachmentsSection);

    // Start disabled until an event is selected
    attachmentsWidget_->setEnabled(false);
    refreshAttachmentsForCurrentEvent();
}


void TimelineSidePanel::refreshAttachmentsForCurrentEvent()
{
    // Note: attachmentsAddButton_ is removed - AttachmentListWidget handles the + button now
    if (!attachmentsSection_ || !attachmentsWidget_ || !attachmentsCountBadge_ || !attachmentsEmptyStateLabel_)
    {
        return;
    }

    const bool hasEvent = !currentEventId_.isEmpty();

    attachmentsWidget_->setEnabled(hasEvent);

    int count = 0;

    if (hasEvent && model_)
    {
        count = model_->getAttachmentCount(currentEventId_);
    }

    attachmentsCountBadge_->setText(QString::number(count));

    attachmentsWidget_->setVisible(attachmentsExpanded_);

    if (count <= 0)
    {
        attachmentsEmptyStateLabel_->setText("No attachments yet. Drag & drop files here, or click +.");
        attachmentsEmptyStateLabel_->setVisible(attachmentsExpanded_); // only show empty state when expanded
    }
    else
    {
        attachmentsEmptyStateLabel_->setVisible(false);
    }

    attachmentsWidget_->setVisible(true);
    attachmentsWidget_->setEventId(currentEventId_);
    attachmentsWidget_->refresh();

    if (count <= 0)
    {
        attachmentsEmptyStateLabel_->setText("No attachments yet. Drag & drop files here, or click +.");
        attachmentsEmptyStateLabel_->setVisible(true);
    }
    else
    {
        attachmentsEmptyStateLabel_->setVisible(false);
    }
}


void TimelineSidePanel::setupInlineEditor()
{
    QVBoxLayout* rootLayout = qobject_cast<QVBoxLayout*>(ui->eventDetailsGroupBox->layout());
    if (!rootLayout)
    {
        return;
    }

    // ---- Edit panel (hidden by default) ----
    editPanel_ = new QWidget(ui->eventDetailsGroupBox);
    QVBoxLayout* editRoot = new QVBoxLayout(editPanel_);
    editRoot->setContentsMargins(0, 0, 0, 0);
    editRoot->setSpacing(8);

    // Make the read-only details section collapsible
    if (rootLayout && ui->detailsScrollAreaWidgetContents)
    {
        QVBoxLayout* scrollLayout = qobject_cast<QVBoxLayout*>(ui->detailsScrollAreaWidgetContents->layout());
        if (scrollLayout)
        {
            // Add toggle button at the top of details
            toggleDetailsButton_ = new QPushButton("▼ Event Details");
            toggleDetailsButton_->setFlat(true);
            toggleDetailsButton_->setStyleSheet("QPushButton { text-align: left; font-weight: bold; padding: 4px; }");
            toggleDetailsButton_->setCursor(Qt::PointingHandCursor);

            // Insert at the beginning of scroll area
            scrollLayout->insertWidget(0, toggleDetailsButton_);

            // Lets the contents widget shrink when children are hidden
            scrollLayout->setSizeConstraint(QLayout::SetMinimumSize);

            connect(toggleDetailsButton_, &QPushButton::clicked, this, &TimelineSidePanel::onToggleDetailsSection);
        }
    }

    // Common fields
    QGroupBox* commonGroup = new QGroupBox("Event Information", editPanel_);
    QFormLayout* commonForm = new QFormLayout(commonGroup);

    typeCombo_ = new QComboBox(commonGroup);
    typeCombo_->addItem("Meeting", TimelineEventType_Meeting);
    typeCombo_->addItem("Action", TimelineEventType_Action);
    typeCombo_->addItem("Test Event", TimelineEventType_TestEvent);
    typeCombo_->addItem("Reminder", TimelineEventType_Reminder);
    typeCombo_->addItem("Jira Ticket", TimelineEventType_JiraTicket);
    commonForm->addRow("Type:", typeCombo_);

    titleEdit_ = new QLineEdit(commonGroup);
    titleEdit_->setPlaceholderText("Enter event title...");
    commonForm->addRow("Title:", titleEdit_);

    prioritySpin_ = new QSpinBox(commonGroup);
    prioritySpin_->setRange(0, 5);
    prioritySpin_->setToolTip("0 = Highest priority, 5 = Lowest priority");
    commonForm->addRow("Priority:", prioritySpin_);

    startDateTimeEdit_ = new QDateTimeEdit(commonGroup);
    startDateTimeEdit_->setCalendarPopup(true);
    startDateTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    commonForm->addRow("Start:", startDateTimeEdit_);

    endDateTimeEdit_ = new QDateTimeEdit(commonGroup);
    endDateTimeEdit_->setCalendarPopup(true);
    endDateTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    commonForm->addRow("End:", endDateTimeEdit_);

    descriptionEdit_ = new QTextEdit(commonGroup);
    descriptionEdit_->setPlaceholderText("Enter description...");
    descriptionEdit_->setMaximumHeight(90);
    commonForm->addRow("Description:", descriptionEdit_);

    editRoot->addWidget(commonGroup);

    // Type-specific
    QGroupBox* typeGroup = new QGroupBox("Type-Specific Details", editPanel_);
    QVBoxLayout* typeVBox = new QVBoxLayout(typeGroup);
    typeSpecificStack_ = new QStackedWidget(typeGroup);
    typeVBox->addWidget(typeSpecificStack_);
    editRoot->addWidget(typeGroup);

    // Make type-specific section collapsible (when it exists)
    if (typeGroup)
    {
        QVBoxLayout* typeVBox = qobject_cast<QVBoxLayout*>(typeGroup->layout());
        if (typeVBox)
        {
            toggleTypeSpecificButton_ = new QPushButton("▼ Type-Specific Details");
            toggleTypeSpecificButton_->setFlat(true);
            toggleTypeSpecificButton_->setStyleSheet("QPushButton { text-align: left; font-weight: bold; padding: 4px; }");
            toggleTypeSpecificButton_->setCursor(Qt::PointingHandCursor);

            typeVBox->insertWidget(0, toggleTypeSpecificButton_);

            connect(toggleTypeSpecificButton_, &QPushButton::clicked, this, &TimelineSidePanel::onToggleTypeSpecificSection);
        }
    }

    // Meeting page
    {
        QWidget* page = new QWidget(typeSpecificStack_);
        QFormLayout* form = new QFormLayout(page);
        meetingLocationEdit_ = new QLineEdit(page);
        meetingParticipantsEdit_ = new QLineEdit(page);
        meetingParticipantsEdit_->setPlaceholderText("Comma-separated...");
        form->addRow("Location:", meetingLocationEdit_);
        form->addRow("Participants:", meetingParticipantsEdit_);
        typeSpecificStack_->addWidget(page);
    }

    // Action page
    {
        QWidget* page = new QWidget(typeSpecificStack_);
        QFormLayout* form = new QFormLayout(page);
        actionStatusCombo_ = new QComboBox(page);
        actionStatusCombo_->addItems({"Not Started", "In Progress", "Blocked", "Completed"});
        actionDueDateTimeEdit_ = new QDateTimeEdit(page);
        actionDueDateTimeEdit_->setCalendarPopup(true);
        actionDueDateTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
        form->addRow("Status:", actionStatusCombo_);
        form->addRow("Due:", actionDueDateTimeEdit_);
        typeSpecificStack_->addWidget(page);
    }

    // Test Event page
    {
        QWidget* page = new QWidget(typeSpecificStack_);
        QVBoxLayout* v = new QVBoxLayout(page);
        QFormLayout* form = new QFormLayout();
        testCategoryCombo_ = new QComboBox(page);
        testCategoryCombo_->addItems({"Dry Run", "Preliminary", "Formal"});
        form->addRow("Category:", testCategoryCombo_);
        v->addLayout(form);

        prepChecklistWidget_ = new QWidget(page);
        prepChecklistLayout_ = new QVBoxLayout(prepChecklistWidget_);
        prepChecklistLayout_->setContentsMargins(0, 0, 0, 0);
        prepChecklistLayout_->setSpacing(2);
        v->addWidget(new QLabel("Preparation Checklist:", page));
        v->addWidget(prepChecklistWidget_);
        v->addStretch();

        typeSpecificStack_->addWidget(page);
    }

    // Reminder page
    {
        QWidget* page = new QWidget(typeSpecificStack_);
        QFormLayout* form = new QFormLayout(page);
        reminderDateTimeEdit_ = new QDateTimeEdit(page);
        reminderDateTimeEdit_->setCalendarPopup(true);
        reminderDateTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
        reminderRecurringCombo_ = new QComboBox(page);
        reminderRecurringCombo_->addItems({"None", "Daily", "Weekly", "Monthly"});
        form->addRow("Reminder:", reminderDateTimeEdit_);
        form->addRow("Recurring:", reminderRecurringCombo_);
        typeSpecificStack_->addWidget(page);
    }

    // Jira page
    {
        QWidget* page = new QWidget(typeSpecificStack_);
        QFormLayout* form = new QFormLayout(page);
        jiraKeyEdit_ = new QLineEdit(page);
        jiraSummaryEdit_ = new QLineEdit(page);
        jiraTypeCombo_ = new QComboBox(page);
        jiraTypeCombo_->addItems({"Story", "Bug", "Task", "Sub-task", "Epic"});
        jiraStatusCombo_ = new QComboBox(page);
        jiraStatusCombo_->addItems({"To Do", "In Progress", "Done"});
        form->addRow("Key:", jiraKeyEdit_);
        form->addRow("Summary:", jiraSummaryEdit_);
        form->addRow("Type:", jiraTypeCombo_);
        form->addRow("Status:", jiraStatusCombo_);
        typeSpecificStack_->addWidget(page);
    }

    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TimelineSidePanel::onDetailsTypeChanged);

    // Advanced group (lane control + lock state)
    QGroupBox* advancedGroup = new QGroupBox("Advanced", editPanel_);
    QVBoxLayout* advVBox = new QVBoxLayout(advancedGroup);

    laneControlEnabledCheck_ = new QCheckBox("Enable manual lane control", advancedGroup);
    manualLaneSpin_ = new QSpinBox(advancedGroup);
    manualLaneSpin_->setRange(0, 20);
    manualLaneSpin_->setEnabled(false);

    QHBoxLayout* laneRow = new QHBoxLayout();
    laneRow->addWidget(new QLabel("Lane number:", advancedGroup));
    laneRow->addWidget(manualLaneSpin_);
    laneRow->addStretch();

    fixedCheck_ = new QCheckBox("Fixed (prevent drag/resize)", advancedGroup);
    lockedCheck_ = new QCheckBox("Locked (prevent all editing)", advancedGroup);

    advVBox->addWidget(laneControlEnabledCheck_);
    advVBox->addLayout(laneRow);
    advVBox->addSpacing(8);
    advVBox->addWidget(fixedCheck_);
    advVBox->addWidget(lockedCheck_);
    advVBox->addStretch();

    connect(laneControlEnabledCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        if (manualLaneSpin_)
        {
            manualLaneSpin_->setEnabled(checked);
        }
    });

    connect(lockedCheck_, &QCheckBox::toggled, this, [this](bool locked) {
        if (!fixedCheck_)
        {
            return;
        }

        if (locked)
        {
            fixedCheck_->setChecked(true);
            fixedCheck_->setEnabled(false);
        }
        else
        {
            fixedCheck_->setEnabled(true);
        }
    });

    connect(titleEdit_, &QLineEdit::textChanged, this, [this]() {
        if (isEditingDetails_)
        {
            clearFieldHighlights(); // Clear on any change
        }
    });

    connect(startDateTimeEdit_, &QDateTimeEdit::dateTimeChanged, this, [this]() {
        if (isEditingDetails_)
        {
            clearFieldHighlights();
        }
    });

    connect(endDateTimeEdit_, &QDateTimeEdit::dateTimeChanged, this, [this]() {
        if (isEditingDetails_)
        {
            clearFieldHighlights();
        }
    });

    editRoot->addWidget(advancedGroup);

    // Insert the edit panel into the SCROLLABLE content area (attachments stay fixed below).
    QVBoxLayout* scrollLayout = nullptr;

    if (ui->detailsScrollAreaWidgetContents)
    {
        scrollLayout = qobject_cast<QVBoxLayout*>(ui->detailsScrollAreaWidgetContents->layout());
    }

    if (!scrollLayout)
    {
        // Fallback: if UI not updated for some reason, keep old behavior.
        const int insertIndex = (attachmentsSection_ != nullptr)
                                    ? qMax(0, rootLayout->indexOf(attachmentsSection_))
                                    : rootLayout->count();

        rootLayout->insertWidget(insertIndex, editPanel_);
        editPanel_->setVisible(false);
        return;
    }

    scrollLayout->insertWidget(0, editPanel_); // put editor above the read-only view
    editPanel_->setVisible(false);
}


void TimelineSidePanel::onDetailsEditClicked()
{
    if (currentEventId_.isEmpty())
    {
        return;
    }

    const TimelineEvent* event = model_->getEvent(currentEventId_);
    if (!event)
    {
        return;
    }

    if (!event->canEditInDialog())
    {
        QMessageBox::information(this, "Event Locked", "This event is locked and cannot be edited.");
        return;
    }

    enterEditMode(*event);
}


void TimelineSidePanel::onDetailsSaveClicked()
{
    if (!validateEditFields())
    {
        return;
    }

    if (!isEditingDetails_ || currentEventId_.isEmpty())
    {
        return;
    }

    const TimelineEvent* current = model_->getEvent(currentEventId_);
    if (!current)
    {
        exitEditMode(true);
        return;
    }

    TimelineEvent edited = buildEditedEvent(*current);

    if (!edited.startDate.isValid() || !edited.endDate.isValid())
    {
        QMessageBox::warning(this, "Invalid Dates", "Start and End date/time must be set.");
        return;
    }

    if (edited.startDate > edited.endDate)
    {
        QMessageBox::warning(this, "Invalid Dates", "Start date/time must be before End date/time.");
        return;
    }

    if (edited.title.trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Missing Title", "Title cannot be empty.");
        return;
    }

    // Push a single undoable update command.
    if (model_ && model_->undoStack())
    {
        model_->undoStack()->push(new UpdateEventCommand(model_, currentEventId_, edited));
    }

    exitEditMode(true);

    // Refresh the displayed details immediately (model signals will also update lists).
    displayEventDetails(currentEventId_);
}


void TimelineSidePanel::onDetailsCancelClicked()
{
    if (!isEditingDetails_)
    {
        return;
    }

    if (hasUnsavedChanges_)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Discard Changes?",
            "You have unsaved changes. Discard them?",
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply != QMessageBox::Yes)
        {
            return;
        }
    }

    hasUnsavedChanges_ = false;
    exitEditMode(true);
    displayEventDetails(currentEventId_);
}


void TimelineSidePanel::onDetailsTypeChanged(int /*index*/)
{
    if (!isEditingDetails_)
    {
        return;
    }

    TimelineEventType type = TimelineEventType_Meeting;
    if (typeCombo_)
    {
        type = static_cast<TimelineEventType>(typeCombo_->currentData().toInt());
    }

    // Switch the visible editor page.
    if (typeSpecificStack_)
    {
        switch (type)
        {
        case TimelineEventType_Meeting:    typeSpecificStack_->setCurrentIndex(0); break;
        case TimelineEventType_Action:     typeSpecificStack_->setCurrentIndex(1); break;
        case TimelineEventType_TestEvent:  typeSpecificStack_->setCurrentIndex(2); break;
        case TimelineEventType_Reminder:   typeSpecificStack_->setCurrentIndex(3); break;
        case TimelineEventType_JiraTicket: typeSpecificStack_->setCurrentIndex(4); break;
        default:                           typeSpecificStack_->setCurrentIndex(0); break;
        }
    }
}


static void setWidgetVisible(QWidget* w, bool visible)
{
    if (w) w->setVisible(visible);
}


void TimelineSidePanel::updateDetailsPaneGeometry()
{
    if (!ui || !ui->eventDetailsGroupBox)
    {
        return;
    }

    // Always keep the contents packed toward the top.
    if (auto l = ui->eventDetailsGroupBox->layout())
    {
        l->setAlignment(Qt::AlignTop);
        l->setSizeConstraint(QLayout::SetMinimumSize);
    }

    // --- Robustly locate the details scroll area ---
    QScrollArea* detailsScrollArea = ui->eventDetailsGroupBox->findChild<QScrollArea*>("detailsScrollArea");

    // If objectName differs, fallback to "the first scroll area inside the groupbox"
    if (!detailsScrollArea)
    {
        const auto scrolls = ui->eventDetailsGroupBox->findChildren<QScrollArea*>();
        if (!scrolls.isEmpty())
        {
            detailsScrollArea = scrolls.first();
        }
    }

    // Clamp/restore scroll area height based on detailsExpanded_
    if (detailsScrollArea && toggleDetailsButton_)
    {
        if (!detailsExpanded_)
        {
            const int headerH = toggleDetailsButton_->sizeHint().height();
            const int collapsedH = headerH + 10;

            detailsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            detailsScrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            detailsScrollArea->setMinimumHeight(collapsedH);
            detailsScrollArea->setMaximumHeight(collapsedH);
        }
        else
        {
            detailsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            detailsScrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            detailsScrollArea->setMinimumHeight(0);
            detailsScrollArea->setMaximumHeight(QWIDGETSIZE_MAX);
        }

        detailsScrollArea->updateGeometry();
    }

    // Clamp/restore attachments section height based on attachmentsExpanded_
    if (attachmentsSection_ && toggleAttachmentsButton_)
    {
        if (!attachmentsExpanded_)
        {
            const int headerH = toggleAttachmentsButton_->sizeHint().height();
            const int collapsedH = headerH + 12;

            attachmentsSection_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            attachmentsSection_->setMinimumHeight(collapsedH);
            attachmentsSection_->setMaximumHeight(collapsedH);
        }
        else
        {
            attachmentsSection_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            attachmentsSection_->setMinimumHeight(0);
            attachmentsSection_->setMaximumHeight(QWIDGETSIZE_MAX);
        }

        attachmentsSection_->updateGeometry();
    }

    // Force re-layout now (prevents "headers floating mid-groupbox")
    if (auto l = ui->eventDetailsGroupBox->layout())
    {
        l->invalidate();
        l->activate();
    }
    ui->eventDetailsGroupBox->updateGeometry();
}



void TimelineSidePanel::enterEditMode(const TimelineEvent& event)
{
    isEditingDetails_ = true;
    hasUnsavedChanges_ = false;
    detailsOriginalEvent_ = event;

    // Toggle header buttons (Edit/Delete ↔ Save/Cancel)
    ui->editDetailsButton->setVisible(false);
    ui->deleteDetailsButton->setVisible(false);
    ui->saveDetailsButton->setVisible(true);
    ui->cancelDetailsButton->setVisible(true);

    // Hide the read-only view widgets while editing
    setWidgetVisible(ui->titleLabel, false);
    setWidgetVisible(ui->titleValueLabel, false);
    setWidgetVisible(ui->typeLabel, false);
    setWidgetVisible(ui->typeValueLabel, false);
    setWidgetVisible(ui->priorityLabel, false);
    setWidgetVisible(ui->priorityValueLabel, false);
    setWidgetVisible(ui->laneLabel, false);
    setWidgetVisible(ui->laneValueLabel, false);
    setWidgetVisible(ui->laneControlLabel, false);
    setWidgetVisible(ui->laneControlValueLabel, false);
    setWidgetVisible(ui->detailsLabel, false);
    setWidgetVisible(ui->detailsTextEdit, false);

    // Show edit panel
    if (editPanel_) editPanel_->setVisible(true);

    // Prevent accidental selection changes while editing.
    ui->tabWidget->setEnabled(false);

    // Populate common editors
    if (titleEdit_) titleEdit_->setText(event.title);
    if (prioritySpin_) prioritySpin_->setValue(event.priority);
    if (startDateTimeEdit_) startDateTimeEdit_->setDateTime(event.startDate);
    if (endDateTimeEdit_) endDateTimeEdit_->setDateTime(event.endDate);
    if (descriptionEdit_) descriptionEdit_->setPlainText(event.description);

    if (typeCombo_)
    {
        int idx = typeCombo_->findData(event.type);
        if (idx >= 0)
        {
            typeCombo_->setCurrentIndex(idx);
        }
    }

    // Preserve styles so we don't wipe designer/theme styling
    if (editPanel_ && originalEditPanelStyle_.isEmpty())
    {
        originalEditPanelStyle_ = editPanel_->styleSheet();
    }

    if (ui->eventDetailsGroupBox && originalDetailsGroupStyle_.isEmpty())
    {
        originalDetailsGroupStyle_ = ui->eventDetailsGroupBox->styleSheet();
    }

    // ADD VISUAL FEEDBACK:
    if (editPanel_)
    {
        editPanel_->setStyleSheet(
            "QGroupBox {"
            "    border: 2px solid #0078d7;"
            "    border-radius: 4px;"
            "    background-color: #f0f8ff;"
            "}"
            "QLineEdit, QTextEdit, QSpinBox, QComboBox, QDateTimeEdit {"
            "    background-color: white;"
            "}"
            );
    }

    // Highlight the event details group box
    if (ui->eventDetailsGroupBox)
    {
        ui->eventDetailsGroupBox->setStyleSheet(
            "QGroupBox {"
            "    border: 2px solid #0078d7;"
            "    background-color: #fffef0;"
            "}"
            );
    }

    if (laneControlEnabledCheck_) laneControlEnabledCheck_->setChecked(event.laneControlEnabled);
    if (manualLaneSpin_) manualLaneSpin_->setValue(event.manualLane);
    if (manualLaneSpin_) manualLaneSpin_->setEnabled(event.laneControlEnabled);
    if (fixedCheck_) fixedCheck_->setChecked(event.isFixed);
    if (lockedCheck_) lockedCheck_->setChecked(event.isLocked);

    populateTypeSpecificEditors(event.type, event);
    onDetailsTypeChanged(typeCombo_ ? typeCombo_->currentIndex() : 0);

    if (editPanel_) editPanel_->setVisible(true);

    // ADD CHANGE TRACKING (store connections for later disconnect):
    auto markDirty = [this]() { hasUnsavedChanges_ = true; };

    // Clear and rebuild tracking connections each time edit mode starts
    changeTrackingConnections_.clear();

    // Common
    if (titleEdit_) changeTrackingConnections_.append(connect(titleEdit_, &QLineEdit::textChanged, this, markDirty));
    if (descriptionEdit_) changeTrackingConnections_.append(connect(descriptionEdit_, &QTextEdit::textChanged, this, markDirty));
    if (prioritySpin_) changeTrackingConnections_.append(connect(prioritySpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, markDirty));
    if (startDateTimeEdit_) changeTrackingConnections_.append(connect(startDateTimeEdit_, &QDateTimeEdit::dateTimeChanged, this, markDirty));
    if (endDateTimeEdit_) changeTrackingConnections_.append(connect(endDateTimeEdit_, &QDateTimeEdit::dateTimeChanged, this, markDirty));
    if (typeCombo_) changeTrackingConnections_.append(connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, markDirty));

    // Meeting
    if (meetingLocationEdit_) changeTrackingConnections_.append(connect(meetingLocationEdit_, &QLineEdit::textChanged, this, markDirty));
    if (meetingParticipantsEdit_) changeTrackingConnections_.append(connect(meetingParticipantsEdit_, &QLineEdit::textChanged, this, markDirty));

    // Action
    if (actionStatusCombo_) changeTrackingConnections_.append(connect(actionStatusCombo_, &QComboBox::currentTextChanged, this, markDirty));
    if (actionDueDateTimeEdit_) changeTrackingConnections_.append(connect(actionDueDateTimeEdit_, &QDateTimeEdit::dateTimeChanged, this, markDirty));

    // Reminder
    if (reminderDateTimeEdit_) changeTrackingConnections_.append(connect(reminderDateTimeEdit_, &QDateTimeEdit::dateTimeChanged, this, markDirty));
    if (reminderRecurringCombo_) changeTrackingConnections_.append(connect(reminderRecurringCombo_, &QComboBox::currentTextChanged, this, markDirty));

    // Jira
    if (jiraKeyEdit_) changeTrackingConnections_.append(connect(jiraKeyEdit_, &QLineEdit::textChanged, this, markDirty));
    if (jiraSummaryEdit_) changeTrackingConnections_.append(connect(jiraSummaryEdit_, &QLineEdit::textChanged, this, markDirty));
    if (jiraTypeCombo_) changeTrackingConnections_.append(connect(jiraTypeCombo_, &QComboBox::currentTextChanged, this, markDirty));
    if (jiraStatusCombo_) changeTrackingConnections_.append(connect(jiraStatusCombo_, &QComboBox::currentTextChanged, this, markDirty));

    // Test Event
    if (testCategoryCombo_) changeTrackingConnections_.append(connect(testCategoryCombo_, &QComboBox::currentTextChanged, this, markDirty));

    // Advanced
    if (laneControlEnabledCheck_) changeTrackingConnections_.append(connect(laneControlEnabledCheck_, &QCheckBox::toggled, this, markDirty));
    if (manualLaneSpin_) changeTrackingConnections_.append(connect(manualLaneSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, markDirty));
    if (fixedCheck_) changeTrackingConnections_.append(connect(fixedCheck_, &QCheckBox::toggled, this, markDirty));
    if (lockedCheck_) changeTrackingConnections_.append(connect(lockedCheck_, &QCheckBox::toggled, this, markDirty));
}


void TimelineSidePanel::exitEditMode(bool /*keepSelection*/)
{
    isEditingDetails_ = false;

    // Disconnect change tracking
    for (const auto& conn : changeTrackingConnections_)
    {
        disconnect(conn);
    }
    changeTrackingConnections_.clear();

    ui->editDetailsButton->setVisible(true);
    ui->deleteDetailsButton->setVisible(true);
    ui->saveDetailsButton->setVisible(false);
    ui->cancelDetailsButton->setVisible(false);

    // Restore the read-only view widgets
    setWidgetVisible(ui->titleLabel, true);
    setWidgetVisible(ui->titleValueLabel, true);
    setWidgetVisible(ui->typeLabel, true);
    setWidgetVisible(ui->typeValueLabel, true);
    setWidgetVisible(ui->priorityLabel, true);
    setWidgetVisible(ui->priorityValueLabel, true);
    setWidgetVisible(ui->laneLabel, true);
    setWidgetVisible(ui->laneValueLabel, true);
    setWidgetVisible(ui->laneControlLabel, true);
    setWidgetVisible(ui->laneControlValueLabel, true);
    setWidgetVisible(ui->detailsLabel, true);
    setWidgetVisible(ui->detailsTextEdit, true);

    // Restore Visual Feedback:
    if (editPanel_)
    {
        editPanel_->setStyleSheet(originalEditPanelStyle_);
    }

    if (ui->eventDetailsGroupBox)
    {
        ui->eventDetailsGroupBox->setStyleSheet(originalDetailsGroupStyle_);
    }

    if (editPanel_) editPanel_->setVisible(false);

    ui->tabWidget->setEnabled(true);

    if (editPanel_) editPanel_->setVisible(false);

    hasUnsavedChanges_ = false;
}


TimelineEvent TimelineSidePanel::buildEditedEvent(const TimelineEvent& base) const
{
    TimelineEvent e = base;

    // Common fields
    if (typeCombo_)
    {
        e.type = static_cast<TimelineEventType>(typeCombo_->currentData().toInt());
        // Keep existing color unless your model derives it elsewhere; this preserves existing visuals.
    }

    if (titleEdit_) e.title = titleEdit_->text();
    if (prioritySpin_) e.priority = prioritySpin_->value();
    if (startDateTimeEdit_) e.startDate = startDateTimeEdit_->dateTime();
    if (endDateTimeEdit_) e.endDate = endDateTimeEdit_->dateTime();
    if (descriptionEdit_) e.description = descriptionEdit_->toPlainText();

    if (laneControlEnabledCheck_) e.laneControlEnabled = laneControlEnabledCheck_->isChecked();
    if (manualLaneSpin_) e.manualLane = manualLaneSpin_->value();
    if (fixedCheck_) e.isFixed = fixedCheck_->isChecked();
    if (lockedCheck_) e.isLocked = lockedCheck_->isChecked();

    collectTypeSpecificEdits(e);
    return e;
}


void TimelineSidePanel::populateTypeSpecificEditors(TimelineEventType type, const TimelineEvent& event)
{
    switch (type)
    {
    case TimelineEventType_Meeting:
        if (meetingLocationEdit_) meetingLocationEdit_->setText(event.location);
        if (meetingParticipantsEdit_) meetingParticipantsEdit_->setText(event.participants);
        break;

    case TimelineEventType_Action:
        if (actionStatusCombo_)
        {
            int idx = actionStatusCombo_->findText(event.status);
            actionStatusCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
        }
        if (actionDueDateTimeEdit_) actionDueDateTimeEdit_->setDateTime(event.dueDateTime);
        break;

    case TimelineEventType_TestEvent:
        if (testCategoryCombo_)
        {
            int idx = testCategoryCombo_->findText(event.testCategory);
            testCategoryCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
        }
        rebuildPreparationChecklistUI(event.preparationChecklist);
        break;

    case TimelineEventType_Reminder:
        if (reminderDateTimeEdit_) reminderDateTimeEdit_->setDateTime(event.reminderDateTime);
        if (reminderRecurringCombo_)
        {
            const QString rule = event.recurringRule.isEmpty() ? "None" : event.recurringRule;
            int idx = reminderRecurringCombo_->findText(rule);
            reminderRecurringCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
        }
        break;

    case TimelineEventType_JiraTicket:
        if (jiraKeyEdit_) jiraKeyEdit_->setText(event.jiraKey);
        if (jiraSummaryEdit_) jiraSummaryEdit_->setText(event.jiraSummary);
        if (jiraTypeCombo_)
        {
            int idx = jiraTypeCombo_->findText(event.jiraType);
            jiraTypeCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
        }
        if (jiraStatusCombo_)
        {
            int idx = jiraStatusCombo_->findText(event.jiraStatus);
            jiraStatusCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
        }
        break;

    default:
        break;
    }
}


void TimelineSidePanel::collectTypeSpecificEdits(TimelineEvent& event) const
{
    switch (event.type)
    {
    case TimelineEventType_Meeting:
        if (meetingLocationEdit_) event.location = meetingLocationEdit_->text();
        if (meetingParticipantsEdit_) event.participants = meetingParticipantsEdit_->text();
        break;

    case TimelineEventType_Action:
        if (actionStatusCombo_) event.status = actionStatusCombo_->currentText();
        if (actionDueDateTimeEdit_) event.dueDateTime = actionDueDateTimeEdit_->dateTime();
        break;

    case TimelineEventType_TestEvent:
        if (testCategoryCombo_) event.testCategory = testCategoryCombo_->currentText();
        for (auto it = prepChecklistChecks_.constBegin(); it != prepChecklistChecks_.constEnd(); ++it)
        {
            if (it.value())
            {
                event.preparationChecklist[it.key()] = it.value()->isChecked();
            }
        }
        break;

    case TimelineEventType_Reminder:
        if (reminderDateTimeEdit_) event.reminderDateTime = reminderDateTimeEdit_->dateTime();
        if (reminderRecurringCombo_)
        {
            const QString txt = reminderRecurringCombo_->currentText();
            event.recurringRule = (txt == "None") ? "" : txt;
        }
        break;

    case TimelineEventType_JiraTicket:
        if (jiraKeyEdit_) event.jiraKey = jiraKeyEdit_->text();
        if (jiraSummaryEdit_) event.jiraSummary = jiraSummaryEdit_->text();
        if (jiraTypeCombo_) event.jiraType = jiraTypeCombo_->currentText();
        if (jiraStatusCombo_) event.jiraStatus = jiraStatusCombo_->currentText();
        break;

    default:
        break;
    }
}


void TimelineSidePanel::rebuildPreparationChecklistUI(const QMap<QString, bool>& checklist)
{
    if (!prepChecklistLayout_ || !prepChecklistWidget_)
    {
        return;
    }

    // Clear previous
    while (QLayoutItem* item = prepChecklistLayout_->takeAt(0))
    {
        if (QWidget* w = item->widget())
        {
            w->deleteLater();
        }
        delete item;
    }
    prepChecklistChecks_.clear();

    // If no checklist stored yet, provide a sensible default set.
    QMap<QString, bool> effective = checklist;
    if (effective.isEmpty())
    {
        effective.insert("IAVM Installation (C/U)", false);
        effective.insert("CM Software Audit (C/U)", false);
        effective.insert("Latest Build Installation (C/U)", false);
        effective.insert("Test Scenarios Prepared", false);
        effective.insert("Test Event Schedule Created", false);
        effective.insert("TRR Slides Prepared", false);
        effective.insert("In-Brief Slides Prepared", false);
        effective.insert("Test Event Meetings Established", false);
    }

    for (auto it = effective.constBegin(); it != effective.constEnd(); ++it)
    {
        QCheckBox* cb = new QCheckBox(it.key(), prepChecklistWidget_);
        cb->setChecked(it.value());
        prepChecklistLayout_->addWidget(cb);
        prepChecklistChecks_.insert(it.key(), cb);

        connect(cb, &QCheckBox::toggled, this, [this](bool) {
            if (isEditingDetails_)
            {
                hasUnsavedChanges_ = true;
            }
        });
    }
    prepChecklistLayout_->addStretch();
}



// Update keyPressEvent() to handle edit mode shortcuts:

void TimelineSidePanel::keyPressEvent(QKeyEvent* event)
{
    // Handle editing shortcuts when in edit mode
    if (isEditingDetails_)
    {
        if (event->key() == Qt::Key_Escape)
        {
            onDetailsCancelClicked();
            event->accept();
            return;
        }
        else if (event->matches(QKeySequence::Save)) // Ctrl+S
        {
            onDetailsSaveClicked();
            event->accept();
            return;
        }
    }

    // Handle Delete key with batch deletion support
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        QStringList selectedIds = getSelectedEventIds();

        if (!selectedIds.isEmpty())
        {
            if (selectedIds.size() == 1)
            {
                emit deleteEventRequested(selectedIds.first());
            }
            else
            {
                emit batchDeleteRequested(selectedIds);
            }

            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}


void TimelineSidePanel::connectSignals()
{
    // Connect to model signals
    connect(model_, &TimelineModel::eventAdded, this, &TimelineSidePanel::onEventAdded);
    connect(model_, &TimelineModel::eventRemoved, this, &TimelineSidePanel::onEventRemoved);
    connect(model_, &TimelineModel::eventUpdated, this, &TimelineSidePanel::onEventUpdated);
    connect(model_, &TimelineModel::lanesRecalculated, this, &TimelineSidePanel::onLanesRecalculated);
    connect(model_, &TimelineModel::eventArchived, this, &TimelineSidePanel::onEventRemoved);
    connect(model_, &TimelineModel::eventRestored, this, &TimelineSidePanel::onEventAdded);

    // Connect to list widget click signals
    connect(ui->allEventsList, &QListWidget::itemClicked, this, &TimelineSidePanel::onAllEventsItemClicked);
    connect(ui->lookaheadList, &QListWidget::itemClicked, this, &TimelineSidePanel::onLookaheadItemClicked);
    connect(ui->todayList, &QListWidget::itemClicked, this, &TimelineSidePanel::onTodayItemClicked);

    // Connect selection change signals
    connect(ui->allEventsList, &QListWidget::itemSelectionChanged, this, &TimelineSidePanel::onListSelectionChanged);
    connect(ui->lookaheadList, &QListWidget::itemSelectionChanged, this, &TimelineSidePanel::onListSelectionChanged);
    connect(ui->todayList, &QListWidget::itemSelectionChanged, this, &TimelineSidePanel::onListSelectionChanged);

    connect(model_, &TimelineModel::eventAttachmentsChanged, this, &TimelineSidePanel::onEventAttachmentsChanged);
}


void TimelineSidePanel::setupTabBarContextMenu()
{
    // Enable context menu on tab bar
    QTabBar* tabBar = ui->tabWidget->tabBar();
    tabBar->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tabBar, &QTabBar::customContextMenuRequested,
            this, &TimelineSidePanel::onTabBarContextMenuRequested);
}


void TimelineSidePanel::onTabBarContextMenuRequested(const QPoint& pos)
{
    QTabBar* tabBar = ui->tabWidget->tabBar();

    // Find which tab was clicked
    int clickedTabIndex = -1;
    for (int i = 0; i < tabBar->count(); ++i)
    {
        if (tabBar->tabRect(i).contains(pos))
        {
            clickedTabIndex = i;
            break;
        }
    }

    if (clickedTabIndex == -1)
    {
        return; // Click was not on a tab
    }

    // Map to global position for menu display
    QPoint globalPos = tabBar->mapToGlobal(pos);

    // Show appropriate context menu based on which tab was clicked
    // Tab order: Today (0), Lookahead (1), All Events (2)
    switch (clickedTabIndex)
    {
    case 0: // Today tab
        showTodayTabContextMenu(globalPos);
        break;
    case 1: // Lookahead tab
        showLookaheadTabContextMenu(globalPos);
        break;
    case 2: // All Events tab
        showAllEventsTabContextMenu(globalPos);
        break;
    default:
        break;
    }
}


void TimelineSidePanel::showTodayTabContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);

    // Common actions (at top)
    QAction* refreshAction = menu.addAction("🔄 Refresh Tab");
    refreshAction->setShortcut(QKeySequence::Refresh);

    // Sort submenu
    QMenu* sortMenu = menu.addMenu("📊 Sort By");
    QAction* sortDateAction = sortMenu->addAction("📅 Date");
    QAction* sortPriorityAction = sortMenu->addAction("⭐ Priority");
    QAction* sortTypeAction = sortMenu->addAction("🏷️ Type");

    // Mark current sort mode
    sortDateAction->setCheckable(true);
    sortPriorityAction->setCheckable(true);
    sortTypeAction->setCheckable(true);

    switch (currentSortMode_)
    {
    case TimelineSettings::SortMode::ByDate:
        sortDateAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByPriority:
        sortPriorityAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByType:
        sortTypeAction->setChecked(true);
        break;
    }

    // Filter submenu
    QMenu* filterMenu = menu.addMenu("🔍 Filter By Type");
    QAction* filterMeetingAction = filterMenu->addAction("Meeting");
    QAction* filterActionAction = filterMenu->addAction("Action");
    QAction* filterTestAction = filterMenu->addAction("Test Event");
    QAction* filterReminderAction = filterMenu->addAction("Reminder");
    QAction* filterJiraAction = filterMenu->addAction("Jira Ticket");
    filterMenu->addSeparator();
    QAction* filterAllAction = filterMenu->addAction("All Types...");

    // Mark active filters
    filterMeetingAction->setCheckable(true);
    filterActionAction->setCheckable(true);
    filterTestAction->setCheckable(true);
    filterReminderAction->setCheckable(true);
    filterJiraAction->setCheckable(true);

    filterMeetingAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Meeting));
    filterActionAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Action));
    filterTestAction->setChecked(activeFilterTypes_.contains(TimelineEventType_TestEvent));
    filterReminderAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Reminder));
    filterJiraAction->setChecked(activeFilterTypes_.contains(TimelineEventType_JiraTicket));

    menu.addSeparator();

    // Selection and clipboard actions
    QAction* selectAllAction = menu.addAction("✅ Select All Events");
    selectAllAction->setShortcut(QKeySequence::SelectAll);

    QAction* copyAction = menu.addAction("📋 Copy Event Details");
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(!getSelectedEventIds(ui->todayList).isEmpty());

    menu.addSeparator();

    // Date selection actions (existing)
    QAction* setCurrentAction = menu.addAction("📅 Set to Current Date");
    setCurrentAction->setToolTip("Reset to today's actual date");

    QAction* setSpecificAction = menu.addAction("📆 Set to Specific Date...");
    setSpecificAction->setToolTip("Choose a custom date to display");

    menu.addSeparator();

    // Export submenu (existing)
    QMenu* exportMenu = menu.addMenu("📤 Export");
    QAction* exportPdfAction = exportMenu->addAction("Export to PDF");
    QAction* exportCsvAction = exportMenu->addAction("Export to CSV");
    QAction* exportPngAction = exportMenu->addAction("Export Screenshot (PNG)");

    // Execute menu and handle selection
    QAction* selected = menu.exec(globalPos);

    if (selected == refreshAction)
    {
        onRefreshTab();
    }
    else if (selected == sortDateAction)
    {
        onSortByDate();
    }
    else if (selected == sortPriorityAction)
    {
        onSortByPriority();
    }
    else if (selected == sortTypeAction)
    {
        onSortByType();
    }
    else if (selected == filterMeetingAction)
    {
        toggleFilter(TimelineEventType_Meeting);
    }
    else if (selected == filterActionAction)
    {
        toggleFilter(TimelineEventType_Action);
    }
    else if (selected == filterTestAction)
    {
        toggleFilter(TimelineEventType_TestEvent);
    }
    else if (selected == filterReminderAction)
    {
        toggleFilter(TimelineEventType_Reminder);
    }
    else if (selected == filterJiraAction)
    {
        toggleFilter(TimelineEventType_JiraTicket);
    }
    else if (selected == filterAllAction)
    {
        onFilterByType(); // Shows dialog for batch selection
    }
    else if (selected == selectAllAction)
    {
        onSelectAllEvents();
    }
    else if (selected == copyAction)
    {
        onCopyEventDetails();
    }
    else if (selected == setCurrentAction)
    {
        onSetToCurrentDate();
    }
    else if (selected == setSpecificAction)
    {
        onSetToSpecificDate();
    }
    else if (selected == exportPdfAction)
    {
        onExportTodayTab("PDF");
    }
    else if (selected == exportCsvAction)
    {
        onExportTodayTab("CSV");
    }
    else if (selected == exportPngAction)
    {
        onExportTodayTab("PNG");
    }
}


void TimelineSidePanel::showLookaheadTabContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);

    // Common actions (at top)
    QAction* refreshAction = menu.addAction("🔄 Refresh Tab");
    refreshAction->setShortcut(QKeySequence::Refresh);

    // Sort submenu
    QMenu* sortMenu = menu.addMenu("📊 Sort By");
    QAction* sortDateAction = sortMenu->addAction("📅 Date");
    QAction* sortPriorityAction = sortMenu->addAction("⭐ Priority");
    QAction* sortTypeAction = sortMenu->addAction("🏷️ Type");

    sortDateAction->setCheckable(true);
    sortPriorityAction->setCheckable(true);
    sortTypeAction->setCheckable(true);

    switch (currentSortMode_)
    {
    case TimelineSettings::SortMode::ByDate:
        sortDateAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByPriority:
        sortPriorityAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByType:
        sortTypeAction->setChecked(true);
        break;
    }

    // Filter submenu
    QMenu* filterMenu = menu.addMenu("🔍 Filter By Type");
    QAction* filterMeetingAction = filterMenu->addAction("Meeting");
    QAction* filterActionAction = filterMenu->addAction("Action");
    QAction* filterTestAction = filterMenu->addAction("Test Event");
    QAction* filterReminderAction = filterMenu->addAction("Reminder");
    QAction* filterJiraAction = filterMenu->addAction("Jira Ticket");
    QAction* filterAllAction = filterMenu->addAction("All Types");

    filterMeetingAction->setCheckable(true);
    filterActionAction->setCheckable(true);
    filterTestAction->setCheckable(true);
    filterReminderAction->setCheckable(true);
    filterJiraAction->setCheckable(true);

    filterMeetingAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Meeting));
    filterActionAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Action));
    filterTestAction->setChecked(activeFilterTypes_.contains(TimelineEventType_TestEvent));
    filterReminderAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Reminder));
    filterJiraAction->setChecked(activeFilterTypes_.contains(TimelineEventType_JiraTicket));

    menu.addSeparator();

    // Selection and clipboard actions
    QAction* selectAllAction = menu.addAction("✅ Select All Events");
    selectAllAction->setShortcut(QKeySequence::SelectAll);

    QAction* copyAction = menu.addAction("📋 Copy Event Details");
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(!getSelectedEventIds(ui->lookaheadList).isEmpty());

    menu.addSeparator();

    // Range selection action (existing)
    int currentDays = TimelineSettings::instance().lookaheadTabDays();
    QAction* setRangeAction = menu.addAction(QString("📆 Set Lookahead Range (currently %1 days)").arg(currentDays));
    setRangeAction->setToolTip("Configure how many days ahead to display");

    menu.addSeparator();

    // Export submenu (existing)
    QMenu* exportMenu = menu.addMenu("📤 Export");
    QAction* exportPdfAction = exportMenu->addAction("Export to PDF");
    QAction* exportCsvAction = exportMenu->addAction("Export to CSV");
    QAction* exportPngAction = exportMenu->addAction("Export Screenshot (PNG)");

    // Execute menu and handle selection
    QAction* selected = menu.exec(globalPos);

    if (selected == refreshAction)
    {
        onRefreshTab();
    }
    else if (selected == sortDateAction)
    {
        onSortByDate();
    }
    else if (selected == sortPriorityAction)
    {
        onSortByPriority();
    }
    else if (selected == sortTypeAction)
    {
        onSortByType();
    }
    else if (selected == filterMeetingAction)
    {
        toggleFilter(TimelineEventType_Meeting);
    }
    else if (selected == filterActionAction)
    {
        toggleFilter(TimelineEventType_Action);
    }
    else if (selected == filterTestAction)
    {
        toggleFilter(TimelineEventType_TestEvent);
    }
    else if (selected == filterReminderAction)
    {
        toggleFilter(TimelineEventType_Reminder);
    }
    else if (selected == filterJiraAction)
    {
        toggleFilter(TimelineEventType_JiraTicket);
    }
    else if (selected == filterAllAction)
    {
        onFilterByType();
    }
    else if (selected == selectAllAction)
    {
        onSelectAllEvents();
    }
    else if (selected == copyAction)
    {
        onCopyEventDetails();
    }
    else if (selected == setRangeAction)
    {
        onSetLookaheadRange();
    }
    else if (selected == exportPdfAction)
    {
        onExportLookaheadTab("PDF");
    }
    else if (selected == exportCsvAction)
    {
        onExportLookaheadTab("CSV");
    }
    else if (selected == exportPngAction)
    {
        onExportLookaheadTab("PNG");
    }
}


void TimelineSidePanel::showAllEventsTabContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);

    // Common actions (at top)
    QAction* refreshAction = menu.addAction("🔄 Refresh Tab");
    refreshAction->setShortcut(QKeySequence::Refresh);

    // Sort submenu
    QMenu* sortMenu = menu.addMenu("📊 Sort By");
    QAction* sortDateAction = sortMenu->addAction("📅 Date");
    QAction* sortPriorityAction = sortMenu->addAction("⭐ Priority");
    QAction* sortTypeAction = sortMenu->addAction("🏷️ Type");

    sortDateAction->setCheckable(true);
    sortPriorityAction->setCheckable(true);
    sortTypeAction->setCheckable(true);

    switch (currentSortMode_)
    {
    case TimelineSettings::SortMode::ByDate:
        sortDateAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByPriority:
        sortPriorityAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByType:
        sortTypeAction->setChecked(true);
        break;
    }

    // Filter submenu
    QMenu* filterMenu = menu.addMenu("🔍 Filter By Type");
    QAction* filterMeetingAction = filterMenu->addAction("Meeting");
    QAction* filterActionAction = filterMenu->addAction("Action");
    QAction* filterTestAction = filterMenu->addAction("Test Event");
    QAction* filterReminderAction = filterMenu->addAction("Reminder");
    QAction* filterJiraAction = filterMenu->addAction("Jira Ticket");
    QAction* filterAllAction = filterMenu->addAction("All Types");

    filterMeetingAction->setCheckable(true);
    filterActionAction->setCheckable(true);
    filterTestAction->setCheckable(true);
    filterReminderAction->setCheckable(true);
    filterJiraAction->setCheckable(true);

    filterMeetingAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Meeting));
    filterActionAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Action));
    filterTestAction->setChecked(activeFilterTypes_.contains(TimelineEventType_TestEvent));
    filterReminderAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Reminder));
    filterJiraAction->setChecked(activeFilterTypes_.contains(TimelineEventType_JiraTicket));

    menu.addSeparator();

    // Selection and clipboard actions
    QAction* selectAllAction = menu.addAction("✅ Select All Events");
    selectAllAction->setShortcut(QKeySequence::SelectAll);

    QAction* copyAction = menu.addAction("📋 Copy Event Details");
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(!getSelectedEventIds(ui->allEventsList).isEmpty());

    menu.addSeparator();

    // Export submenu (existing)
    QMenu* exportMenu = menu.addMenu("📤 Export");
    QAction* exportPdfAction = exportMenu->addAction("Export to PDF");
    QAction* exportCsvAction = exportMenu->addAction("Export to CSV");
    QAction* exportPngAction = exportMenu->addAction("Export Screenshot (PNG)");

    // Execute menu and handle selection
    QAction* selected = menu.exec(globalPos);

    if (selected == refreshAction)
    {
        onRefreshTab();
    }
    else if (selected == sortDateAction)
    {
        onSortByDate();
    }
    else if (selected == sortPriorityAction)
    {
        onSortByPriority();
    }
    else if (selected == sortTypeAction)
    {
        onSortByType();
    }
    else if (selected == filterMeetingAction)
    {
        toggleFilter(TimelineEventType_Meeting);
    }
    else if (selected == filterActionAction)
    {
        toggleFilter(TimelineEventType_Action);
    }
    else if (selected == filterTestAction)
    {
        toggleFilter(TimelineEventType_TestEvent);
    }
    else if (selected == filterJiraAction)
    {
        toggleFilter(TimelineEventType_JiraTicket);
    }
    else if (selected == filterReminderAction)
    {
        toggleFilter(TimelineEventType_Reminder);
    }
    else if (selected == filterAllAction)
    {
        onFilterByType();
    }
    else if (selected == selectAllAction)
    {
        onSelectAllEvents();
    }
    else if (selected == copyAction)
    {
        onCopyEventDetails();
    }
    else if (selected == exportPdfAction)
    {
        onExportAllEventsTab("PDF");
    }
    else if (selected == exportCsvAction)
    {
        onExportAllEventsTab("CSV");
    }
    else if (selected == exportPngAction)
    {
        onExportAllEventsTab("PNG");
    }
}


void TimelineSidePanel::onEventAttachmentsChanged(const QString& eventId)
{
    if (eventId.isEmpty())
    {
        return;
    }

    // Only refresh the attachments pane if it pertains to the currently displayed event.
    if (eventId == currentEventId_)
    {
        refreshAttachmentsForCurrentEvent();
    }
}


void TimelineSidePanel::onSetToCurrentDate()
{
    // Reset to using current date
    TimelineSettings::instance().setTodayTabUseCustomDate(false);

    // Refresh the Today tab to reflect current date
    refreshTodayTab();
    updateTodayTabLabel();

    QMessageBox::information(this, "Today Tab Reset",
                             QString("Today tab reset to current date: %1")
                                 .arg(QDate::currentDate().toString("yyyy-MM-dd")));
}


void TimelineSidePanel::onSetToSpecificDate()
{
    // Get current custom date if set, otherwise use today
    QDate currentDate = TimelineSettings::instance().todayTabUseCustomDate()
                            ? TimelineSettings::instance().todayTabCustomDate()
                            : QDate::currentDate();

    // Show date selection dialog
    SetTodayDateDialog dialog(
        currentDate,
        model_->versionStartDate(),
        model_->versionEndDate(),
        this
        );

    if (dialog.exec() == QDialog::Accepted)
    {
        QDate selectedDate = dialog.selectedDate();

        // Save custom date settings
        TimelineSettings::instance().setTodayTabUseCustomDate(true);
        TimelineSettings::instance().setTodayTabCustomDate(selectedDate);

        // Refresh the Today tab to show events for selected date
        refreshTodayTab();
        updateTodayTabLabel();

        QMessageBox::information(this, "Today Tab Updated",
                                 QString("Today tab now showing events for: %1")
                                     .arg(selectedDate.toString("yyyy-MM-dd")));
    }
}


void TimelineSidePanel::onExportTodayTab(const QString& format)
{
    QVector<TimelineEvent> events = getEventsFromTab(0); // Today tab is index 0

    if (events.isEmpty())
    {
        QMessageBox::information(this, "No Events",
                                 "There are no events in the Today tab to export.");
        return;
    }

    QString tabName = TimelineSettings::instance().todayTabUseCustomDate()
                          ? QString("UserSetDate_%1").arg(TimelineSettings::instance().todayTabCustomDate().toString("yyyy-MM-dd"))
                          : QString("Today_%1").arg(QDate::currentDate().toString("yyyy-MM-dd"));

    exportEvents(events, format, tabName);
}


void TimelineSidePanel::onSetLookaheadRange()
{
    int currentDays = TimelineSettings::instance().lookaheadTabDays();

    // Show range selection dialog
    SetLookaheadRangeDialog dialog(currentDays, this);

    if (dialog.exec() == QDialog::Accepted)
    {
        int newDays = dialog.selectedDays();

        // Save new range
        TimelineSettings::instance().setLookaheadTabDays(newDays);

        // Refresh the Lookahead tab
        refreshLookaheadTab();
        updateLookaheadTabLabel();

        QDate today = QDate::currentDate();
        QDate endDate = today.addDays(newDays);

        QMessageBox::information(this, "Lookahead Range Updated",
                                 QString("Lookahead tab now showing %1 days (%2 to %3)")
                                     .arg(newDays)
                                     .arg(today.toString("MMM dd"))
                                     .arg(endDate.toString("MMM dd, yyyy")));
    }
}


void TimelineSidePanel::onExportLookaheadTab(const QString& format)
{
    QVector<TimelineEvent> events = getEventsFromTab(1); // Lookahead tab is index 1

    if (events.isEmpty())
    {
        QMessageBox::information(this, "No Events",
                                 "There are no events in the Lookahead tab to export.");
        return;
    }

    int days = TimelineSettings::instance().lookaheadTabDays();
    QString tabName = QString("Lookahead_%1days").arg(days);

    exportEvents(events, format, tabName);
}


void TimelineSidePanel::onExportAllEventsTab(const QString& format)
{
    QVector<TimelineEvent> events = getEventsFromTab(2); // All Events tab is index 2

    if (events.isEmpty())
    {
        QMessageBox::information(this, "No Events",
                                 "There are no events to export.");
        return;
    }

    QString tabName = "AllEvents";

    exportEvents(events, format, tabName);
}


QVector<TimelineEvent> TimelineSidePanel::getEventsFromTab(int tabIndex) const
{
    switch (tabIndex)
    {
    case 0: // Today tab
    {
        QDate targetDate = TimelineSettings::instance().todayTabUseCustomDate()
        ? TimelineSettings::instance().todayTabCustomDate()
        : QDate::currentDate();

        QVector<TimelineEvent> allEvents = model_->getAllEvents();
        QVector<TimelineEvent> filteredEvents;

        for (const auto& event : allEvents)
        {
            if (event.occursOnDate(targetDate))
            {
                filteredEvents.append(event);
            }
        }

        return filteredEvents;
    }

    case 1: // Lookahead tab
    {
        int days = TimelineSettings::instance().lookaheadTabDays();
        return model_->getEventsLookahead(days);
    }

    case 2: // All Events tab
    {
        return model_->getAllEvents();
    }

    default:
        return QVector<TimelineEvent>();
    }
}


bool TimelineSidePanel::exportEvents(const QVector<TimelineEvent>& events,
                                     const QString& format,
                                     const QString& tabName)
{
    QString filter;
    QString defaultName;

    if (format == "PDF")
    {
        filter = "PDF Files (*.pdf)";
        defaultName = QString("timeline_%1.pdf").arg(tabName);
    }
    else if (format == "CSV")
    {
        filter = "CSV Files (*.csv)";
        defaultName = QString("timeline_%1.csv").arg(tabName);
    }
    else if (format == "PNG")
    {
        filter = "PNG Images (*.png)";
        defaultName = QString("timeline_%1.png").arg(tabName);
    }
    else
    {
        qWarning() << "Unknown export format:" << format;
        return false;
    }

    // Show file save dialog
    QString filePath = QFileDialog::getSaveFileName(
        this,
        QString("Export %1 to %2").arg(tabName, format),
        defaultName,
        filter
        );

    if (filePath.isEmpty())
    {
        return false; // User cancelled
    }

    bool success = false;

    if (format == "PDF")
    {
        QString reportTitle = QString("Timeline Export: %1 (%2 events)")
        .arg(tabName)
            .arg(events.size());

        success = TimelineExporter::exportEventsToPDF(events, view_, filePath, true, reportTitle);
    }
    else if (format == "CSV")
    {
        success = TimelineExporter::exportEventsToCSV(events, filePath);
    }
    else if (format == "PNG")
    {
        if (!view_)
        {
            QMessageBox::warning(this, "Export Failed",
                                 "Screenshot export requires a timeline view reference.");
            return false;
        }

        success = TimelineExporter::exportScreenshot(view_, filePath, true);
    }

    if (success)
    {
        QMessageBox::information(this, "Export Successful",
                                 QString("Exported %1 events to:\n%2")
                                     .arg(events.size())
                                     .arg(filePath));
    }
    else
    {
        QMessageBox::critical(this, "Export Failed",
                              QString("Failed to export to %1").arg(format));
    }

    return success;
}


void TimelineSidePanel::updateTodayTabLabel()
{
    QDate targetDate;
    QString labelPrefix;

    if (TimelineSettings::instance().todayTabUseCustomDate())
    {
        targetDate = TimelineSettings::instance().todayTabCustomDate();
        labelPrefix = "User Set";
    }
    else
    {
        targetDate = QDate::currentDate();
        labelPrefix = "Today";
    }

    // Count events on target date
    QVector<TimelineEvent> allEvents = model_->getAllEvents();
    int eventCount = 0;

    for (const auto& event : allEvents)
    {
        if (event.occursOnDate(targetDate))
        {
            eventCount++;
        }
    }

    QString label = QString("%1 (%2) - %3")
                        .arg(labelPrefix)
                        .arg(eventCount)
                        .arg(targetDate.toString("MMM dd"));

    ui->tabWidget->setTabText(0, label);
}


void TimelineSidePanel::updateLookaheadTabLabel()
{
    int days = TimelineSettings::instance().lookaheadTabDays();
    QVector<TimelineEvent> events = model_->getEventsLookahead(days);

    QString label = QString("Lookahead (%1) - %2 days")
                        .arg(events.size())
                        .arg(days);

    ui->tabWidget->setTabText(1, label);
}


void TimelineSidePanel::updateAllEventsTabLabel()
{
    QVector<TimelineEvent> events = model_->getAllEvents();

    QString label = QString("All Events (%1)").arg(events.size());

    ui->tabWidget->setTabText(2, label);
}


void TimelineSidePanel::refreshAllTabs()
{
    refreshAllEventsTab();
    refreshLookaheadTab();
    refreshTodayTab();
}


void TimelineSidePanel::refreshTodayTab()
{
    QDate targetDate = TimelineSettings::instance().todayTabUseCustomDate()
    ? TimelineSettings::instance().todayTabCustomDate()
    : QDate::currentDate();

    QVector<TimelineEvent> allEvents = model_->getAllEvents();
    QVector<TimelineEvent> events;

    for (const auto& event : allEvents)
    {
        if (event.occursOnDate(targetDate))
        {
            events.append(event);
        }
    }

    // Apply sort and filter
    events = applySortAndFilter(events);

    populateListWidget(ui->todayList, events);
    updateTodayTabLabel();
    adjustWidthToFitTabs();
}


void TimelineSidePanel::refreshLookaheadTab()
{
    int days = TimelineSettings::instance().lookaheadTabDays();
    QVector<TimelineEvent> events = model_->getEventsLookahead(days);

    // Apply sort and filter
    events = applySortAndFilter(events);

    populateListWidget(ui->lookaheadList, events);
    updateLookaheadTabLabel();
    adjustWidthToFitTabs();
}


void TimelineSidePanel::refreshAllEventsTab()
{
    QVector<TimelineEvent> events = model_->getAllEvents();

    // Apply sort and filter
    events = applySortAndFilter(events);

    populateListWidget(ui->allEventsList, events);
    updateAllEventsTabLabel();
    adjustWidthToFitTabs();
}


void TimelineSidePanel::populateListWidget(QListWidget* listWidget, const QVector<TimelineEvent>& events)
{
    listWidget->clear();

    if (events.isEmpty())
    {
        auto emptyItem = new QListWidgetItem("No events");
        emptyItem->setFlags(Qt::NoItemFlags);
        emptyItem->setForeground(Qt::gray);
        listWidget->addItem(emptyItem);
        return;
    }

    for (const auto& event : events)
    {
        QListWidgetItem* item = createListItem(event);
        listWidget->addItem(item);
    }
}


QListWidgetItem* TimelineSidePanel::createListItem(const TimelineEvent& event)
{
    auto item = new QListWidgetItem();
    item->setText(formatEventText(event));
    item->setData(Qt::UserRole, event.id);

    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(event.color);
    item->setIcon(QIcon(colorPixmap));

    QString tooltip = QString("%1\n%2\nPriority: %3\nLane: %4")
                          .arg(event.title)
                          .arg(formatEventDateRange(event))
                          .arg(event.priority)
                          .arg(event.lane);

    if (!event.description.isEmpty())
    {
        tooltip += "\n\n" + event.description;
    }

    item->setToolTip(tooltip);
    return item;
}


QString TimelineSidePanel::formatEventText(const TimelineEvent& event) const
{
    QString dateRange = formatEventDateRange(event);
    QString priorityIndicator = (event.priority <= 2) ? "🔴 " : "";

    return QString("%1%2\n%3")
        .arg(priorityIndicator)
        .arg(event.title)
        .arg(dateRange);
}


QString TimelineSidePanel::formatEventDateRange(const TimelineEvent& event) const
{
    if (event.startDate == event.endDate)
    {
        return event.startDate.toString("MMM dd, yyyy");
    }
    else
    {
        return QString("%1 - %2")
        .arg(event.startDate.toString("MMM dd"))
            .arg(event.endDate.toString("MMM dd, yyyy"));
    }
}


void TimelineSidePanel::adjustWidthToFitTabs()
{
    QTabBar* tabBar = ui->tabWidget->tabBar();
    QFontMetrics fm(tabBar->font());

    int totalWidth = 0;

    for (int i = 0; i < ui->tabWidget->count(); ++i)
    {
        QString tabText = ui->tabWidget->tabText(i);
        int textWidth = fm.horizontalAdvance(tabText);
        totalWidth += textWidth + 30;
    }

    totalWidth += 20;
    setMinimumWidth(totalWidth);
}


void TimelineSidePanel::displayEventDetails(const QString& eventId)
{
    // If the user changes selection while editing, warn about unsaved changes
    if (isEditingDetails_ && eventId != currentEventId_ && hasUnsavedChanges_)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Unsaved Changes",
            "You have unsaved changes. Discard them?",
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply != QMessageBox::Yes)
        {
            return; // Stay on current event
        }
    }

    // If the user changes selection while editing, discard inline edits (Cancel behavior).
    if (isEditingDetails_ && eventId != currentEventId_)
    {
        exitEditMode(true);
    }

    currentEventId_ = eventId;

    const TimelineEvent* event = model_->getEvent(eventId);
    if (!event)
    {
        clearEventDetails();

        return;
    }

    // Common Fields (All Event Types)
    ui->titleValueLabel->setText(event->title);
    ui->typeValueLabel->setText(eventTypeToString(event->type));
    ui->priorityValueLabel->setText(QString::number(event->priority));
    ui->laneValueLabel->setText(QString::number(event->lane));

    // Lane Control Field
    if (event->laneControlEnabled)
    {
        ui->laneControlValueLabel->setText("Manual");
        ui->laneControlValueLabel->setStyleSheet("color: #2E7D32; font-weight: bold;");
    }
    else
    {
        ui->laneControlValueLabel->setText("Automatic");
        ui->laneControlValueLabel->setStyleSheet("color: gray;");
    }

    // Clear previous type-specific content
    clearTypeSpecificFields();

    // Build details text starting with description, then type-specific info
    QString detailsText;

    // Description First (Common to all types)
    if (!event->description.isEmpty())
    {
        detailsText = QString("<b>Description:</b><br>%1").arg(event->description);
    }

    // Type-Specific Fields
    QString typeSpecificDetails;

    switch (event->type)
    {
    case TimelineEventType_Meeting:     typeSpecificDetails = buildMeetingDetails(*event);      break;
    case TimelineEventType_Action:      typeSpecificDetails = buildActionDetails(*event);       break;
    case TimelineEventType_TestEvent:   typeSpecificDetails = buildTestEventDetails(*event);    break;
    case TimelineEventType_Reminder:    typeSpecificDetails = buildReminderDetails(*event);     break;
    case TimelineEventType_JiraTicket:  typeSpecificDetails = buildJiraTicketDetails(*event);   break;
    default:                            typeSpecificDetails = buildGenericDetails(*event);      break;
    }

    // Combine description and type-specific details with proper spacing
    if (!typeSpecificDetails.isEmpty())
    {
        if (!detailsText.isEmpty())
        {
            detailsText += "<br><br>";  // Add blank line between description and type-specific
        }
        detailsText += typeSpecificDetails;
    }

    // Set the combined details text & show the details group box
    ui->detailsTextEdit->setHtml(detailsText);
    ui->eventDetailsGroupBox->setVisible(true);

    refreshAttachmentsForCurrentEvent();

    updateDetailsPaneGeometry();
}


QString TimelineSidePanel::buildMeetingDetails(const TimelineEvent& event)
{
    QString details;

    // ========== DATE/TIME SECTION ==========
    details += QString("<b>Date & Time:</b><br>");

    // Check if all-day event (time is 00:00:00 to 23:59:59)
    bool isAllDay = (event.startDate.time() == QTime(0, 0, 0) &&
                     event.endDate.time() == QTime(23, 59, 59));

    if (isAllDay)
    {
        details += QString("Start: %1 (All Day)<br>")
        .arg(event.startDate.toString("yyyy-MM-dd"));
        details += QString("End: %1 (All Day)<br>")
                       .arg(event.endDate.toString("yyyy-MM-dd"));
    }
    else
    {
        details += QString("Start: %1 at %2<br>")
        .arg(event.startDate.toString("yyyy-MM-dd"))
            .arg(event.startDate.time().toString("HH:mm"));
        details += QString("End: %1 at %2<br>")
                       .arg(event.endDate.toString("yyyy-MM-dd"))
                       .arg(event.endDate.time().toString("HH:mm"));
    }

    // Duration calculation
    if (event.startDate.date() == event.endDate.date())
    {
        // Same day - calculate time duration
        qint64 seconds = event.startDate.secsTo(event.endDate);
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;

        if (hours > 0)
        {
            details += QString("Duration: %1h %2m<br>").arg(hours).arg(mins);
        }
        else
        {
            details += QString("Duration: %1m<br>").arg(mins);
        }
    }
    else
    {
        // Multi-day event
        int days = event.startDate.daysTo(event.endDate);
        details += QString("Duration: %1 days<br>").arg(days + 1);
    }

    // ========== MEETING-SPECIFIC FIELDS ==========
    if (!event.location.isEmpty() || !event.participants.isEmpty())
    {
        details += "<br>";  // Add spacing before next section
    }

    if (!event.location.isEmpty())
    {
        details += QString("<b>Location:</b><br>%1<br>").arg(event.location);
    }

    if (!event.participants.isEmpty())
    {
        if (!event.location.isEmpty())
        {
            details += "<br>";  // Spacing between location and participants
        }
        details += QString("<b>Participants:</b><br>%1<br>").arg(event.participants);
    }

    return details;
}



QString TimelineSidePanel::buildActionDetails(const TimelineEvent& event)
{
    QString details;

    // ========== DATE/TIME SECTION ==========
    details += QString("<b>Date & Time:</b><br>");

    // Check if all-day event
    bool isStartAllDay = (event.startDate.time() == QTime(0, 0, 0));

    if (isStartAllDay)
    {
        details += QString("Start: %1 (All Day)<br>")
        .arg(event.startDate.toString("yyyy-MM-dd"));
    }
    else
    {
        details += QString("Start: %1 at %2<br>")
        .arg(event.startDate.toString("yyyy-MM-dd"))
            .arg(event.startDate.time().toString("HH:mm"));
    }

    // Due Date/Time
    if (event.dueDateTime.isValid())
    {
        // Check if due time is midnight (all-day)
        bool isDueAllDay = (event.dueDateTime.time() == QTime(0, 0, 0) ||
                            event.dueDateTime.time() == QTime(23, 59, 59));

        if (isDueAllDay)
        {
            details += QString("Due: %1 (All Day)<br>")
            .arg(event.dueDateTime.toString("yyyy-MM-dd"));
        }
        else
        {
            details += QString("Due: %1 at %2<br>")
            .arg(event.dueDateTime.toString("yyyy-MM-dd"))
                .arg(event.dueDateTime.time().toString("HH:mm"));
        }
    }

    // ========== STATUS SECTION ==========
    if (!event.status.isEmpty())
    {
        details += "<br>";  // Add spacing before status section

        QString statusColor = "black";
        if (event.status == "Completed")
            statusColor = "green";
        else if (event.status == "In Progress")
            statusColor = "blue";
        else if (event.status == "Blocked")
            statusColor = "red";

        details += QString("<b>Status:</b> <span style='color:%1;'>%2</span><br>")
                       .arg(statusColor)
                       .arg(event.status);
    }

    // ========== TIME TRACKING SECTION ==========
    if (event.dueDateTime.isValid())
    {
        details += "<br>";  // Add spacing before time tracking

        QDateTime now = QDateTime::currentDateTime();
        qint64 secsRemaining = now.secsTo(event.dueDateTime);

        if (secsRemaining > 0)
        {
            int days = secsRemaining / 86400;
            int hours = (secsRemaining % 86400) / 3600;

            if (days > 0)
            {
                details += QString("<b>Time Remaining:</b><br>%1 days, %2 hours<br>")
                .arg(days)
                    .arg(hours);
            }
            else if (hours > 0)
            {
                details += QString("<b>Time Remaining:</b><br>%1 hours<br>")
                .arg(hours);
            }
            else
            {
                int mins = secsRemaining / 60;
                details += QString("<b>Time Remaining:</b><br>%1 minutes<br>")
                               .arg(mins);
            }
        }
        else if (secsRemaining < 0)
        {
            int days = (-secsRemaining) / 86400;
            int hours = ((-secsRemaining) % 86400) / 3600;

            if (days > 0)
            {
                details += QString("<b style='color:red;'>Overdue By:</b><br>%1 days, %2 hours<br>")
                .arg(days)
                    .arg(hours);
            }
            else
            {
                details += QString("<b style='color:red;'>Overdue By:</b><br>%1 hours<br>")
                .arg(hours);
            }
        }
        else
        {
            details += QString("<b style='color:orange;'>Due Now!</b><br>");
        }
    }

    return details;
}


QString TimelineSidePanel::buildTestEventDetails(const TimelineEvent& event)
{
    QString details;

    // ========== DATE/TIME SECTION ==========
    details += QString("<b>Date & Time:</b><br>");

    // Check if all-day events
    bool isStartAllDay = (event.startDate.time() == QTime(0, 0, 0));
    bool isEndAllDay = (event.endDate.time() == QTime(23, 59, 59));

    if (isStartAllDay && isEndAllDay)
    {
        details += QString("Start: %1 (All Day)<br>")
        .arg(event.startDate.toString("yyyy-MM-dd"));
        details += QString("End: %1 (All Day)<br>")
                       .arg(event.endDate.toString("yyyy-MM-dd"));
    }
    else
    {
        details += QString("Start: %1 at %2<br>")
        .arg(event.startDate.toString("yyyy-MM-dd"))
            .arg(event.startDate.time().toString("HH:mm"));
        details += QString("End: %1 at %2<br>")
                       .arg(event.endDate.toString("yyyy-MM-dd"))
                       .arg(event.endDate.time().toString("HH:mm"));
    }

    // Duration calculation
    int durationDays = event.startDate.daysTo(event.endDate) + 1;
    details += QString("Duration: %1 days<br>").arg(durationDays);

    // ========== TEST-SPECIFIC FIELDS ==========
    if (!event.testCategory.isEmpty())
    {
        details += "<br>";  // Add spacing before category section
        details += QString("<b>Event Category:</b><br>%1<br>").arg(event.testCategory);
    }

    // ========== PROGRESS SECTION ==========
    if (!event.preparationChecklist.isEmpty())
    {
        details += "<br>";  // Add spacing before progress section

        int completedCount = 0;
        int totalCount = event.preparationChecklist.size();

        for (bool completed : event.preparationChecklist.values())
        {
            if (completed) completedCount++;
        }

        double percentage = (totalCount > 0) ? (completedCount * 100.0 / totalCount) : 0.0;

        details += QString("<b>Preparation Progress:</b><br>%1 / %2 completed (%3%)<br>")
                       .arg(completedCount)
                       .arg(totalCount)
                       .arg(QString::number(percentage, 'f', 0));
    }

    return details;
}


QString TimelineSidePanel::buildReminderDetails(const TimelineEvent& event)
{
    QString details;

    // ========== REMINDER DATE/TIME SECTION ==========
    if (event.reminderDateTime.isValid())
    {
        details += QString("<b>Reminder:</b><br>");
        details += QString("%1 at %2<br>")
                       .arg(event.reminderDateTime.toString("yyyy-MM-dd"))
                       .arg(event.reminderDateTime.time().toString("HH:mm"));
    }

    // ========== EVENT DATE/TIME SECTION ==========
    if (event.startDate.isValid() && event.endDate.isValid())
    {
        details += "<br>";  // Add spacing before event timeline section
        details += QString("<b>Event Date & Time:</b><br>");

        bool isStartAllDay = (event.startDate.time() == QTime(0, 0, 0));
        bool isEndAllDay = (event.endDate.time() == QTime(23, 59, 59));

        if (isStartAllDay && isEndAllDay)
        {
            details += QString("Start: %1 (All Day)<br>")
            .arg(event.startDate.toString("yyyy-MM-dd"));
            details += QString("End: %1 (All Day)<br>")
                           .arg(event.endDate.toString("yyyy-MM-dd"));
        }
        else
        {
            details += QString("Start: %1 at %2<br>")
            .arg(event.startDate.toString("yyyy-MM-dd"))
                .arg(event.startDate.time().toString("HH:mm"));
            details += QString("End: %1 at %2<br>")
                           .arg(event.endDate.toString("yyyy-MM-dd"))
                           .arg(event.endDate.time().toString("HH:mm"));
        }

        // Duration calculation
        int durationDays = event.startDate.daysTo(event.endDate) + 1;
        if (durationDays > 1)
        {
            details += QString("Duration: %1 days<br>").arg(durationDays);
        }
    }

    // ========== RECURRENCE SECTION ==========
    if (!event.recurringRule.isEmpty() && event.recurringRule != "None")
    {
        details += "<br>";  // Add spacing before recurrence section
        details += QString("<b>Recurrence:</b><br>%1<br>").arg(event.recurringRule);
    }

    // ========== TIME TRACKING SECTION ==========
    if (event.reminderDateTime.isValid())
    {
        details += "<br>";  // Add spacing before time tracking

        QDateTime now = QDateTime::currentDateTime();
        qint64 secsUntilReminder = now.secsTo(event.reminderDateTime);

        if (secsUntilReminder > 0)
        {
            int days = secsUntilReminder / 86400;
            int hours = (secsUntilReminder % 86400) / 3600;
            int mins = ((secsUntilReminder % 86400) % 3600) / 60;

            if (days > 0)
            {
                details += QString("<b>Time Until Reminder:</b><br>%1 days, %2 hours, %3 minutes<br>")
                .arg(days)
                    .arg(hours)
                    .arg(mins);
            }
            else if (hours > 0)
            {
                details += QString("<b>Time Until Reminder:</b><br>%1 hours, %2 minutes<br>")
                .arg(hours)
                    .arg(mins);
            }
            else
            {
                details += QString("<b>Time Until Reminder:</b><br>%1 minutes<br>").arg(mins);
            }
        }
        else if (secsUntilReminder < 0)
        {
            details += QString("<b style='color:red;'>Reminder Past Due</b><br>");
        }
        else
        {
            details += QString("<b style='color:orange;'>Reminder is NOW!</b><br>");
        }
    }

    return details;
}


QString TimelineSidePanel::buildJiraTicketDetails(const TimelineEvent& event)
{
    QString details;

    // ========== DATE/TIME SECTION ==========
    details += QString("<b>Date & Time:</b><br>");

    // Check if all-day events
    bool isStartAllDay = (event.startDate.time() == QTime(0, 0, 0));
    bool isEndAllDay = (event.endDate.time() == QTime(23, 59, 59));

    if (isStartAllDay)
    {
        details += QString("Start: %1 (All Day)<br>")
        .arg(event.startDate.toString("yyyy-MM-dd"));
    }
    else
    {
        details += QString("Start: %1 at %2<br>")
        .arg(event.startDate.toString("yyyy-MM-dd"))
            .arg(event.startDate.time().toString("HH:mm"));
    }

    if (isEndAllDay)
    {
        details += QString("Due: %1 (All Day)<br>")
        .arg(event.endDate.toString("yyyy-MM-dd"));
    }
    else
    {
        details += QString("Due: %1 at %2<br>")
        .arg(event.endDate.toString("yyyy-MM-dd"))
            .arg(event.endDate.time().toString("HH:mm"));
    }

    // Duration calculation
    int durationDays = event.startDate.daysTo(event.endDate) + 1;
    details += QString("Duration: %1 days<br>").arg(durationDays);

    // ========== JIRA-SPECIFIC FIELDS ==========
    if (!event.jiraKey.isEmpty() || !event.jiraType.isEmpty())
    {
        details += "<br>";  // Add spacing before Jira section
    }

    if (!event.jiraKey.isEmpty())
    {
        details += QString("<b>Jira Key:</b> %1<br>").arg(event.jiraKey);
    }

    if (!event.jiraType.isEmpty())
    {
        QString typeIcon = "📝";
        if (event.jiraType == "Bug")
            typeIcon = "🐛";
        else if (event.jiraType == "Story")
            typeIcon = "📖";
        else if (event.jiraType == "Epic")
            typeIcon = "🎯";
        else if (event.jiraType == "Task")
            typeIcon = "✅";

        if (!event.jiraKey.isEmpty())
        {
            details += "<br>";  // Spacing between key and type
        }
        details += QString("<b>Type:</b> %1 %2<br>").arg(typeIcon).arg(event.jiraType);
    }

    // ========== STATUS SECTION ==========
    if (!event.jiraStatus.isEmpty())
    {
        details += "<br>";  // Add spacing before status section

        QString statusColor = "black";
        if (event.jiraStatus == "Done")
            statusColor = "green";
        else if (event.jiraStatus == "In Progress")
            statusColor = "blue";

        details += QString("<b>Status:</b> <span style='color:%1;'>%2</span><br>")
                       .arg(statusColor)
                       .arg(event.jiraStatus);
    }

    // ========== TIME TRACKING SECTION ==========
    details += "<br>";  // Add spacing before time tracking

    QDate today = QDate::currentDate();
    int daysRemaining = today.daysTo(event.endDate.date());

    if (daysRemaining > 0)
    {
        details += QString("<b>Days Remaining:</b> %1<br>").arg(daysRemaining);
    }
    else if (daysRemaining < 0)
    {
        details += QString("<b style='color:red;'>Overdue By:</b> %1 days<br>").arg(-daysRemaining);
    }
    else
    {
        details += QString("<b style='color:orange;'>Due Today!</b><br>");
    }

    return details;
}


QString TimelineSidePanel::buildGenericDetails(const TimelineEvent& event)
{
    QString details;

    // ========== DATE/TIME SECTION ==========
    details += QString("<b>Date & Time:</b><br>");

    // Check if all-day events
    bool isStartAllDay = (event.startDate.time() == QTime(0, 0, 0));
    bool isEndAllDay = (event.endDate.time() == QTime(23, 59, 59));

    if (isStartAllDay && isEndAllDay)
    {
        details += QString("Start: %1 (All Day)<br>")
        .arg(event.startDate.toString("yyyy-MM-dd"));
        details += QString("End: %1 (All Day)<br>")
                       .arg(event.endDate.toString("yyyy-MM-dd"));
    }
    else
    {
        if (isStartAllDay)
        {
            details += QString("Start: %1 (All Day)<br>")
            .arg(event.startDate.toString("yyyy-MM-dd"));
        }
        else
        {
            details += QString("Start: %1 at %2<br>")
            .arg(event.startDate.toString("yyyy-MM-dd"))
                .arg(event.startDate.time().toString("HH:mm"));
        }

        if (isEndAllDay)
        {
            details += QString("End: %1 (All Day)<br>")
            .arg(event.endDate.toString("yyyy-MM-dd"));
        }
        else
        {
            details += QString("End: %1 at %2<br>")
            .arg(event.endDate.toString("yyyy-MM-dd"))
                .arg(event.endDate.time().toString("HH:mm"));
        }
    }

    // Duration calculation
    int durationDays = event.startDate.daysTo(event.endDate) + 1;
    if (durationDays > 1)
    {
        details += QString("Duration: %1 days<br>").arg(durationDays);
    }

    return details;
}


void TimelineSidePanel::clearTypeSpecificFields()
{
    // Clear any dynamically created labels/widgets for type-specific fields
    // This ensures clean slate before displaying new event details

    // If using dynamic labels, remove them here
    // For simplicity, we'll use a rich text display approach
}


void TimelineSidePanel::updateEventDetails(const TimelineEvent& event)
{
    ui->titleValueLabel->setText(event.title);

    QString typeText;

    switch (event.type)
    {
    case TimelineEventType_Meeting: typeText = "Meeting"; break;
    case TimelineEventType_Action: typeText = "Action"; break;
    case TimelineEventType_TestEvent: typeText = "Test Event"; break;
    case TimelineEventType_Reminder: typeText = "Reminder"; break;
    case TimelineEventType_JiraTicket: typeText = "Jira Ticket"; break;
    default: typeText = "Unknown"; break;
    }
    ui->typeValueLabel->setText(typeText);

    ui->priorityValueLabel->setText(QString::number(event.priority));
    ui->laneValueLabel->setText(QString::number(event.lane));

    // Build combined details text
    QString detailsText;

    // Add date range
    QString dateRange;
    if (event.startDate == event.endDate)
    {
        dateRange = event.startDate.toString("MMM dd, yyyy");
    }
    else
    {
        dateRange = QString("%1 to %2")
        .arg(event.startDate.toString("MMM dd, yyyy"))
            .arg(event.endDate.toString("MMM dd, yyyy"));
    }
    detailsText = QString("<b>Date Range:</b><br>%1").arg(dateRange);

    // Add description if present
    if (!event.description.isEmpty())
    {
        detailsText += QString("\n\n<b>Description:</b><br>%1").arg(event.description);
    }

    ui->detailsTextEdit->setHtml(detailsText);

    ui->eventDetailsGroupBox->setVisible(true);
}


void TimelineSidePanel::clearEventDetails()
{
    if (isEditingDetails_)
    {
        exitEditMode(true);
    }

    currentEventId_.clear();

    ui->titleValueLabel->clear();
    ui->typeValueLabel->clear();
    ui->priorityValueLabel->clear();
    ui->laneValueLabel->clear();
    ui->laneControlValueLabel->clear();
    ui->detailsTextEdit->clear();

    ui->eventDetailsGroupBox->setVisible(false);

    // Attachments section stays visible per design, but is disabled with no selection.
    if (attachmentsSection_)
    {
        attachmentsSection_->setVisible(true);
    }

    if (attachmentsWidget_)
    {
        attachmentsWidget_->setEnabled(false);
    }

    if (attachmentsEmptyStateLabel_)
    {
        attachmentsEmptyStateLabel_->setText("Select an event to view attachments.");
        attachmentsEmptyStateLabel_->setVisible(true);
    }

    if (attachmentsCountBadge_)
    {
        attachmentsCountBadge_->setText("0");
        attachmentsCountBadge_->setEnabled(false);
    }

    updateDetailsPaneGeometry();
}



void TimelineSidePanel::onEventAdded(const QString& /*eventId*/)
{
    refreshAllTabs();
}


void TimelineSidePanel::onEventRemoved(const QString& /*eventId*/)
{
    refreshAllTabs();
}


void TimelineSidePanel::onEventUpdated(const QString& /*eventId*/)
{
    refreshAllTabs();
}


void TimelineSidePanel::onLanesRecalculated()
{
    refreshAllTabs();
}


void TimelineSidePanel::onAllEventsItemClicked(QListWidgetItem* item)
{
    if (item && item->flags() != Qt::NoItemFlags)
    {
        QString eventId = item->data(Qt::UserRole).toString();

        if (!eventId.isEmpty())
        {
            displayEventDetails(eventId);
            emit eventSelected(eventId);
        }
    }
}


void TimelineSidePanel::onLookaheadItemClicked(QListWidgetItem* item)
{
    if (item && item->flags() != Qt::NoItemFlags)
    {
        QString eventId = item->data(Qt::UserRole).toString();
        if (!eventId.isEmpty())
        {
            displayEventDetails(eventId);
            emit eventSelected(eventId);
        }
    }
}


void TimelineSidePanel::onTodayItemClicked(QListWidgetItem* item)
{
    if (item && item->flags() != Qt::NoItemFlags)
    {
        QString eventId = item->data(Qt::UserRole).toString();

        if (!eventId.isEmpty())
        {
            displayEventDetails(eventId);
            emit eventSelected(eventId);
        }
    }
}


void TimelineSidePanel::setupListWidgetContextMenu(QListWidget* listWidget)
{
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(listWidget, &QListWidget::customContextMenuRequested,
            this, &TimelineSidePanel::onListContextMenuRequested);
}


void TimelineSidePanel::onListContextMenuRequested(const QPoint& pos)
{
    QListWidget* listWidget = qobject_cast<QListWidget*>(sender());

    if (!listWidget)
    {
        return;
    }

    showListItemContextMenu(listWidget, pos);
}


void TimelineSidePanel::showListItemContextMenu(QListWidget* listWidget, const QPoint& pos)
{
    QListWidgetItem* item = listWidget->itemAt(pos);

    if (!item || item->flags() == Qt::NoItemFlags)
    {
        return;
    }

    QString eventId = item->data(Qt::UserRole).toString();

    if (eventId.isEmpty())
    {
        return;
    }

    QMenu menu(this);
    QAction* editAction = menu.addAction("✏️ Edit Event");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("🗑️ Delete Event");

    QFont deleteFont = deleteAction->font();
    deleteFont.setBold(true);
    deleteAction->setFont(deleteFont);

    QAction* selected = menu.exec(listWidget->mapToGlobal(pos));

    if (selected == editAction)
    {
        emit editEventRequested(eventId);
    }
    else if (selected == deleteAction)
    {
        emit deleteEventRequested(eventId);
    }
}


void TimelineSidePanel::enableMultiSelection(QListWidget* listWidget)
{
    if (!listWidget)
    {
        return;
    }

    listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
}


QStringList TimelineSidePanel::getSelectedEventIds(QListWidget* listWidget) const
{
    QStringList eventIds;

    if (!listWidget)
    {
        return eventIds;
    }

    QList<QListWidgetItem*> selectedItems = listWidget->selectedItems();

    for (QListWidgetItem* item : selectedItems)
    {
        if (item && item->flags() != Qt::NoItemFlags)
        {
            QString eventId = item->data(Qt::UserRole).toString();
            if (!eventId.isEmpty())
            {
                eventIds.append(eventId);
            }
        }
    }

    return eventIds;
}


QStringList TimelineSidePanel::getSelectedEventIds() const
{
    QStringList eventIds;

    QWidget* currentWidget = ui->tabWidget->currentWidget();

    QListWidget* activeList = nullptr;

    if (currentWidget == ui->todayTab)
    {
        activeList = ui->todayList;
    }
    else if (currentWidget == ui->lookaheadTab)
    {
        activeList = ui->lookaheadList;
    }
    else if (currentWidget == ui->allEventsTab)
    {
        activeList = ui->allEventsList;
    }

    if (activeList)
    {
        eventIds = getSelectedEventIds(activeList);
    }

    return eventIds;
}


void TimelineSidePanel::onListSelectionChanged()
{
    emit selectionChanged();
}


void TimelineSidePanel::focusNextItem(QListWidget* listWidget, int deletedRow)
{
    if (!listWidget || listWidget->count() == 0)
    {
        return;
    }

    int nextRow = deletedRow;

    if (nextRow >= listWidget->count())
    {
        nextRow = listWidget->count() - 1;
    }

    if (nextRow >= 0 && nextRow < listWidget->count())
    {
        QListWidgetItem* nextItem = listWidget->item(nextRow);
        if (nextItem)
        {
            listWidget->setCurrentItem(nextItem);
            nextItem->setSelected(true);

            QString eventId = nextItem->data(Qt::UserRole).toString();
            if (!eventId.isEmpty())
            {
                displayEventDetails(eventId);
            }
        }
    }
}


void TimelineSidePanel::toggleFilter(TimelineEventType type)
{
    if (activeFilterTypes_.contains(type))
    {
        activeFilterTypes_.remove(type);
    }
    else
    {
        activeFilterTypes_.insert(type);
    }

    // Save to settings
    TimelineSettings::instance().setSidePanelFilterTypes(activeFilterTypes_);

    // Refresh current tab
    onRefreshTab();
}


void TimelineSidePanel::onRefreshTab()
{
    // Refresh currently active tab
    QWidget* currentWidget = ui->tabWidget->currentWidget();

    if (currentWidget == ui->todayTab)
    {
        refreshTodayTab();
    }
    else if (currentWidget == ui->lookaheadTab)
    {
        refreshLookaheadTab();
    }
    else if (currentWidget == ui->allEventsTab)
    {
        refreshAllEventsTab();
    }
}


void TimelineSidePanel::onSortByDate()
{
    currentSortMode_ = TimelineSettings::SortMode::ByDate;
    TimelineSettings::instance().setSidePanelSortMode(currentSortMode_);
    refreshAllTabs();
}


void TimelineSidePanel::onSortByPriority()
{
    currentSortMode_ = TimelineSettings::SortMode::ByPriority;
    TimelineSettings::instance().setSidePanelSortMode(currentSortMode_);
    refreshAllTabs();
}


void TimelineSidePanel::onSortByType()
{
    currentSortMode_ = TimelineSettings::SortMode::ByType;
    TimelineSettings::instance().setSidePanelSortMode(currentSortMode_);
    refreshAllTabs();
}


void TimelineSidePanel::onFilterByType()
{
    // Show dialog to select multiple types at once
    QDialog dialog(this);
    dialog.setWindowTitle("Filter Events by Type");
    dialog.setMinimumWidth(300);

    auto layout = new QVBoxLayout(&dialog);

    layout->addWidget(new QLabel("Select event types to display:"));

    QCheckBox* meetingCheck = new QCheckBox("Meeting");
    QCheckBox* actionCheck = new QCheckBox("Action");
    QCheckBox* testCheck = new QCheckBox("Test Event");
    QCheckBox* reminderCheck = new QCheckBox("Reminder");
    QCheckBox* jiraCheck = new QCheckBox("Jira Ticket");

    meetingCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_Meeting));
    actionCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_Action));
    testCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_TestEvent));
    reminderCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_Reminder));
    jiraCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_JiraTicket));

    layout->addWidget(meetingCheck);
    layout->addWidget(actionCheck);
    layout->addWidget(testCheck);
    layout->addWidget(reminderCheck);
    layout->addWidget(jiraCheck);

    layout->addStretch();

    auto buttonLayout = new QHBoxLayout();
    auto selectAllButton = new QPushButton("Select All");
    auto clearAllButton = new QPushButton("Clear All");
    auto okButton = new QPushButton("OK");
    auto cancelButton = new QPushButton("Cancel");

    buttonLayout->addWidget(selectAllButton);
    buttonLayout->addWidget(clearAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    connect(selectAllButton, &QPushButton::clicked, [&]() {
        meetingCheck->setChecked(true);
        actionCheck->setChecked(true);
        testCheck->setChecked(true);
        reminderCheck->setChecked(true);
        jiraCheck->setChecked(true);
    });

    connect(clearAllButton, &QPushButton::clicked, [&]() {
        meetingCheck->setChecked(false);
        actionCheck->setChecked(false);
        testCheck->setChecked(false);
        reminderCheck->setChecked(false);
        jiraCheck->setChecked(false);
    });

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        activeFilterTypes_.clear();

        if (meetingCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_Meeting);
        if (actionCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_Action);
        if (testCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_TestEvent);
        if (reminderCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_Reminder);
        if (jiraCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_JiraTicket);

        // Save to settings
        TimelineSettings::instance().setSidePanelFilterTypes(activeFilterTypes_);

        // Refresh all tabs
        refreshAllTabs();
    }
}

void TimelineSidePanel::onSelectAllEvents()
{
    QWidget* currentWidget = ui->tabWidget->currentWidget();

    QListWidget* activeList = nullptr;

    if (currentWidget == ui->todayTab)
    {
        activeList = ui->todayList;
    }
    else if (currentWidget == ui->lookaheadTab)
    {
        activeList = ui->lookaheadList;
    }
    else if (currentWidget == ui->allEventsTab)
    {
        activeList = ui->allEventsList;
    }

    if (activeList)
    {
        activeList->selectAll();
    }
}


void TimelineSidePanel::onCopyEventDetails()
{
    QStringList selectedIds = getSelectedEventIds();

    if (selectedIds.isEmpty())
    {
        return;
    }

    QStringList eventDetails;

    for (const QString& eventId : selectedIds)
    {
        const TimelineEvent* event = model_->getEvent(eventId);
        if (event)
        {
            QString details = QString(
                                  "Title: %1\n"
                                  "Type: %2\n"
                                  "Dates: %3 to %4\n"
                                  "Priority: %5\n"
                                  "Lane: %6\n"
                                  "Description: %7\n"
                                  ).arg(event->title)
                                  .arg(eventTypeToString(event->type))
                                  .arg(event->startDate.toString("yyyy-MM-dd"))
                                  .arg(event->endDate.toString("yyyy-MM-dd"))
                                  .arg(event->priority)
                                  .arg(event->lane)
                                  .arg(event->description.isEmpty() ? "(No description)" : event->description);

            eventDetails.append(details);
        }
    }

    QString fullDetails = eventDetails.join("\n---\n");
    QGuiApplication::clipboard()->setText(fullDetails);
}


void TimelineSidePanel::sortEvents(QVector<TimelineEvent>& events) const
{
    switch (currentSortMode_)
    {
    case TimelineSettings::SortMode::ByDate:
        std::sort(events.begin(), events.end(),
                  [](const TimelineEvent& a, const TimelineEvent& b) {
                      return a.startDate < b.startDate;
                  });
        break;

    case TimelineSettings::SortMode::ByPriority:
        std::sort(events.begin(), events.end(),
                  [](const TimelineEvent& a, const TimelineEvent& b) {
                      if (a.priority != b.priority)
                      {
                          return a.priority < b.priority; // Lower number = higher priority
                      }
                      return a.startDate < b.startDate; // Tie-breaker
                  });
        break;

    case TimelineSettings::SortMode::ByType:
        std::sort(events.begin(), events.end(),
                  [](const TimelineEvent& a, const TimelineEvent& b) {
                      if (a.type != b.type)
                      {
                          return a.type < b.type;
                      }
                      return a.startDate < b.startDate; // Tie-breaker
                  });
        break;
    }
}

QVector<TimelineEvent> TimelineSidePanel::filterEvents(const QVector<TimelineEvent>& events) const
{
    QVector<TimelineEvent> filtered;

    for (const auto& event : events)
    {
        if (activeFilterTypes_.contains(event.type))
        {
            filtered.append(event);
        }
    }

    return filtered;
}


QVector<TimelineEvent> TimelineSidePanel::applySortAndFilter(const QVector<TimelineEvent>& events) const
{
    // First filter
    QVector<TimelineEvent> result = filterEvents(events);

    // Then sort
    sortEvents(result);

    return result;
}


QString TimelineSidePanel::sortModeToString(TimelineSettings::SortMode mode) const
{
    switch (mode)
    {
    case TimelineSettings::SortMode::ByDate: return "Date";
    case TimelineSettings::SortMode::ByPriority: return "Priority";
    case TimelineSettings::SortMode::ByType: return "Type";
    default: return "Unknown";
    }
}


QString TimelineSidePanel::eventTypeToString(TimelineEventType type) const
{
    switch (type)
    {
    case TimelineEventType_Meeting:
        return "Meeting";
    case TimelineEventType_Action:
        return "Action";
    case TimelineEventType_TestEvent:
        return "Test Event";
    case TimelineEventType_Reminder:
        return "Reminder";
    case TimelineEventType_JiraTicket:
        return "Jira Ticket";
    default:
        return "Unknown";
    }
}


void TimelineSidePanel::onToggleDetailsSection()
{
    detailsExpanded_ = !detailsExpanded_;

    // Toggle visibility of details widgets
    setWidgetVisible(ui->titleLabel, detailsExpanded_);
    setWidgetVisible(ui->titleValueLabel, detailsExpanded_);
    setWidgetVisible(ui->typeLabel, detailsExpanded_);
    setWidgetVisible(ui->typeValueLabel, detailsExpanded_);
    setWidgetVisible(ui->priorityLabel, detailsExpanded_);
    setWidgetVisible(ui->priorityValueLabel, detailsExpanded_);
    setWidgetVisible(ui->laneLabel, detailsExpanded_);
    setWidgetVisible(ui->laneValueLabel, detailsExpanded_);
    setWidgetVisible(ui->laneControlLabel, detailsExpanded_);
    setWidgetVisible(ui->laneControlValueLabel, detailsExpanded_);
    setWidgetVisible(ui->detailsLabel, detailsExpanded_);
    setWidgetVisible(ui->detailsTextEdit, detailsExpanded_);

    // Update toggle button text
    if (toggleDetailsButton_)
    {
        toggleDetailsButton_->setText(detailsExpanded_ ? "▼ Event Details" : "▶ Event Details");
    }

    // Collapse the *scroll area* height so attachments shift up ---
    // Try to find the scroll area by common names. (Keeps this robust across .ui changes.)
    QScrollArea* detailsScrollArea = nullptr;

    if (ui && ui->eventDetailsGroupBox)
    {
        detailsScrollArea = ui->eventDetailsGroupBox->findChild<QScrollArea*>("detailsScrollArea");
    }

    // If your .ui exposes it directly (some setups do), use that too:
    // (If it doesn't exist, this line won't compile — so only add it if your ui has it.)
    // detailsScrollArea = ui->detailsScrollArea;

    if (detailsScrollArea && toggleDetailsButton_)
    {
        if (!detailsExpanded_)
        {
            // Collapse to header-only height
            const int headerH = toggleDetailsButton_->sizeHint().height();
            const int collapsedH = headerH + 10; // a little padding

            detailsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            detailsScrollArea->setMinimumHeight(collapsedH);
            detailsScrollArea->setMaximumHeight(collapsedH);
        }
        else
        {
            // Restore normal behavior
            detailsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            detailsScrollArea->setMinimumHeight(0);
            detailsScrollArea->setMaximumHeight(QWIDGETSIZE_MAX);
        }

        // Force layout to recompute so attachments moves immediately
        detailsScrollArea->updateGeometry();

        updateDetailsPaneGeometry();
    }
}


void TimelineSidePanel::onToggleTypeSpecificSection()
{
    typeSpecificExpanded_ = !typeSpecificExpanded_;

    // Toggle visibility of type-specific stack
    if (typeSpecificStack_)
    {
        typeSpecificStack_->setVisible(typeSpecificExpanded_);
    }

    // Update toggle button text
    if (toggleTypeSpecificButton_)
    {
        toggleTypeSpecificButton_->setText(typeSpecificExpanded_ ? "▼ Type-Specific Details" : "▶ Type-Specific Details");
    }
}


void TimelineSidePanel::onToggleAttachmentsSection()
{
    attachmentsExpanded_ = !attachmentsExpanded_;

    if (toggleAttachmentsButton_)
    {
        toggleAttachmentsButton_->setText(attachmentsExpanded_ ? "▼ Attachments" : "▶ Attachments");
    }

    refreshAttachmentsForCurrentEvent();
    updateDetailsPaneGeometry();
}




bool TimelineSidePanel::validateEditFields()
{
    clearFieldHighlights();

    // Validate title
    if (titleEdit_ && titleEdit_->text().trimmed().isEmpty())
    {
        highlightInvalidField(titleEdit_, "Title cannot be empty");
        return false;
    }

    // Validate dates
    if (startDateTimeEdit_ && endDateTimeEdit_)
    {
        if (startDateTimeEdit_->dateTime() > endDateTimeEdit_->dateTime())
        {
            highlightInvalidField(endDateTimeEdit_, "End date must be after start date");
            return false;
        }
    }

    // Type-specific validation
    if (typeCombo_)
    {
        TimelineEventType type = static_cast<TimelineEventType>(typeCombo_->currentData().toInt());

        switch (type)
        {
        case TimelineEventType_Action:
            if (actionDueDateTimeEdit_ && startDateTimeEdit_)
            {
                if (actionDueDateTimeEdit_->dateTime() < startDateTimeEdit_->dateTime())
                {
                    highlightInvalidField(actionDueDateTimeEdit_, "Due date cannot be before start date");
                    return false;
                }
            }
            break;

        case TimelineEventType_Reminder:
            if (reminderDateTimeEdit_ && !reminderDateTimeEdit_->dateTime().isValid())
            {
                highlightInvalidField(reminderDateTimeEdit_, "Reminder date/time is required");
                return false;
            }
            break;

        case TimelineEventType_JiraTicket:
            if (jiraKeyEdit_ && jiraKeyEdit_->text().trimmed().isEmpty())
            {
                highlightInvalidField(jiraKeyEdit_, "Jira key is required for Jira tickets");
                return false;
            }
            break;

        default:
            break;
        }
    }

    return true;
}


void TimelineSidePanel::highlightInvalidField(QWidget* field, const QString& message)
{
    if (!field) return;

    // Preserve original style once
    if (!originalFieldStyles_.contains(field))
    {
        originalFieldStyles_.insert(field, field->styleSheet());
    }

    field->setStyleSheet(
        "border: 2px solid #d9534f;"
        "background-color: #fff0f0;"
        );

    field->setFocus();
    QToolTip::showText(field->mapToGlobal(QPoint(0, field->height())), message, field);
}


void TimelineSidePanel::clearFieldHighlights()
{
    QList<QWidget*> fieldsToCheck = {
        titleEdit_,
        startDateTimeEdit_,
        endDateTimeEdit_,
        prioritySpin_,
        descriptionEdit_,
        actionDueDateTimeEdit_,
        reminderDateTimeEdit_,
        jiraKeyEdit_
    };

    for (QWidget* field : fieldsToCheck)
    {
        if (!field) continue;

        if (originalFieldStyles_.contains(field))
        {
            field->setStyleSheet(originalFieldStyles_.value(field));
        }
        else
        {
            // If we never overrode it, leave it alone.
        }
    }
}

