// AddEventDialog.cpp - Complete Implementation with Dynamic Fields

#include "AddEventDialog.h"

AddEventDialog::AddEventDialog(const QDate& versionStart,
                               const QDate& versionEnd,
                               QWidget* parent)
    : QDialog(parent)
    , versionStart_(versionStart)
    , versionEnd_(versionEnd)
{
    setupUi();
    createFieldGroups();

    // Set default to Meeting type
    typeCombo_->setCurrentIndex(0);
    onTypeChanged(0);
}

AddEventDialog::~AddEventDialog()
{
    // Qt parent-child hierarchy handles cleanup
}

void AddEventDialog::setupUi()
{
    setWindowTitle("Add Timeline Event");
    setMinimumWidth(500);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Common Fields Section
    QGroupBox* commonGroup = new QGroupBox("Event Information");
    QFormLayout* commonLayout = new QFormLayout(commonGroup);

    // Type Combo
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
    prioritySpinner_->setValue(2);
    prioritySpinner_->setToolTip("0 = Highest priority, 5 = Lowest priority");
    commonLayout->addRow("Priority:", prioritySpinner_);

    // Description
    descriptionEdit_ = new QTextEdit();
    descriptionEdit_->setPlaceholderText("Enter description...");
    descriptionEdit_->setMaximumHeight(100);
    commonLayout->addRow("Description:", descriptionEdit_);

    // Lane Control
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
    manualLaneSpinner_->setEnabled(false);  // Initially disabled
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
    connect(laneControlCheckbox_, &QCheckBox::toggled, [this](bool checked)
    {
        manualLaneSpinner_->setEnabled(checked);
        laneControlWarningLabel_->setVisible(checked);
    });

    mainLayout->addWidget(commonGroup);

    // Type-Specific Fields Section
    fieldStack_ = new QStackedWidget();
    mainLayout->addWidget(fieldStack_);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);

    // Connections
    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AddEventDialog::onTypeChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddEventDialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AddEventDialog::createFieldGroups()
{
    fieldStack_->addWidget(createMeetingFields());          // Index 0
    fieldStack_->addWidget(createActionFields());           // Index 1
    fieldStack_->addWidget(createTestEventFields());        // Index 2
    fieldStack_->addWidget(createReminderFields());         // Index 3
    fieldStack_->addWidget(createJiraTicketFields());       // Index 4
}

QWidget* AddEventDialog::createMeetingFields()
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


QWidget* AddEventDialog::createActionFields()
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

QWidget* AddEventDialog::createTestEventFields()
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

QWidget* AddEventDialog::createReminderFields()
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

QWidget* AddEventDialog::createJiraTicketFields()
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

void AddEventDialog::onTypeChanged(int index)
{
    TimelineEventType type = static_cast<TimelineEventType>(
        typeCombo_->itemData(index).toInt());

    showFieldsForType(type);
}

void AddEventDialog::showFieldsForType(TimelineEventType type)
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

void AddEventDialog::onStartDateChanged(const QDate& date)
{
    // This method can be used for additional date validation if needed
}

bool AddEventDialog::validateCommonFields()
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

bool AddEventDialog::validateTypeSpecificFields()
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

    // Other types have no specific validation requirements
    default:
        break;
    }

    return true;
}


void AddEventDialog::validateAndAccept()
{
    //
    if (!validateCommonFields() || !validateTypeSpecificFields())
    {
        return;
    }

    // Validate lane control conflicts
    if (laneControlCheckbox_->isChecked())
    {
        // Get the event data to check for conflicts
        TimelineEvent tempEvent = getEvent();

        // Note: We need access to the model to check conflicts
        // This requires passing the model pointer to the dialog
        // For now, we'll add this validation in the MainWindow when the dialog is accepted
    }

    accept();
}


TimelineEvent AddEventDialog::getEvent() const
{
    TimelineEvent event;

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


void AddEventDialog::populateCommonFields(TimelineEvent& event) const
{
    event.type = static_cast<TimelineEventType>(typeCombo_->currentData().toInt());
    event.title = titleEdit_->text().trimmed();
    event.priority = prioritySpinner_->value();
    event.description = descriptionEdit_->toPlainText().trimmed();
}


void AddEventDialog::populateTypeSpecificFields(TimelineEvent& event) const
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
        // Set endDate to due date for timeline rendering
        event.endDate = event.dueDateTime.date();
        break;

    case TimelineEventType_TestEvent:
        event.startDate = testStartDate_->date();
        event.endDate = testEndDate_->date();
        event.testCategory = testCategoryCombo_->currentText();
        // Collect checklist items
        for (auto it = checklistItems_.begin(); it != checklistItems_.end(); ++it)
        {
            event.preparationChecklist[it.key()] = it.value()->isChecked();
        }
        break;

    case TimelineEventType_Reminder:
        event.reminderDateTime = reminderDateTime_->dateTime();
        event.recurringRule = recurringRuleCombo_->currentText();
        // Set startDate and endDate to reminder date for timeline rendering
        event.startDate = event.reminderDateTime.date();
        event.endDate = event.reminderDateTime.date();
        break;

    case TimelineEventType_JiraTicket:
        event.jiraKey = jiraKeyEdit_->text().trimmed();
        event.jiraSummary = jiraSummaryEdit_->text().trimmed();
        event.jiraType = jiraTypeCombo_->currentText();
        event.jiraStatus = jiraStatusCombo_->currentText();
        event.startDate = jiraStartDate_->date();
        event.endDate = jiraDueDate_->date();  // Due date as end date
        break;
    }
}
