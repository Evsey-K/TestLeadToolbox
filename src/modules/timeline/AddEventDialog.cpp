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

    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
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

    // All-Day Event Checkbox
    meetingAllDayCheckbox_ = new QCheckBox("All-day event");
    layout->addRow("", meetingAllDayCheckbox_);

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

    // Connect signals
    connect(meetingStartDate_, &QDateEdit::dateChanged, [this](const QDate& date) {
        if (meetingEndDate_->date() < date)
            meetingEndDate_->setDate(date);
    });
    connect(meetingAllDayCheckbox_, &QCheckBox::toggled, this, &AddEventDialog::onMeetingAllDayChanged);

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
    QHBoxLayout* dueLayout = new QHBoxLayout();
    actionDueDate_ = new QDateEdit(QDate::currentDate().addDays(7));
    actionDueDate_->setCalendarPopup(true);
    actionDueDate_->setMinimumDate(versionStart_);
    actionDueDate_->setMaximumDate(versionEnd_);
    actionDueTime_ = new QTimeEdit(QTime::currentTime());
    dueLayout->addWidget(actionDueDate_);
    dueLayout->addWidget(actionDueTime_);
    layout->addRow("Due:", dueLayout);

    // All-Day Event Checkbox
    actionAllDayCheckbox_ = new QCheckBox("All-day event");
    layout->addRow("", actionAllDayCheckbox_);

    // Status
    statusCombo_ = new QComboBox();
    statusCombo_->addItems({"Not Started", "In Progress", "Blocked", "Completed"});
    layout->addRow("Status:", statusCombo_);

    // Connect signals
    connect(actionAllDayCheckbox_, &QCheckBox::toggled, this, &AddEventDialog::onActionAllDayChanged);

    return widget;
}


QWidget* AddEventDialog::createTestEventFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    // Start Date/Time
    QHBoxLayout* startLayout = new QHBoxLayout();
    startLayout->setSpacing(5);
    startLayout->setContentsMargins(0, 0, 0, 0);
    testStartDate_ = new QDateEdit(QDate::currentDate());
    testStartDate_->setCalendarPopup(true);
    testStartDate_->setMinimumDate(versionStart_);
    testStartDate_->setMaximumDate(versionEnd_);
    testStartTime_ = new QTimeEdit(QTime(8, 0));
    startLayout->addWidget(testStartDate_);
    startLayout->addWidget(testStartTime_);
    layout->addRow("Start:", startLayout);

    // End Date/Time
    QHBoxLayout* endLayout = new QHBoxLayout();
    endLayout->setSpacing(5);
    endLayout->setContentsMargins(0, 0, 0, 0);
    testEndDate_ = new QDateEdit(QDate::currentDate().addDays(1));
    testEndDate_->setCalendarPopup(true);
    testEndDate_->setMinimumDate(versionStart_);
    testEndDate_->setMaximumDate(versionEnd_);
    testEndTime_ = new QTimeEdit(QTime(17, 0));
    endLayout->addWidget(testEndDate_);
    endLayout->addWidget(testEndTime_);
    layout->addRow("End:", endLayout);

    // All-Day Event Checkbox
    testAllDayCheckbox_ = new QCheckBox("All-day event");
    layout->addRow("", testAllDayCheckbox_);

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
    connect(testAllDayCheckbox_, &QCheckBox::toggled, this, &AddEventDialog::onTestAllDayChanged);

    return widget;
}


QWidget* AddEventDialog::createReminderFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    // Reminder Date/Time
    QHBoxLayout* reminderLayout = new QHBoxLayout();
    reminderLayout->setSpacing(5);
    reminderLayout->setContentsMargins(0, 0, 0, 0);
    reminderDate_ = new QDateEdit(QDate::currentDate().addDays(1));
    reminderDate_->setCalendarPopup(true);
    reminderDate_->setMinimumDate(versionStart_);
    reminderDate_->setMaximumDate(versionEnd_);
    reminderTime_ = new QTimeEdit(QTime::currentTime());
    reminderLayout->addWidget(reminderDate_);
    reminderLayout->addWidget(reminderTime_);
    layout->addRow("Reminder:", reminderLayout);

    // All-Day Event Checkbox
    reminderAllDayCheckbox_ = new QCheckBox("All-day event");
    layout->addRow("", reminderAllDayCheckbox_);

    // Recurring Rule
    recurringRuleCombo_ = new QComboBox();
    recurringRuleCombo_->addItems({"None", "Daily", "Weekly", "Monthly"});
    layout->addRow("Recurrence:", recurringRuleCombo_);

    // Connect signals
    connect(reminderAllDayCheckbox_, &QCheckBox::toggled, this, &AddEventDialog::onReminderAllDayChanged);

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

    // Start Date/Time
    QHBoxLayout* startLayout = new QHBoxLayout();
    startLayout->setSpacing(5);
    startLayout->setContentsMargins(0, 0, 0, 0);
    jiraStartDate_ = new QDateEdit(QDate::currentDate());
    jiraStartDate_->setCalendarPopup(true);
    jiraStartDate_->setMinimumDate(versionStart_);
    jiraStartDate_->setMaximumDate(versionEnd_);
    jiraStartTime_ = new QTimeEdit(QTime(9, 0));
    startLayout->addWidget(jiraStartDate_);
    startLayout->addWidget(jiraStartTime_);
    layout->addRow("Start:", startLayout);

    // Due Date/Time
    QHBoxLayout* dueLayout = new QHBoxLayout();
    dueLayout->setSpacing(5);
    dueLayout->setContentsMargins(0, 0, 0, 0);
    jiraDueDate_ = new QDateEdit(QDate::currentDate().addDays(14));
    jiraDueDate_->setCalendarPopup(true);
    jiraDueDate_->setMinimumDate(versionStart_);
    jiraDueDate_->setMaximumDate(versionEnd_);
    jiraDueTime_ = new QTimeEdit(QTime(17, 0));
    dueLayout->addWidget(jiraDueDate_);
    dueLayout->addWidget(jiraDueTime_);
    layout->addRow("Due:", dueLayout);

    // All-Day Event Checkbox
    jiraAllDayCheckbox_ = new QCheckBox("All-day event");
    layout->addRow("", jiraAllDayCheckbox_);

    // Connect signals
    connect(jiraStartDate_, &QDateEdit::dateChanged, [this](const QDate& date) {
        if (jiraDueDate_->date() < date)
            jiraDueDate_->setDate(date);
    });
    connect(jiraAllDayCheckbox_, &QCheckBox::toggled, this, &AddEventDialog::onJiraAllDayChanged);

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
            !meetingAllDayCheckbox_->isChecked() &&
            meetingEndTime_->time() < meetingStartTime_->time())
        {
            QMessageBox::warning(this, "Validation Error",
                                 "End time cannot be before start time on the same day.");
            return false;
        }
        break;

    case TimelineEventType_Action:
    {
        if (actionDueDate_->date() < actionStartDate_->date())
        {
            QMessageBox::warning(this, "Validation Error",
                                 "Due date cannot be before start date.");
            return false;
        }
        if (actionDueDate_->date() == actionStartDate_->date() &&
            !actionAllDayCheckbox_->isChecked())
        {
            QDateTime startDateTime(actionStartDate_->date(), actionStartTime_->time());
            QDateTime dueDateTime(actionDueDate_->date(), actionDueTime_->time());
            if (dueDateTime < startDateTime)
            {
                QMessageBox::warning(this, "Validation Error",
                                     "Due time must be after start time on the same day.");
                return false;
            }
        }
        break;
    }

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

        if (!meetingAllDayCheckbox_->isChecked())
        {
            event.startTime = meetingStartTime_->time();
            event.endTime = meetingEndTime_->time();
        }
        else
        {
            event.startTime = QTime(0, 0);
            event.endTime = QTime(23, 59);
        }

        event.location = locationEdit_->text().trimmed();
        event.participants = participantsEdit_->toPlainText().trimmed();
        break;

    case TimelineEventType_Action:
        event.startDate = actionStartDate_->date();

        if (!actionAllDayCheckbox_->isChecked())
        {
            event.startTime = actionStartTime_->time();
            event.dueDateTime = QDateTime(actionDueDate_->date(), actionDueTime_->time());
        }
        else
        {
            event.startTime = QTime(0, 0);
            event.dueDateTime = QDateTime(actionDueDate_->date(), QTime(23, 59));
        }

        event.status = statusCombo_->currentText();
        // Set endDate to due date for timeline rendering
        event.endDate = event.dueDateTime.date();
        break;

    case TimelineEventType_TestEvent:
        event.startDate = testStartDate_->date();
        event.endDate = testEndDate_->date();

        if (!testAllDayCheckbox_->isChecked())
        {
            event.startTime = testStartTime_->time();
            event.endTime = testEndTime_->time();
        }
        else
        {
            event.startTime = QTime(0, 0);
            event.endTime = QTime(23, 59);
        }

        event.testCategory = testCategoryCombo_->currentText();

        // Collect checklist items
        for (auto it = checklistItems_.begin(); it != checklistItems_.end(); ++it)
        {
            event.preparationChecklist[it.key()] = it.value()->isChecked();
        }
        break;

    case TimelineEventType_Reminder:
        if (!reminderAllDayCheckbox_->isChecked())
        {
            event.reminderDateTime = QDateTime(reminderDate_->date(), reminderTime_->time());
        }
        else
        {
            event.reminderDateTime = QDateTime(reminderDate_->date(), QTime(9, 0)); // Default to 9 AM
        }

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
        event.endDate = jiraDueDate_->date();

        if (!jiraAllDayCheckbox_->isChecked())
        {
            event.startTime = jiraStartTime_->time();
            event.endTime = jiraDueTime_->time();
        }
        else
        {
            event.startTime = QTime(0, 0);
            event.endTime = QTime(23, 59);
        }
        break;
    }
}


void AddEventDialog::onMeetingAllDayChanged(bool checked)
{
    meetingStartTime_->setEnabled(!checked);
    meetingEndTime_->setEnabled(!checked);
}


void AddEventDialog::onActionAllDayChanged(bool checked)
{
    actionStartTime_->setEnabled(!checked);
    actionDueTime_->setEnabled(!checked);
}


void AddEventDialog::onTestAllDayChanged(bool checked)
{
    testStartTime_->setEnabled(!checked);
    testEndTime_->setEnabled(!checked);
}


void AddEventDialog::onReminderAllDayChanged(bool checked)
{
    reminderTime_->setEnabled(!checked);
}


void AddEventDialog::onJiraAllDayChanged(bool checked)
{
    jiraStartTime_->setEnabled(!checked);
    jiraDueTime_->setEnabled(!checked);
}
