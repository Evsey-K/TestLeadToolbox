// EventDetailsWidget.cpp

#include "EventDetailsWidget.h"
#include <QMessageBox>
#include <QDebug>
#include <QStyle>

EventDetailsWidget::EventDetailsWidget(TimelineModel* model, QWidget* parent)
    : QWidget(parent)
    , model_(model)
    , isEditMode_(false)
    , hasUnsavedChanges_(false)
    , descriptionExpanded_(false)
    , typeSpecificExpanded_(false)
    , attachmentsExpanded_(true)  // Attachments expanded by default
{
    setupUi();
    setupConnections();
    applyProfessionalStyling();
    clear();
}

EventDetailsWidget::~EventDetailsWidget()
{
}

void EventDetailsWidget::setupUi()
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(8, 8, 8, 8);
    mainLayout_->setSpacing(8);

    // Create scroll area for all content
    scrollArea_ = new QScrollArea();
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    contentWidget_ = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget_);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setSpacing(8);

    // Create sections
    createHeaderSection();
    createCommonFieldsSection();
    createTypeSpecificSection();
    createDescriptionSection();
    createAttachmentsSection();

    contentLayout->addWidget(headerFrame_);
    contentLayout->addWidget(descriptionGroup_);
    contentLayout->addWidget(typeSpecificGroup_);
    contentLayout->addWidget(attachmentsGroup_);
    contentLayout->addStretch();

    scrollArea_->setWidget(contentWidget_);
    mainLayout_->addWidget(scrollArea_);
}

void EventDetailsWidget::createHeaderSection()
{
    headerFrame_ = new QFrame();
    headerFrame_->setFrameShape(QFrame::StyledPanel);
    headerFrame_->setFrameShadow(QFrame::Raised);

    QHBoxLayout* headerLayout = new QHBoxLayout(headerFrame_);
    headerLayout->setContentsMargins(8, 6, 8, 6);

    // Type badge
    typeBadgeLabel_ = new QLabel("No Event Selected");
    QFont badgeFont = typeBadgeLabel_->font();
    badgeFont.setBold(true);
    badgeFont.setPointSize(badgeFont.pointSize() + 1);
    typeBadgeLabel_->setFont(badgeFont);
    headerLayout->addWidget(typeBadgeLabel_);

    headerLayout->addStretch();

    // Action buttons
    fullEditButton_ = new QPushButton("Full Edit");
    fullEditButton_->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    fullEditButton_->setToolTip("Open full edit dialog with all options");
    fullEditButton_->setMaximumWidth(100);

    saveButton_ = new QPushButton("Save");
    saveButton_->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    saveButton_->setVisible(false);
    saveButton_->setMaximumWidth(80);

    cancelButton_ = new QPushButton("Cancel");
    cancelButton_->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    cancelButton_->setVisible(false);
    cancelButton_->setMaximumWidth(80);

    headerLayout->addWidget(saveButton_);
    headerLayout->addWidget(cancelButton_);
    headerLayout->addWidget(fullEditButton_);
}

void EventDetailsWidget::createCommonFieldsSection()
{
    // Common fields are in a form layout embedded in the description group
    // We'll actually put them directly in the content widget for always-visible access

    commonFieldsLayout_ = new QFormLayout();
    commonFieldsLayout_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    commonFieldsLayout_->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    commonFieldsLayout_->setSpacing(6);

    // Title
    titleEdit_ = new QLineEdit();
    titleEdit_->setPlaceholderText("Event title...");
    titleEdit_->setReadOnly(true);
    commonFieldsLayout_->addRow("Title:", titleEdit_);

    // Start Date/Time
    startDateTimeEdit_ = new QDateTimeEdit();
    startDateTimeEdit_->setCalendarPopup(true);
    startDateTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    startDateTimeEdit_->setReadOnly(true);
    commonFieldsLayout_->addRow("Start:", startDateTimeEdit_);

    // End Date/Time
    endDateTimeEdit_ = new QDateTimeEdit();
    endDateTimeEdit_->setCalendarPopup(true);
    endDateTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    endDateTimeEdit_->setReadOnly(true);
    commonFieldsLayout_->addRow("End:", endDateTimeEdit_);

    // Priority
    prioritySpinner_ = new QSpinBox();
    prioritySpinner_->setRange(0, 5);
    prioritySpinner_->setToolTip("Priority (0 = Highest, 5 = Lowest)");
    prioritySpinner_->setReadOnly(true);
    commonFieldsLayout_->addRow("Priority:", prioritySpinner_);

    // Lane Control
    QHBoxLayout* laneLayout = new QHBoxLayout();
    laneControlCheckbox_ = new QCheckBox("Manual");
    laneControlCheckbox_->setEnabled(false);
    laneSpinner_ = new QSpinBox();
    laneSpinner_->setRange(0, 20);
    laneSpinner_->setEnabled(false);
    laneSpinner_->setReadOnly(true);
    laneLayout->addWidget(laneControlCheckbox_);
    laneLayout->addWidget(laneSpinner_);
    laneLayout->addStretch();
    commonFieldsLayout_->addRow("Lane:", laneLayout);

    // Lock State
    QHBoxLayout* lockLayout = new QHBoxLayout();
    fixedCheckbox_ = new QCheckBox("Fixed");
    fixedCheckbox_->setToolTip("Prevents drag/resize in timeline view");
    fixedCheckbox_->setEnabled(false);
    lockedCheckbox_ = new QCheckBox("Locked");
    lockedCheckbox_->setToolTip("Prevents all editing");
    lockedCheckbox_->setEnabled(false);
    lockLayout->addWidget(fixedCheckbox_);
    lockLayout->addWidget(lockedCheckbox_);
    lockLayout->addStretch();
    commonFieldsLayout_->addRow("Lock:", lockLayout);

    // Lock warning label
    lockWarningLabel_ = new QLabel();
    lockWarningLabel_->setStyleSheet("QLabel { color: #d9534f; font-weight: bold; }");
    lockWarningLabel_->setWordWrap(true);
    lockWarningLabel_->setVisible(false);
    commonFieldsLayout_->addRow("", lockWarningLabel_);

    // Add common fields to content (will be inserted after header)
    QGroupBox* commonGroup = new QGroupBox("Event Details");
    commonGroup->setLayout(commonFieldsLayout_);

    // Insert after header in content layout
    QVBoxLayout* contentLayout = qobject_cast<QVBoxLayout*>(contentWidget_->layout());
    if (contentLayout) {
        contentLayout->insertWidget(1, commonGroup);
    }
}

void EventDetailsWidget::createDescriptionSection()
{
    descriptionGroup_ = new QGroupBox();
    QVBoxLayout* descLayout = new QVBoxLayout(descriptionGroup_);
    descLayout->setContentsMargins(8, 6, 8, 8);
    descLayout->setSpacing(4);

    // Header with toggle button
    QHBoxLayout* headerLayout = new QHBoxLayout();
    toggleDescriptionButton_ = new QPushButton("▶ Description");
    toggleDescriptionButton_->setFlat(true);
    toggleDescriptionButton_->setStyleSheet("QPushButton { text-align: left; font-weight: bold; }");
    headerLayout->addWidget(toggleDescriptionButton_);
    headerLayout->addStretch();
    descLayout->addLayout(headerLayout);

    // Description text editor
    descriptionEdit_ = new QTextEdit();
    descriptionEdit_->setPlaceholderText("Event description...");
    descriptionEdit_->setMaximumHeight(150);
    descriptionEdit_->setReadOnly(true);
    descriptionEdit_->setVisible(false);
    descLayout->addWidget(descriptionEdit_);
}

void EventDetailsWidget::createTypeSpecificSection()
{
    typeSpecificGroup_ = new QGroupBox();
    QVBoxLayout* typeLayout = new QVBoxLayout(typeSpecificGroup_);
    typeLayout->setContentsMargins(8, 6, 8, 8);
    typeLayout->setSpacing(4);

    // Header with toggle button
    QHBoxLayout* headerLayout = new QHBoxLayout();
    toggleTypeSpecificButton_ = new QPushButton("▶ Type-Specific Fields");
    toggleTypeSpecificButton_->setFlat(true);
    toggleTypeSpecificButton_->setStyleSheet("QPushButton { text-align: left; font-weight: bold; }");
    headerLayout->addWidget(toggleTypeSpecificButton_);
    headerLayout->addStretch();
    typeLayout->addLayout(headerLayout);

    // Container for type-specific fields
    typeSpecificContainer_ = new QWidget();
    typeSpecificLayout_ = new QVBoxLayout(typeSpecificContainer_);
    typeSpecificLayout_->setContentsMargins(0, 0, 0, 0);
    typeSpecificLayout_->setSpacing(4);
    typeSpecificContainer_->setVisible(false);
    typeLayout->addWidget(typeSpecificContainer_);

    typeSpecificGroup_->setVisible(false);  // Hidden until event selected
}

void EventDetailsWidget::createAttachmentsSection()
{
    attachmentsGroup_ = new QGroupBox();
    QVBoxLayout* attachLayout = new QVBoxLayout(attachmentsGroup_);
    attachLayout->setContentsMargins(8, 6, 8, 8);
    attachLayout->setSpacing(4);

    // Header with toggle button
    QHBoxLayout* headerLayout = new QHBoxLayout();
    toggleAttachmentsButton_ = new QPushButton("▼ Attachments");
    toggleAttachmentsButton_->setFlat(true);
    toggleAttachmentsButton_->setStyleSheet("QPushButton { text-align: left; font-weight: bold; }");
    headerLayout->addWidget(toggleAttachmentsButton_);
    headerLayout->addStretch();
    attachLayout->addLayout(headerLayout);

    // Attachment widget
    attachmentWidget_ = new AttachmentListWidget();
    attachmentWidget_->setVisible(true);  // Expanded by default
    attachLayout->addWidget(attachmentWidget_);
}

void EventDetailsWidget::setupConnections()
{
    // Header buttons
    connect(fullEditButton_, &QPushButton::clicked, this, &EventDetailsWidget::onOpenFullEditor);
    connect(saveButton_, &QPushButton::clicked, this, &EventDetailsWidget::onSaveChanges);
    connect(cancelButton_, &QPushButton::clicked, this, &EventDetailsWidget::onCancelChanges);

    // Toggle buttons
    connect(toggleDescriptionButton_, &QPushButton::clicked, this, &EventDetailsWidget::onToggleDescription);
    connect(toggleTypeSpecificButton_, &QPushButton::clicked, this, &EventDetailsWidget::onToggleTypeSpecific);
    connect(toggleAttachmentsButton_, &QPushButton::clicked, this, &EventDetailsWidget::onToggleAttachments);

    // Field editing signals
    connect(titleEdit_, &QLineEdit::textEdited, this, [this]() { hasUnsavedChanges_ = true; });
    connect(descriptionEdit_, &QTextEdit::textChanged, this, [this]() { hasUnsavedChanges_ = true; });
    connect(prioritySpinner_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { hasUnsavedChanges_ = true; });
    connect(startDateTimeEdit_, &QDateTimeEdit::dateTimeChanged, this, [this]() { hasUnsavedChanges_ = true; });
    connect(endDateTimeEdit_, &QDateTimeEdit::dateTimeChanged, this, [this]() { hasUnsavedChanges_ = true; });
    connect(laneControlCheckbox_, &QCheckBox::toggled, this, [this](bool checked) {
        laneSpinner_->setEnabled(checked && isEditMode_);
        hasUnsavedChanges_ = true;
    });
    connect(laneSpinner_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { hasUnsavedChanges_ = true; });
    connect(fixedCheckbox_, &QCheckBox::toggled, this, [this]() { hasUnsavedChanges_ = true; });
    connect(lockedCheckbox_, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) fixedCheckbox_->setChecked(true);
        hasUnsavedChanges_ = true;
    });

    // Attachments
    connect(attachmentWidget_, &AttachmentListWidget::attachmentsChanged, this, &EventDetailsWidget::onAttachmentsChanged);

    // Model signals
    if (model_) {
        connect(model_, &TimelineModel::eventUpdated, this, &EventDetailsWidget::onEventUpdatedInModel);
        connect(model_, &TimelineModel::eventRemoved, this, &EventDetailsWidget::onEventRemovedFromModel);
    }

    // Focus events for auto-save
    connect(titleEdit_, &QLineEdit::editingFinished, this, &EventDetailsWidget::onFieldEditingFinished);
}

void EventDetailsWidget::applyProfessionalStyling()
{
    // Header styling
    headerFrame_->setStyleSheet(
        "QFrame {"
        "    background-color: palette(window);"
        "    border: 1px solid palette(mid);"
        "    border-radius: 4px;"
        "}"
        );

    // Group box styling
    QString groupStyle =
        "QGroupBox {"
        "    border: 1px solid palette(mid);"
        "    border-radius: 4px;"
        "    margin-top: 8px;"
        "    padding-top: 8px;"
        "    font-weight: bold;"
        "}";

    descriptionGroup_->setStyleSheet(groupStyle);
    typeSpecificGroup_->setStyleSheet(groupStyle);
    attachmentsGroup_->setStyleSheet(groupStyle);

    // Edit mode highlighting (applied dynamically)
    QString editModeStyle =
        "QLineEdit:focus, QTextEdit:focus, QSpinBox:focus, QComboBox:focus, QDateTimeEdit:focus {"
        "    border: 2px solid #0078d7;"
        "    background-color: #f0f8ff;"
        "}";

    setStyleSheet(editModeStyle);
}

void EventDetailsWidget::displayEvent(const QString& eventId)
{
    if (eventId.isEmpty()) {
        clear();
        return;
    }

    const TimelineEvent* event = model_->getEvent(eventId);
    if (!event) {
        qWarning() << "EventDetailsWidget: Event not found:" << eventId;
        clear();
        return;
    }

    // Save current event ID
    currentEventId_ = eventId;
    originalEvent_ = *event;  // Backup for cancel

    // Update fields from event
    updateFieldsFromEvent(*event);

    // Update attachments
    attachmentWidget_->setEventId(eventId);
    attachmentWidget_->refresh();

    // Update visibility
    setEditMode(false);
    hasUnsavedChanges_ = false;

    qDebug() << "EventDetailsWidget: Displayed event" << eventId;
}

void EventDetailsWidget::clear()
{
    currentEventId_.clear();

    // Clear header
    typeBadgeLabel_->setText("No Event Selected");
    typeBadgeLabel_->setStyleSheet("");

    // Clear common fields
    titleEdit_->clear();
    descriptionEdit_->clear();
    startDateTimeEdit_->setDateTime(QDateTime::currentDateTime());
    endDateTimeEdit_->setDateTime(QDateTime::currentDateTime());
    prioritySpinner_->setValue(0);
    laneControlCheckbox_->setChecked(false);
    laneSpinner_->setValue(0);
    fixedCheckbox_->setChecked(false);
    lockedCheckbox_->setChecked(false);
    lockWarningLabel_->setVisible(false);

    // Clear type-specific
    clearTypeSpecificFields();
    typeSpecificGroup_->setVisible(false);

    // Clear attachments
    attachmentWidget_->clear();

    // Reset state
    setEditMode(false);
    hasUnsavedChanges_ = false;

    // Disable all editing
    fullEditButton_->setEnabled(false);
}

void EventDetailsWidget::updateFieldsFromEvent(const TimelineEvent& event)
{
    // Update header badge
    updateHeaderBadge(event.type);

    // Update common fields
    titleEdit_->setText(event.title);
    descriptionEdit_->setPlainText(event.description);
    startDateTimeEdit_->setDateTime(event.startDate);
    endDateTimeEdit_->setDateTime(event.endDate);
    prioritySpinner_->setValue(event.priority);
    laneControlCheckbox_->setChecked(event.laneControlEnabled);
    laneSpinner_->setValue(event.laneControlEnabled ? event.manualLane : event.lane);
    laneSpinner_->setEnabled(event.laneControlEnabled);
    fixedCheckbox_->setChecked(event.isFixed);
    lockedCheckbox_->setChecked(event.isLocked);

    // Update lock state UI
    updateLockStateUI();

    // Show type-specific fields
    showTypeSpecificFields(event.type);

    // Enable full edit button
    fullEditButton_->setEnabled(true);
}

void EventDetailsWidget::updateEventFromFields(TimelineEvent& event)
{
    // Update common fields
    event.title = titleEdit_->text().trimmed();
    event.description = descriptionEdit_->toPlainText().trimmed();
    event.startDate = startDateTimeEdit_->dateTime();
    event.endDate = endDateTimeEdit_->dateTime();
    event.priority = prioritySpinner_->value();
    event.laneControlEnabled = laneControlCheckbox_->isChecked();
    event.manualLane = laneSpinner_->value();
    event.isFixed = fixedCheckbox_->isChecked();
    event.isLocked = lockedCheckbox_->isChecked();

    // Update type-specific fields based on event type
    switch (event.type) {
    case TimelineEventType_Meeting:
        extractMeetingFields(event);
        break;
    case TimelineEventType_Action:
        extractActionFields(event);
        break;
    case TimelineEventType_TestEvent:
        extractTestEventFields(event);
        break;
    case TimelineEventType_Reminder:
        extractReminderFields(event);
        break;
    case TimelineEventType_JiraTicket:
        extractJiraTicketFields(event);
        break;
    }
}

void EventDetailsWidget::updateHeaderBadge(TimelineEventType type)
{
    QString typeName;
    QString colorStyle;

    switch (type) {
    case TimelineEventType_Meeting:
        typeName = "📅 Meeting";
        colorStyle = "background-color: #4CAF50; color: white; padding: 4px 8px; border-radius: 4px;";
        break;
    case TimelineEventType_Action:
        typeName = "✓ Action";
        colorStyle = "background-color: #2196F3; color: white; padding: 4px 8px; border-radius: 4px;";
        break;
    case TimelineEventType_TestEvent:
        typeName = "🔬 Test Event";
        colorStyle = "background-color: #FF9800; color: white; padding: 4px 8px; border-radius: 4px;";
        break;
    case TimelineEventType_Reminder:
        typeName = "⏰ Reminder";
        colorStyle = "background-color: #9C27B0; color: white; padding: 4px 8px; border-radius: 4px;";
        break;
    case TimelineEventType_JiraTicket:
        typeName = "🎫 Jira Ticket";
        colorStyle = "background-color: #0052CC; color: white; padding: 4px 8px; border-radius: 4px;";
        break;
    default:
        typeName = "Event";
        colorStyle = "background-color: #999; color: white; padding: 4px 8px; border-radius: 4px;";
    }

    typeBadgeLabel_->setText(typeName);
    typeBadgeLabel_->setStyleSheet(colorStyle);
}

void EventDetailsWidget::updateLockStateUI()
{
    bool isLocked = lockedCheckbox_->isChecked();
    bool isFixed = fixedCheckbox_->isChecked();

    if (isLocked) {
        lockWarningLabel_->setText("⚠ This event is LOCKED and cannot be edited.");
        lockWarningLabel_->setVisible(true);

        // Disable all editing when locked
        titleEdit_->setReadOnly(true);
        descriptionEdit_->setReadOnly(true);
        startDateTimeEdit_->setReadOnly(true);
        endDateTimeEdit_->setReadOnly(true);
        prioritySpinner_->setReadOnly(true);
        laneControlCheckbox_->setEnabled(false);
        laneSpinner_->setReadOnly(true);
        fixedCheckbox_->setEnabled(false);
        lockedCheckbox_->setEnabled(false);
    } else if (isFixed) {
        lockWarningLabel_->setText("⚠ This event is FIXED (view manipulation disabled).");
        lockWarningLabel_->setVisible(true);
    } else {
        lockWarningLabel_->setVisible(false);
    }
}

void EventDetailsWidget::setEditMode(bool editing)
{
    isEditMode_ = editing;

    // Check if event is locked
    bool isLocked = lockedCheckbox_->isChecked();

    if (isLocked && editing) {
        QMessageBox::warning(this, "Event Locked",
                             "This event is locked and cannot be edited.\n\n"
                             "Use the full edit dialog to unlock it first.");
        return;
    }

    // Update field states
    titleEdit_->setReadOnly(!editing);
    descriptionEdit_->setReadOnly(!editing);
    startDateTimeEdit_->setReadOnly(!editing);
    endDateTimeEdit_->setReadOnly(!editing);
    prioritySpinner_->setReadOnly(!editing);
    laneControlCheckbox_->setEnabled(editing);
    laneSpinner_->setEnabled(editing && laneControlCheckbox_->isChecked());
    fixedCheckbox_->setEnabled(editing);
    lockedCheckbox_->setEnabled(editing);

    // Update button visibility
    saveButton_->setVisible(editing);
    cancelButton_->setVisible(editing);
    fullEditButton_->setVisible(!editing);

    // Apply visual feedback
    QString editStyle = editing
                            ? "QLineEdit, QTextEdit, QSpinBox, QDateTimeEdit { background-color: #fffbf0; }"
                            : "";

    titleEdit_->setStyleSheet(editStyle);
    descriptionEdit_->setStyleSheet(editStyle);
}

void EventDetailsWidget::showTypeSpecificFields(TimelineEventType type)
{
    // Clear existing type-specific fields
    clearTypeSpecificFields();

    QWidget* typeWidget = nullptr;

    switch (type) {
    case TimelineEventType_Meeting:
        typeWidget = createMeetingFieldsWidget();
        populateMeetingFields(originalEvent_);
        break;
    case TimelineEventType_Action:
        typeWidget = createActionFieldsWidget();
        populateActionFields(originalEvent_);
        break;
    case TimelineEventType_TestEvent:
        typeWidget = createTestEventFieldsWidget();
        populateTestEventFields(originalEvent_);
        break;
    case TimelineEventType_Reminder:
        typeWidget = createReminderFieldsWidget();
        populateReminderFields(originalEvent_);
        break;
    case TimelineEventType_JiraTicket:
        typeWidget = createJiraTicketFieldsWidget();
        populateJiraTicketFields(originalEvent_);
        break;
    }

    if (typeWidget) {
        typeSpecificLayout_->addWidget(typeWidget);
        typeSpecificGroup_->setVisible(true);
        toggleTypeSpecificButton_->setText("▶ Type-Specific Fields");
        typeSpecificContainer_->setVisible(false);
        typeSpecificExpanded_ = false;
    } else {
        typeSpecificGroup_->setVisible(false);
    }
}

void EventDetailsWidget::clearTypeSpecificFields()
{
    // Remove all widgets from type-specific layout
    while (typeSpecificLayout_->count() > 0) {
        QLayoutItem* item = typeSpecificLayout_->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // Clear stored references
    locationEdit_ = nullptr;
    participantsEdit_ = nullptr;
    dueDateTimeEdit_ = nullptr;
    actionStatusCombo_ = nullptr;
    testCategoryCombo_ = nullptr;
    checklistItems_.clear();
    reminderDateTimeEdit_ = nullptr;
    recurringRuleCombo_ = nullptr;
    jiraKeyEdit_ = nullptr;
    jiraSummaryEdit_ = nullptr;
    jiraTypeCombo_ = nullptr;
    jiraStatusCombo_ = nullptr;
    jiraDueDateTimeEdit_ = nullptr;
}

// ============================================================================
// TYPE-SPECIFIC FIELD WIDGET CREATORS
// ============================================================================

QWidget* EventDetailsWidget::createMeetingFieldsWidget()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    locationEdit_ = new QLineEdit();
    locationEdit_->setPlaceholderText("Location or virtual link...");
    locationEdit_->setReadOnly(true);
    layout->addRow("Location:", locationEdit_);

    participantsEdit_ = new QTextEdit();
    participantsEdit_->setPlaceholderText("Participants (one per line)...");
    participantsEdit_->setMaximumHeight(80);
    participantsEdit_->setReadOnly(true);
    layout->addRow("Participants:", participantsEdit_);

    return widget;
}

QWidget* EventDetailsWidget::createActionFieldsWidget()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    dueDateTimeEdit_ = new QDateTimeEdit();
    dueDateTimeEdit_->setCalendarPopup(true);
    dueDateTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    dueDateTimeEdit_->setReadOnly(true);
    layout->addRow("Due Date:", dueDateTimeEdit_);

    actionStatusCombo_ = new QComboBox();
    actionStatusCombo_->addItems({"Not Started", "In Progress", "Blocked", "Completed"});
    actionStatusCombo_->setEnabled(false);
    layout->addRow("Status:", actionStatusCombo_);

    return widget;
}

QWidget* EventDetailsWidget::createTestEventFieldsWidget()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setSpacing(6);

    QFormLayout* formLayout = new QFormLayout();

    testCategoryCombo_ = new QComboBox();
    testCategoryCombo_->addItems({"Dry Run", "Preliminary", "Formal"});
    testCategoryCombo_->setEnabled(false);
    formLayout->addRow("Category:", testCategoryCombo_);

    layout->addLayout(formLayout);

    // Checklist section
    QLabel* checklistLabel = new QLabel("Preparation Checklist:");
    checklistLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(checklistLabel);

    checklistLayout_ = new QVBoxLayout();
    checklistLayout_->setSpacing(4);
    layout->addLayout(checklistLayout_);

    return widget;
}

QWidget* EventDetailsWidget::createReminderFieldsWidget()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    reminderDateTimeEdit_ = new QDateTimeEdit();
    reminderDateTimeEdit_->setCalendarPopup(true);
    reminderDateTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    reminderDateTimeEdit_->setReadOnly(true);
    layout->addRow("Reminder:", reminderDateTimeEdit_);

    recurringRuleCombo_ = new QComboBox();
    recurringRuleCombo_->addItems({"None", "Daily", "Weekly", "Monthly"});
    recurringRuleCombo_->setEnabled(false);
    layout->addRow("Recurring:", recurringRuleCombo_);

    return widget;
}

QWidget* EventDetailsWidget::createJiraTicketFieldsWidget()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    jiraKeyEdit_ = new QLineEdit();
    jiraKeyEdit_->setPlaceholderText("e.g., PROJ-123");
    jiraKeyEdit_->setReadOnly(true);
    layout->addRow("Jira Key:", jiraKeyEdit_);

    jiraSummaryEdit_ = new QLineEdit();
    jiraSummaryEdit_->setPlaceholderText("Brief summary from Jira...");
    jiraSummaryEdit_->setReadOnly(true);
    layout->addRow("Summary:", jiraSummaryEdit_);

    jiraTypeCombo_ = new QComboBox();
    jiraTypeCombo_->addItems({"Story", "Bug", "Task", "Sub-task", "Epic"});
    jiraTypeCombo_->setEnabled(false);
    layout->addRow("Type:", jiraTypeCombo_);

    jiraStatusCombo_ = new QComboBox();
    jiraStatusCombo_->addItems({"To Do", "In Progress", "Done"});
    jiraStatusCombo_->setEnabled(false);
    layout->addRow("Status:", jiraStatusCombo_);

    jiraDueDateTimeEdit_ = new QDateTimeEdit();
    jiraDueDateTimeEdit_->setCalendarPopup(true);
    jiraDueDateTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    jiraDueDateTimeEdit_->setReadOnly(true);
    layout->addRow("Due Date:", jiraDueDateTimeEdit_);

    return widget;
}

// ============================================================================
// TYPE-SPECIFIC FIELD POPULATION
// ============================================================================

void EventDetailsWidget::populateMeetingFields(const TimelineEvent& event)
{
    if (locationEdit_) locationEdit_->setText(event.location);
    if (participantsEdit_) participantsEdit_->setPlainText(event.participants);
}

void EventDetailsWidget::populateActionFields(const TimelineEvent& event)
{
    if (dueDateTimeEdit_ && event.dueDateTime.isValid()) {
        dueDateTimeEdit_->setDateTime(event.dueDateTime);
    }
    if (actionStatusCombo_) {
        int index = actionStatusCombo_->findText(event.status);
        if (index >= 0) actionStatusCombo_->setCurrentIndex(index);
    }
}

void EventDetailsWidget::populateTestEventFields(const TimelineEvent& event)
{
    if (testCategoryCombo_) {
        int index = testCategoryCombo_->findText(event.testCategory);
        if (index >= 0) testCategoryCombo_->setCurrentIndex(index);
    }

    // Populate checklist
    if (checklistLayout_) {
        for (auto it = event.preparationChecklist.constBegin();
             it != event.preparationChecklist.constEnd(); ++it) {
            QCheckBox* cb = new QCheckBox(it.key());
            cb->setChecked(it.value());
            cb->setEnabled(false);
            checklistItems_[it.key()] = cb;
            checklistLayout_->addWidget(cb);
        }
    }
}

void EventDetailsWidget::populateReminderFields(const TimelineEvent& event)
{
    if (reminderDateTimeEdit_ && event.reminderDateTime.isValid()) {
        reminderDateTimeEdit_->setDateTime(event.reminderDateTime);
    }
    if (recurringRuleCombo_) {
        int index = recurringRuleCombo_->findText(event.recurringRule);
        if (index >= 0) recurringRuleCombo_->setCurrentIndex(index);
    }
}

void EventDetailsWidget::populateJiraTicketFields(const TimelineEvent& event)
{
    if (jiraKeyEdit_) jiraKeyEdit_->setText(event.jiraKey);
    if (jiraSummaryEdit_) jiraSummaryEdit_->setText(event.jiraSummary);

    if (jiraTypeCombo_) {
        int index = jiraTypeCombo_->findText(event.jiraType);
        if (index >= 0) jiraTypeCombo_->setCurrentIndex(index);
    }
    if (jiraStatusCombo_) {
        int index = jiraStatusCombo_->findText(event.jiraStatus);
        if (index >= 0) jiraStatusCombo_->setCurrentIndex(index);
    }
    if (jiraDueDateTimeEdit_ && event.dueDateTime.isValid()) {
        jiraDueDateTimeEdit_->setDateTime(event.dueDateTime);
    }
}

// ============================================================================
// TYPE-SPECIFIC FIELD EXTRACTION
// ============================================================================

void EventDetailsWidget::extractMeetingFields(TimelineEvent& event)
{
    if (locationEdit_) event.location = locationEdit_->text().trimmed();
    if (participantsEdit_) event.participants = participantsEdit_->toPlainText().trimmed();
}

void EventDetailsWidget::extractActionFields(TimelineEvent& event)
{
    if (dueDateTimeEdit_) event.dueDateTime = dueDateTimeEdit_->dateTime();
    if (actionStatusCombo_) event.status = actionStatusCombo_->currentText();
}

void EventDetailsWidget::extractTestEventFields(TimelineEvent& event)
{
    if (testCategoryCombo_) event.testCategory = testCategoryCombo_->currentText();

    // Extract checklist
    event.preparationChecklist.clear();
    for (auto it = checklistItems_.constBegin(); it != checklistItems_.constEnd(); ++it) {
        event.preparationChecklist[it.key()] = it.value()->isChecked();
    }
}

void EventDetailsWidget::extractReminderFields(TimelineEvent& event)
{
    if (reminderDateTimeEdit_) event.reminderDateTime = reminderDateTimeEdit_->dateTime();
    if (recurringRuleCombo_) event.recurringRule = recurringRuleCombo_->currentText();
}

void EventDetailsWidget::extractJiraTicketFields(TimelineEvent& event)
{
    if (jiraKeyEdit_) event.jiraKey = jiraKeyEdit_->text().trimmed();
    if (jiraSummaryEdit_) event.jiraSummary = jiraSummaryEdit_->text().trimmed();
    if (jiraTypeCombo_) event.jiraType = jiraTypeCombo_->currentText();
    if (jiraStatusCombo_) event.jiraStatus = jiraStatusCombo_->currentText();
    if (jiraDueDateTimeEdit_) event.dueDateTime = jiraDueDateTimeEdit_->dateTime();
}

// ============================================================================
// SLOT IMPLEMENTATIONS
// ============================================================================

void EventDetailsWidget::onFieldEditingStarted()
{
    if (!isEditMode_) {
        setEditMode(true);
    }
}

void EventDetailsWidget::onFieldEditingFinished()
{
    // Auto-save on focus loss if changes were made
    if (hasUnsavedChanges_ && isEditMode_) {
        onSaveChanges();
    }
}

void EventDetailsWidget::onSaveChanges()
{
    if (currentEventId_.isEmpty() || !model_) {
        return;
    }

    // Validate fields
    if (!validateFields()) {
        return;
    }

    // Get current event from model
    const TimelineEvent* currentEvent = model_->getEvent(currentEventId_);
    if (!currentEvent) {
        qWarning() << "EventDetailsWidget: Event not found during save:" << currentEventId_;
        return;
    }

    // Create updated event
    TimelineEvent updatedEvent = *currentEvent;
    updateEventFromFields(updatedEvent);

    // Update model
    bool success = model_->updateEvent(currentEventId_, updatedEvent);

    if (success) {
        qDebug() << "EventDetailsWidget: Successfully saved changes to event" << currentEventId_;
        originalEvent_ = updatedEvent;  // Update backup
        hasUnsavedChanges_ = false;
        setEditMode(false);
        emit eventModified(currentEventId_);
    } else {
        QMessageBox::warning(this, "Save Failed",
                             "Failed to save event changes. Please try again.");
    }
}

void EventDetailsWidget::onCancelChanges()
{
    if (currentEventId_.isEmpty()) {
        return;
    }

    // Confirm if there are unsaved changes
    if (hasUnsavedChanges_) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Discard Changes?",
            "You have unsaved changes. Discard them?",
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    // Restore original event
    updateFieldsFromEvent(originalEvent_);

    hasUnsavedChanges_ = false;
    setEditMode(false);

    qDebug() << "EventDetailsWidget: Cancelled changes for event" << currentEventId_;
}

void EventDetailsWidget::onOpenFullEditor()
{
    if (!currentEventId_.isEmpty()) {
        emit editFullDialogRequested(currentEventId_);
    }
}

void EventDetailsWidget::onAttachmentsChanged()
{
    if (!currentEventId_.isEmpty()) {
        emit eventModified(currentEventId_);
    }
}

void EventDetailsWidget::onToggleDescription()
{
    descriptionExpanded_ = !descriptionExpanded_;
    descriptionEdit_->setVisible(descriptionExpanded_);
    toggleDescriptionButton_->setText(descriptionExpanded_ ? "▼ Description" : "▶ Description");
}

void EventDetailsWidget::onToggleTypeSpecific()
{
    typeSpecificExpanded_ = !typeSpecificExpanded_;
    typeSpecificContainer_->setVisible(typeSpecificExpanded_);
    toggleTypeSpecificButton_->setText(typeSpecificExpanded_ ? "▼ Type-Specific Fields" : "▶ Type-Specific Fields");
}

void EventDetailsWidget::onToggleAttachments()
{
    attachmentsExpanded_ = !attachmentsExpanded_;
    attachmentWidget_->setVisible(attachmentsExpanded_);
    toggleAttachmentsButton_->setText(attachmentsExpanded_ ? "▼ Attachments" : "▶ Attachments");
}

void EventDetailsWidget::onEventUpdatedInModel(const QString& eventId)
{
    if (eventId == currentEventId_) {
        // Refresh display if the current event was updated externally
        if (!isEditMode_) {
            displayEvent(eventId);
        }
    }
}

void EventDetailsWidget::onEventRemovedFromModel(const QString& eventId)
{
    if (eventId == currentEventId_) {
        clear();
    }
}

bool EventDetailsWidget::validateFields()
{
    clearFieldHighlights();

    // Title validation
    if (titleEdit_->text().trimmed().isEmpty()) {
        highlightInvalidField(titleEdit_);
        QMessageBox::warning(this, "Validation Error", "Title cannot be empty.");
        return false;
    }

    // Date validation
    if (endDateTimeEdit_->dateTime() < startDateTimeEdit_->dateTime()) {
        highlightInvalidField(endDateTimeEdit_);
        QMessageBox::warning(this, "Validation Error", "End date cannot be before start date.");
        return false;
    }

    return true;
}

void EventDetailsWidget::highlightInvalidField(QWidget* field)
{
    if (field) {
        field->setStyleSheet("border: 2px solid #d9534f; background-color: #fff0f0;");
        field->setFocus();
    }
}

void EventDetailsWidget::clearFieldHighlights()
{
    titleEdit_->setStyleSheet("");
    startDateTimeEdit_->setStyleSheet("");
    endDateTimeEdit_->setStyleSheet("");
}
