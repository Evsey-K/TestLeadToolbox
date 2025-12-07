// EditEventDialog.cpp - Complete Implementation with Dynamic Fields


#include "EditEventDialog.h"


EditEventDialog::EditEventDialog(const QString& eventId,
                                 TimelineModel* model,
                                 const QDate& versionStart,
                                 const QDate& versionEnd,
                                 QWidget* parent)
    : QDialog(parent)
    , eventId_(eventId)
    , model_(model)
    , versionStart_(versionStart)
    , versionEnd_(versionEnd)
{
    setupUi();
    createFieldGroups();

    // Load existing event data
    const TimelineEvent* event = model_->getEvent(eventId_);
    if (event)
    {
        originalType_ = event->type;
        populateFromEvent(*event);
    }
    else
    {
        QMessageBox::critical(this, "Error", "Event not found!");
        reject();
    }
}


EditEventDialog::~EditEventDialog()
{
    // Qt parent-child hierarchy handles cleanup
}


void EditEventDialog::setupUi()
{
    setWindowTitle("Edit Timeline Event");
    setMinimumWidth(500);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === COMMON FIELDS SECTION ===
    QGroupBox* commonGroup = new QGroupBox("Event Information");
    QFormLayout* commonLayout = new QFormLayout(commonGroup);

    // Type combo
    typeCombo_ = new QComboBox();
    typeCombo_->addItem("Meeting", TimelineEventType_Meeting);
    typeCombo_->addItem("Action", TimelineEventType_Action);
    typeCombo_->addItem("Test Event", TimelineEventType_TestEvent);
    typeCombo_->addItem("Reminder", TimelineEventType_Reminder);
    typeCombo_->addItem("Jira Ticket", TimelineEventType_JiraTicket);
    commonLayout->addRow("Type:", typeCombo_);

    // Title
    titleEdit_ = new QLineEdit();
    titleEdit_->setPlaceholderText("Enter event title...");
    commonLayout->addRow("Title:", titleEdit_);

    // Priority
    prioritySpinner_ = new QSpinBox();
    prioritySpinner_->setRange(0, 5);
    prioritySpinner_->setToolTip("0 = Highest priority, 5 = Lowest priority");
    commonLayout->addRow("Priority:", prioritySpinner_);

    // Description
    descriptionEdit_ = new QTextEdit();
    descriptionEdit_->setPlaceholderText("Enter description...");
    descriptionEdit_->setMaximumHeight(100);
    commonLayout->addRow("Description:", descriptionEdit_);

    // Lane Control Section
    QGroupBox* laneControlGroup = new QGroupBox("Lane Control (Advanced)");
    QVBoxLayout* laneControlLayout = new QVBoxLayout(laneControlGroup);

    laneControlCheckbox_ = new QCheckBox("Enable manual lane control");
    laneControlCheckbox_->setToolTip("When enabled, you can specify which lane this event appears in");
    laneControlLayout->addWidget(laneControlCheckbox_);

    QHBoxLayout* laneNumberLayout = new QHBoxLayout();
    QLabel* laneLabel = new QLabel("Lane number:");
    manualLaneSpinner_ = new QSpinBox();
    manualLaneSpinner_->setRange(0, 20);
    manualLaneSpinner_->setValue(0);
    manualLaneSpinner_->setEnabled(false);
    manualLaneSpinner_->setToolTip("Lane 0 is at the top, higher numbers are below");
    laneNumberLayout->addWidget(laneLabel);
    laneNumberLayout->addWidget(manualLaneSpinner_);
    laneNumberLayout->addStretch();
    laneControlLayout->addLayout(laneNumberLayout);

    laneControlWarningLabel_ = new QLabel(
        "<i>Note: If another manually-controlled event already occupies this lane "
        "at the same time, you'll receive a conflict warning.</i>");
    laneControlWarningLabel_->setWordWrap(true);
    laneControlWarningLabel_->setStyleSheet("QLabel { color: #666; }");
    laneControlWarningLabel_->setVisible(false);
    laneControlLayout->addWidget(laneControlWarningLabel_);

    mainLayout->addWidget(laneControlGroup);

    // Connect lane control checkbox
    connect(laneControlCheckbox_, &QCheckBox::toggled, [this](bool checked) {
        manualLaneSpinner_->setEnabled(checked);
        laneControlWarningLabel_->setVisible(checked);
    });

    mainLayout->addWidget(commonGroup);

    // Type-Specific Fields Section
    fieldStack_ = new QStackedWidget();
    mainLayout->addWidget(fieldStack_);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox();

    QPushButton* saveButton = buttonBox->addButton("Save", QDialogButtonBox::AcceptRole);
    deleteButton_ = buttonBox->addButton("Delete", QDialogButtonBox::DestructiveRole);
    QPushButton* cancelButton = buttonBox->addButton("Cancel", QDialogButtonBox::RejectRole);

    mainLayout->addWidget(buttonBox);

    // Connections
    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EditEventDialog::onTypeChanged);
    connect(saveButton, &QPushButton::clicked, this, &EditEventDialog::validateAndAccept);
    connect(deleteButton_, &QPushButton::clicked, this, &EditEventDialog::onDeleteClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}


void EditEventDialog::createFieldGroups()
{
    fieldStack_->addWidget(createMeetingFields());        // Index 0
    fieldStack_->addWidget(createActionFields());          // Index 1
    fieldStack_->addWidget(createTestEventFields());       // Index 2
    fieldStack_->addWidget(createReminderFields());        // Index 3
    fieldStack_->addWidget(createJiraTicketFields());      // Index 4
}


QWidget* EditEventDialog::createMeetingFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    // Match spacing to other event types
    layout->setVerticalSpacing(6);
    layout->setHorizontalSpacing(10);

    // Start Date/Time
    QHBoxLayout* startLayout = new QHBoxLayout();
    startLayout->setSpacing(5);
    startLayout->setContentsMargins(0, 0, 0, 0);
    meetingStartDate_ = new QDateEdit(QDate::currentDate());
    meetingStartDate_->setCalendarPopup(true);
    meetingStartDate_->setMinimumDate(versionStart_);
    meetingStartDate_->setMaximumDate(versionEnd_);
    meetingStartTime_ = new QTimeEdit(QTime(9, 0));
    startLayout->addWidget(meetingStartDate_);
    startLayout->addWidget(meetingStartTime_);
    layout->addRow("Start:", startLayout);

    // End Date/Time
    QHBoxLayout* endLayout = new QHBoxLayout();
    endLayout->setSpacing(5);
    endLayout->setContentsMargins(0, 0, 0, 0);
    meetingEndDate_ = new QDateEdit(QDate::currentDate());
    meetingEndDate_->setCalendarPopup(true);
    meetingEndDate_->setMinimumDate(versionStart_);
    meetingEndDate_->setMaximumDate(versionEnd_);
    meetingEndTime_ = new QTimeEdit(QTime(10, 0));
    endLayout->addWidget(meetingEndDate_);
    endLayout->addWidget(meetingEndTime_);
    layout->addRow("End:", endLayout);

    // Location
    locationEdit_ = new QLineEdit();
    locationEdit_->setPlaceholderText("Physical location or virtual link");
    layout->addRow("Location:", locationEdit_);

    // Participants
    participantsEdit_ = new QTextEdit();
    participantsEdit_->setPlaceholderText("Enter attendee names/emails...");
    participantsEdit_->setMaximumHeight(60);
    participantsEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addRow("Participants:", participantsEdit_);

    // Connect date change handler
    connect(meetingStartDate_, &QDateEdit::dateChanged, [this](const QDate& date) {
        if (meetingEndDate_->date() < date)
            meetingEndDate_->setDate(date);
    });

    return widget;
}


QWidget* EditEventDialog::createActionFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    // Start Date/Time
    QHBoxLayout* startLayout = new QHBoxLayout();
    actionStartDate_ = new QDateEdit(QDate::currentDate());
    actionStartDate_->setCalendarPopup(true);
    actionStartDate_->setMinimumDate(versionStart_);
    actionStartDate_->setMaximumDate(versionEnd_);
    actionStartTime_ = new QTimeEdit(QTime::currentTime());
    startLayout->addWidget(actionStartDate_);
    startLayout->addWidget(actionStartTime_);
    layout->addRow("Start:", startLayout);

    // Due Date/Time
    actionDueDateTime_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(7));
    actionDueDateTime_->setCalendarPopup(true);
    actionDueDateTime_->setDisplayFormat("yyyy-MM-dd HH:mm");
    layout->addRow("Due:", actionDueDateTime_);

    // Status
    statusCombo_ = new QComboBox();
    statusCombo_->addItems({"Not Started", "In Progress", "Blocked", "Completed"});
    layout->addRow("Status:", statusCombo_);

    return widget;
}


QWidget* EditEventDialog::createTestEventFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    // Start Date
    testStartDate_ = new QDateEdit(QDate::currentDate());
    testStartDate_->setCalendarPopup(true);
    testStartDate_->setMinimumDate(versionStart_);
    testStartDate_->setMaximumDate(versionEnd_);
    layout->addRow("Start Date:", testStartDate_);

    // End Date
    testEndDate_ = new QDateEdit(QDate::currentDate().addDays(1));
    testEndDate_->setCalendarPopup(true);
    testEndDate_->setMinimumDate(versionStart_);
    testEndDate_->setMaximumDate(versionEnd_);
    layout->addRow("End Date:", testEndDate_);

    // Test Category
    testCategoryCombo_ = new QComboBox();
    testCategoryCombo_->addItems({"Dry Run", "Preliminary", "Formal"});
    layout->addRow("Category:", testCategoryCombo_);

    // Preparation Checklist
    QGroupBox* checklistGroup = new QGroupBox("Preparation Checklist");
    QVBoxLayout* checklistLayout = new QVBoxLayout(checklistGroup);

    QStringList checklistItemNames = {
        "IAVM Installation (C/U)",
        "CM Software Audit (C/U)",
        "Latest Build Installation (C/U)",
        "Lab Configuration Verified (C/U)",
        "Test Scenarios Prepared",
        "Test Event Schedule Created",
        "Test Binders Prepared",
        "Issue Report Forms Prepared",
        "TRR Slides Prepared",
        "In-Brief Slides Prepared",
        "Test Event Meetings Established"
    };

    for (const QString& itemName : checklistItemNames)
    {
        QCheckBox* checkbox = new QCheckBox(itemName);
        checklistLayout->addWidget(checkbox);
        checklistItems_[itemName] = checkbox;
    }

    layout->addRow(checklistGroup);

    // Connect date change handler
    connect(testStartDate_, &QDateEdit::dateChanged, [this](const QDate& date) {
        if (testEndDate_->date() < date)
            testEndDate_->setDate(date);
    });

    return widget;
}


QWidget* EditEventDialog::createReminderFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    // Reminder Date/Time
    reminderDateTime_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1));
    reminderDateTime_->setCalendarPopup(true);
    reminderDateTime_->setDisplayFormat("yyyy-MM-dd HH:mm");
    layout->addRow("Reminder:", reminderDateTime_);

    // Recurring Rule
    recurringRuleCombo_ = new QComboBox();
    recurringRuleCombo_->addItems({"None", "Daily", "Weekly", "Monthly"});
    layout->addRow("Recurrence:", recurringRuleCombo_);

    return widget;
}


QWidget* EditEventDialog::createJiraTicketFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    // Jira Key
    jiraKeyEdit_ = new QLineEdit();
    jiraKeyEdit_->setPlaceholderText("e.g., ABC-123");
    layout->addRow("Ticket Key:", jiraKeyEdit_);

    // Jira Summary
    jiraSummaryEdit_ = new QLineEdit();
    jiraSummaryEdit_->setPlaceholderText("Brief summary from Jira");
    layout->addRow("Summary:", jiraSummaryEdit_);

    // Jira Type
    jiraTypeCombo_ = new QComboBox();
    jiraTypeCombo_->addItems({"Story", "Bug", "Task", "Sub-task", "Epic"});
    layout->addRow("Type:", jiraTypeCombo_);

    // Jira Status
    jiraStatusCombo_ = new QComboBox();
    jiraStatusCombo_->addItems({"To Do", "In Progress", "Done"});
    layout->addRow("Status:", jiraStatusCombo_);

    // Start Date
    jiraStartDate_ = new QDateEdit(QDate::currentDate());
    jiraStartDate_->setCalendarPopup(true);
    jiraStartDate_->setMinimumDate(versionStart_);
    jiraStartDate_->setMaximumDate(versionEnd_);
    layout->addRow("Start Date:", jiraStartDate_);

    // Due Date
    jiraDueDate_ = new QDateEdit(QDate::currentDate().addDays(14));
    jiraDueDate_->setCalendarPopup(true);
    jiraDueDate_->setMinimumDate(versionStart_);
    jiraDueDate_->setMaximumDate(versionEnd_);
    layout->addRow("Due Date:", jiraDueDate_);

    return widget;
}


void EditEventDialog::populateFromEvent(const TimelineEvent& event)
{
    // Set type combo
    for (int i = 0; i < typeCombo_->count(); ++i)
    {
        if (typeCombo_->itemData(i).toInt() == static_cast<int>(event.type))
        {
            typeCombo_->setCurrentIndex(i);
            break;
        }
    }

    // Set common fields
    titleEdit_->setText(event.title);
    prioritySpinner_->setValue(event.priority);
    laneControlCheckbox_->setChecked(event.laneControlEnabled);
    manualLaneSpinner_->setValue(event.manualLane);
    manualLaneSpinner_->setEnabled(event.laneControlEnabled);
    laneControlWarningLabel_->setVisible(event.laneControlEnabled);
    descriptionEdit_->setPlainText(event.description);

    // Set type-specific fields
    switch (event.type)
    {
    case TimelineEventType_Meeting:
        meetingStartDate_->setDate(event.startDate);
        meetingStartTime_->setTime(event.startTime);
        meetingEndDate_->setDate(event.endDate);
        meetingEndTime_->setTime(event.endTime);
        locationEdit_->setText(event.location);
        participantsEdit_->setPlainText(event.participants);
        break;

    case TimelineEventType_Action:
        actionStartDate_->setDate(event.startDate);
        actionStartTime_->setTime(event.startTime);
        if (event.dueDateTime.isValid())
            actionDueDateTime_->setDateTime(event.dueDateTime);
        // Set status combo
        for (int i = 0; i < statusCombo_->count(); ++i)
        {
            if (statusCombo_->itemText(i) == event.status)
            {
                statusCombo_->setCurrentIndex(i);
                break;
            }
        }
        break;

    case TimelineEventType_TestEvent:
        testStartDate_->setDate(event.startDate);
        testEndDate_->setDate(event.endDate);
        // Set category
        for (int i = 0; i < testCategoryCombo_->count(); ++i)
        {
            if (testCategoryCombo_->itemText(i) == event.testCategory)
            {
                testCategoryCombo_->setCurrentIndex(i);
                break;
            }
        }
        // Set checklist items
        for (auto it = event.preparationChecklist.begin();
             it != event.preparationChecklist.end(); ++it)
        {
            if (checklistItems_.contains(it.key()))
            {
                checklistItems_[it.key()]->setChecked(it.value());
            }
        }
        break;

    case TimelineEventType_Reminder:
        if (event.reminderDateTime.isValid())
            reminderDateTime_->setDateTime(event.reminderDateTime);
        // Set recurring rule
        for (int i = 0; i < recurringRuleCombo_->count(); ++i)
        {
            if (recurringRuleCombo_->itemText(i) == event.recurringRule)
            {
                recurringRuleCombo_->setCurrentIndex(i);
                break;
            }
        }
        break;

    case TimelineEventType_JiraTicket:
        jiraKeyEdit_->setText(event.jiraKey);
        jiraSummaryEdit_->setText(event.jiraSummary);
        // Set Jira type
        for (int i = 0; i < jiraTypeCombo_->count(); ++i)
        {
            if (jiraTypeCombo_->itemText(i) == event.jiraType)
            {
                jiraTypeCombo_->setCurrentIndex(i);
                break;
            }
        }
        // Set Jira status
        for (int i = 0; i < jiraStatusCombo_->count(); ++i)
        {
            if (jiraStatusCombo_->itemText(i) == event.jiraStatus)
            {
                jiraStatusCombo_->setCurrentIndex(i);
                break;
            }
        }
        jiraStartDate_->setDate(event.startDate);
        jiraDueDate_->setDate(event.endDate);
        break;
    }

    // Show appropriate fields for the type
    showFieldsForType(event.type);
}


void EditEventDialog::onTypeChanged(int index)
{
    TimelineEventType newType = static_cast<TimelineEventType>(
        typeCombo_->itemData(index).toInt());

    // Warn user if changing type
    if (newType != originalType_)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Change Event Type?",
            "Changing the event type will clear type-specific fields. Continue?",
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply == QMessageBox::No)
        {
            // Revert to original type
            for (int i = 0; i < typeCombo_->count(); ++i)
            {
                if (typeCombo_->itemData(i).toInt() == static_cast<int>(originalType_))
                {
                    typeCombo_->blockSignals(true);
                    typeCombo_->setCurrentIndex(i);
                    typeCombo_->blockSignals(false);
                    return;
                }
            }
        }
    }

    showFieldsForType(newType);
}


void EditEventDialog::showFieldsForType(TimelineEventType type)
{
    switch (type)
    {
    case TimelineEventType_Meeting:
        fieldStack_->setCurrentIndex(0);
        break;
    case TimelineEventType_Action:
        fieldStack_->setCurrentIndex(1);
        break;
    case TimelineEventType_TestEvent:
        fieldStack_->setCurrentIndex(2);
        break;
    case TimelineEventType_Reminder:
        fieldStack_->setCurrentIndex(3);
        break;
    case TimelineEventType_JiraTicket:
        fieldStack_->setCurrentIndex(4);
        break;
    }

    // Adjust dialog size to fit content
    adjustSize();
}


bool EditEventDialog::validateCommonFields()
{
    if (titleEdit_->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Validation Error",
                             "Please enter a title for the event.");
        titleEdit_->setFocus();
        return false;
    }
    return true;
}


bool EditEventDialog::validateTypeSpecificFields()
{
    TimelineEventType type = static_cast<TimelineEventType>(
        typeCombo_->currentData().toInt());

    switch (type)
    {
    case TimelineEventType_Meeting:
        if (meetingEndDate_->date() < meetingStartDate_->date())
        {
            QMessageBox::warning(this, "Validation Error",
                                 "End date cannot be before start date.");
            return false;
        }
        if (meetingEndDate_->date() == meetingStartDate_->date() &&
            meetingEndTime_->time() < meetingStartTime_->time())
        {
            QMessageBox::warning(this, "Validation Error",
                                 "End time cannot be before start time on the same day.");
            return false;
        }
        break;

    case TimelineEventType_Action:
        if (actionDueDateTime_->dateTime() < QDateTime(actionStartDate_->date(), actionStartTime_->time()))
        {
            QMessageBox::warning(this, "Validation Error",
                                 "Due date/time must be after start date/time.");
            return false;
        }
        break;

    case TimelineEventType_TestEvent:
        if (testEndDate_->date() < testStartDate_->date())
        {
            QMessageBox::warning(this, "Validation Error",
                                 "End date cannot be before start date.");
            return false;
        }
        break;

    case TimelineEventType_JiraTicket:
        if (jiraKeyEdit_->text().trimmed().isEmpty())
        {
            QMessageBox::warning(this, "Validation Error",
                                 "Please enter a Jira ticket key.");
            jiraKeyEdit_->setFocus();
            return false;
        }
        if (jiraDueDate_->date() < jiraStartDate_->date())
        {
            QMessageBox::warning(this, "Validation Error",
                                 "Due date cannot be before start date.");
            return false;
        }
        break;

    default:
        break;
    }

    return true;
}


void EditEventDialog::validateAndAccept()
{
    if (!validateCommonFields() || !validateTypeSpecificFields())
    {
        return;
    }
    accept();
}


void EditEventDialog::onDeleteClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Event?",
        "Are you sure you want to delete this event?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes)
    {
        emit deleteRequested();
        accept();
    }
}


TimelineEvent EditEventDialog::getEvent() const
{
    TimelineEvent event;

    // Preserve the ID
    event.id = eventId_;

    // Populate common fields
    populateCommonFields(event);

    // Populate lane control fields
    event.laneControlEnabled = laneControlCheckbox_->isChecked();
    event.manualLane = manualLaneSpinner_->value();

    // Populate type-specific fields
    populateTypeSpecificFields(event);

    // Assign color based on type
    event.color = TimelineModel::colorForType(event.type);

    return event;
}


void EditEventDialog::populateCommonFields(TimelineEvent& event) const
{
    event.type = static_cast<TimelineEventType>(typeCombo_->currentData().toInt());
    event.title = titleEdit_->text().trimmed();
    event.priority = prioritySpinner_->value();
    event.description = descriptionEdit_->toPlainText().trimmed();
}


void EditEventDialog::populateTypeSpecificFields(TimelineEvent& event) const
{
    switch (event.type)
    {
    case TimelineEventType_Meeting:
        event.startDate = meetingStartDate_->date();
        event.endDate = meetingEndDate_->date();
        event.startTime = meetingStartTime_->time();
        event.endTime = meetingEndTime_->time();
        event.location = locationEdit_->text().trimmed();
        event.participants = participantsEdit_->toPlainText().trimmed();
        break;

    case TimelineEventType_Action:
        event.startDate = actionStartDate_->date();
        event.startTime = actionStartTime_->time();
        event.dueDateTime = actionDueDateTime_->dateTime();
        event.status = statusCombo_->currentText();
        event.endDate = event.dueDateTime.date();
        break;

    case TimelineEventType_TestEvent:
        event.startDate = testStartDate_->date();
        event.endDate = testEndDate_->date();
        event.testCategory = testCategoryCombo_->currentText();
        for (auto it = checklistItems_.begin(); it != checklistItems_.end(); ++it)
        {
            event.preparationChecklist[it.key()] = it.value()->isChecked();
        }
        break;

    case TimelineEventType_Reminder:
        event.reminderDateTime = reminderDateTime_->dateTime();
        event.recurringRule = recurringRuleCombo_->currentText();
        event.startDate = event.reminderDateTime.date();
        event.endDate = event.reminderDateTime.date();
        break;

    case TimelineEventType_JiraTicket:
        event.jiraKey = jiraKeyEdit_->text().trimmed();
        event.jiraSummary = jiraSummaryEdit_->text().trimmed();
        event.jiraType = jiraTypeCombo_->currentText();
        event.jiraStatus = jiraStatusCombo_->currentText();
        event.startDate = jiraStartDate_->date();
        event.endDate = jiraDueDate_->date();
        break;
    }
}
