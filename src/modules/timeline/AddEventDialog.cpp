// AddEventDialog.cpp


#include "AddEventDialog.h"
#include <qtimer.h>


// Custom QStackedWidget that only reports current page size
class DynamicStackedWidget : public QStackedWidget
{
public:
    explicit DynamicStackedWidget(QWidget* parent = nullptr) : QStackedWidget(parent) {}

    QSize sizeHint() const override
    {
        QWidget* current = currentWidget();
        return current ? current->sizeHint() : QStackedWidget::sizeHint();
    }

    QSize minimumSizeHint() const override
    {
        QWidget* current = currentWidget();
        return current ? current->minimumSizeHint() : QStackedWidget::minimumSizeHint();
    }
};


// Constructor and destructor remain the same
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
    setFixedWidth(850);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    // ========== MAIN CONTENT WIDGET ==========
    QWidget* contentWidget = new QWidget(this);

    // ========== CREATE 2x2 GRID LAYOUT ==========
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(10, 10, 10, 10);

    // ========== TOP LEFT: EVENT INFORMATION ==========
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
    prioritySpinner_->setValue(2);  // PRESERVED: Default value of 2
    prioritySpinner_->setToolTip("0 = Highest priority, 5 = Lowest priority");
    commonLayout->addRow("Priority:", prioritySpinner_);

    // Description
    descriptionEdit_ = new QTextEdit();
    descriptionEdit_->setPlaceholderText("Enter description...");
    descriptionEdit_->setMaximumHeight(100);
    commonLayout->addRow("Description:", descriptionEdit_);

    gridLayout->addWidget(commonGroup, 0, 0);

    commonGroup->setMinimumWidth(560);
    commonGroup->setMaximumWidth(560);

    // ========== TOP RIGHT: ATTACHMENTS ==========
    QGroupBox* attachmentGroup = new QGroupBox("Attachments (Optional)");
    attachmentGroup->setMinimumWidth(240);
    attachmentGroup->setMaximumWidth(240);
    QVBoxLayout* attachmentLayout = new QVBoxLayout(attachmentGroup);
    attachmentLayout->setSpacing(5);
    attachmentLayout->setContentsMargins(5, 10, 5, 5);

    attachmentWidget_ = new AttachmentListWidget(contentWidget);
    attachmentWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    attachmentLayout->addWidget(attachmentWidget_);
    gridLayout->addWidget(attachmentGroup, 0, 1);

    // ========== BOTTOM LEFT: TYPE-SPECIFIC FIELDS ==========
    QGroupBox* typeSpecificGroup = new QGroupBox("Type-Specific Details");
    typeSpecificGroup->setMinimumWidth(560);
    typeSpecificGroup->setMaximumWidth(560);
    QVBoxLayout* typeSpecificLayout = new QVBoxLayout(typeSpecificGroup);
    typeSpecificLayout->setContentsMargins(5, 10, 5, 5);

    fieldStack_ = new DynamicStackedWidget();
    fieldStack_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    typeSpecificLayout->addWidget(fieldStack_);

    gridLayout->addWidget(typeSpecificGroup, 1, 0);

    // ========== BOTTOM RIGHT: LANE CONTROL ==========
    QGroupBox* laneControlGroup = new QGroupBox("Lane Control (Advanced)");
    laneControlGroup->setMinimumWidth(240);
    laneControlGroup->setMaximumWidth(240);
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
    laneControlLayout->addWidget(laneControlWarningLabel_);

    laneControlLayout->addStretch();

    gridLayout->addWidget(laneControlGroup, 1, 1);

    connect(laneControlCheckbox_, &QCheckBox::toggled, [this](bool checked) {
        manualLaneSpinner_->setEnabled(checked);
    });

    // Set content widget layout
    contentWidget->setLayout(gridLayout);

    // ========== BUTTONS ==========
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // ========== MAIN DIALOG LAYOUT ==========
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 10);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(contentWidget);
    mainLayout->addWidget(buttonBox);

    // ========== CONNECTIONS ==========
    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AddEventDialog::onTypeChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddEventDialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Let Qt calculate optimal size based on content
    adjustSize();

    // Ensure dialog fits on screen
    QSize screenSize = QGuiApplication::primaryScreen()->availableSize();
    int maxHeight = static_cast<int>(screenSize.height() * 0.9);
    int maxWidth = static_cast<int>(screenSize.width() * 0.9);

    if (height() > maxHeight) {
        resize(width(), maxHeight);
    }
    if (width() > maxWidth) {
        resize(maxWidth, height());
    }
}


// createFieldGroups and all create*Fields methods remain the same
void AddEventDialog::createFieldGroups()
{
    fieldStack_->addWidget(createMeetingFields());
    fieldStack_->addWidget(createActionFields());
    fieldStack_->addWidget(createTestEventFields());
    fieldStack_->addWidget(createReminderFields());
    fieldStack_->addWidget(createJiraTicketFields());
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
    meetingAllDayCheckbox_ = new QCheckBox("All-day event");
    endLayout->addWidget(meetingEndDate_);
    endLayout->addWidget(meetingEndTime_);
    endLayout->addWidget(meetingAllDayCheckbox_);
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

    connect(meetingStartDate_, &QDateEdit::dateChanged, [this](const QDate& date) {
        if (meetingEndDate_->date() < date)
            meetingEndDate_->setDate(date);
    });
    connect(meetingAllDayCheckbox_, &QCheckBox::toggled, this, &AddEventDialog::onMeetingAllDayChanged);

    return widget;
}

// createActionFields, createTestEventFields, createReminderFields, createJiraTicketFields
// all remain the same as before - no changes needed

QWidget* AddEventDialog::createActionFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setVerticalSpacing(6);
    layout->setHorizontalSpacing(10);

    QHBoxLayout* startLayout = new QHBoxLayout();
    actionStartDate_ = new QDateEdit(QDate::currentDate());
    actionStartDate_->setCalendarPopup(true);
    actionStartDate_->setMinimumDate(versionStart_);
    actionStartDate_->setMaximumDate(versionEnd_);
    actionStartTime_ = new QTimeEdit(QTime::currentTime());
    startLayout->addWidget(actionStartDate_);
    startLayout->addWidget(actionStartTime_);
    layout->addRow("Start:", startLayout);

    QHBoxLayout* dueLayout = new QHBoxLayout();
    actionDueDate_ = new QDateEdit(QDate::currentDate().addDays(7));
    actionDueDate_->setCalendarPopup(true);
    actionDueDate_->setMinimumDate(versionStart_);
    actionDueDate_->setMaximumDate(versionEnd_);
    actionDueTime_ = new QTimeEdit(QTime::currentTime());
    actionAllDayCheckbox_ = new QCheckBox("All-day event");
    dueLayout->addWidget(actionDueDate_);
    dueLayout->addWidget(actionDueTime_);
    dueLayout->addWidget(actionAllDayCheckbox_);
    layout->addRow("Due:", dueLayout);

    statusCombo_ = new QComboBox();
    statusCombo_->addItems({"Not Started", "In Progress", "Blocked", "Completed"});
    layout->addRow("Status:", statusCombo_);

    connect(actionAllDayCheckbox_, &QCheckBox::toggled, this, &AddEventDialog::onActionAllDayChanged);

    return widget;
}

QWidget* AddEventDialog::createTestEventFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setVerticalSpacing(6);
    layout->setHorizontalSpacing(10);

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

    QHBoxLayout* endLayout = new QHBoxLayout();
    endLayout->setSpacing(5);
    endLayout->setContentsMargins(0, 0, 0, 0);
    testEndDate_ = new QDateEdit(QDate::currentDate().addDays(1));
    testEndDate_->setCalendarPopup(true);
    testEndDate_->setMinimumDate(versionStart_);
    testEndDate_->setMaximumDate(versionEnd_);
    testEndTime_ = new QTimeEdit(QTime(17, 0));
    testAllDayCheckbox_ = new QCheckBox("All-day event");
    endLayout->addWidget(testEndDate_);
    endLayout->addWidget(testEndTime_);
    endLayout->addWidget(testAllDayCheckbox_);
    layout->addRow("End:", endLayout);

    testCategoryCombo_ = new QComboBox();
    testCategoryCombo_->addItems({"Dry Run", "Preliminary", "Formal"});
    layout->addRow("Category:", testCategoryCombo_);

    QGroupBox* checklistGroup = new QGroupBox("Preparation Checklist");
    QGridLayout* checklistLayout = new QGridLayout(checklistGroup);
    checklistLayout->setSpacing(8);
    checklistLayout->setContentsMargins(10, 10, 10, 10);

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

    // Arrange checkboxes in a 2-column grid
    int row = 0;
    int col = 0;
    for (const QString& itemName : checklistItemNames)
    {
        QCheckBox* checkbox = new QCheckBox(itemName);
        checklistLayout->addWidget(checkbox, row, col);
        checklistItems_[itemName] = checkbox;

        col++;
        if (col >= 2) {
            col = 0;
            row++;
        }
    }

    layout->addRow(checklistGroup);

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

    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setVerticalSpacing(6);
    layout->setHorizontalSpacing(10);

    QHBoxLayout* startLayout = new QHBoxLayout();
    startLayout->setSpacing(5);
    startLayout->setContentsMargins(0, 0, 0, 0);
    reminderDate_ = new QDateEdit(QDate::currentDate().addDays(1));
    reminderDate_->setCalendarPopup(true);
    reminderDate_->setMinimumDate(versionStart_);
    reminderDate_->setMaximumDate(versionEnd_);
    reminderTime_ = new QTimeEdit(QTime::currentTime());
    startLayout->addWidget(reminderDate_);
    startLayout->addWidget(reminderTime_);
    layout->addRow("Start:", startLayout);

    QHBoxLayout* endLayout = new QHBoxLayout();
    endLayout->setSpacing(5);
    endLayout->setContentsMargins(0, 0, 0, 0);
    reminderEndDate_ = new QDateEdit(QDate::currentDate().addDays(1));
    reminderEndDate_->setCalendarPopup(true);
    reminderEndDate_->setMinimumDate(versionStart_);
    reminderEndDate_->setMaximumDate(versionEnd_);
    reminderEndTime_ = new QTimeEdit(QTime::currentTime());
    reminderAllDayCheckbox_ = new QCheckBox("All-day event");
    endLayout->addWidget(reminderEndDate_);
    endLayout->addWidget(reminderEndTime_);
    endLayout->addWidget(reminderAllDayCheckbox_);
    layout->addRow("End:", endLayout);

    recurringRuleCombo_ = new QComboBox();
    recurringRuleCombo_->addItems({"None", "Daily", "Weekly", "Monthly"});
    layout->addRow("Recurrence:", recurringRuleCombo_);

    connect(reminderDate_, &QDateEdit::dateChanged, [this](const QDate& date) {
        if (reminderEndDate_->date() < date)
            reminderEndDate_->setDate(date);
    });

    connect(reminderAllDayCheckbox_, &QCheckBox::toggled, this, &AddEventDialog::onReminderAllDayChanged);

    return widget;
}

QWidget* AddEventDialog::createJiraTicketFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setVerticalSpacing(6);
    layout->setHorizontalSpacing(10);

    jiraKeyEdit_ = new QLineEdit();
    jiraKeyEdit_->setPlaceholderText("e.g., ABC-123");
    layout->addRow("Ticket Key:", jiraKeyEdit_);

    jiraSummaryEdit_ = new QLineEdit();
    jiraSummaryEdit_->setPlaceholderText("Brief summary from Jira");
    layout->addRow("Summary:", jiraSummaryEdit_);

    jiraTypeCombo_ = new QComboBox();
    jiraTypeCombo_->addItems({"Story", "Bug", "Task", "Sub-task", "Epic"});
    layout->addRow("Type:", jiraTypeCombo_);

    jiraStatusCombo_ = new QComboBox();
    jiraStatusCombo_->addItems({"To Do", "In Progress", "Done"});
    layout->addRow("Status:", jiraStatusCombo_);

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

    QHBoxLayout* dueLayout = new QHBoxLayout();
    dueLayout->setSpacing(5);
    dueLayout->setContentsMargins(0, 0, 0, 0);
    jiraDueDate_ = new QDateEdit(QDate::currentDate().addDays(14));
    jiraDueDate_->setCalendarPopup(true);
    jiraDueDate_->setMinimumDate(versionStart_);
    jiraDueDate_->setMaximumDate(versionEnd_);
    jiraDueTime_ = new QTimeEdit(QTime(17, 0));
    jiraAllDayCheckbox_ = new QCheckBox("All-day event");
    dueLayout->addWidget(jiraDueDate_);
    dueLayout->addWidget(jiraDueTime_);
    dueLayout->addWidget(jiraAllDayCheckbox_);
    layout->addRow("Due:", dueLayout);

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

    // Resize dialog to fit new content
    QTimer::singleShot(0, this, [this]() {
        adjustSize();

        // Ensure dialog still fits on screen
        QSize screenSize = QGuiApplication::primaryScreen()->availableSize();
        int maxHeight = static_cast<int>(screenSize.height() * 0.9);
        int maxWidth = static_cast<int>(screenSize.width() * 0.9);

        if (height() > maxHeight) {
            resize(width(), maxHeight);
        }
        if (width() > maxWidth) {
            resize(maxWidth, height());
        }
    });
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

    default:
        break;
    }

    return true;
}

void AddEventDialog::validateAndAccept()
{
    if (!validateCommonFields() || !validateTypeSpecificFields())
    {
        return;
    }

    accept();
}

TimelineEvent AddEventDialog::getEvent() const
{
    TimelineEvent event;

    populateCommonFields(event);

    event.laneControlEnabled = laneControlCheckbox_->isChecked();
    event.manualLane = manualLaneSpinner_->value();

    populateTypeSpecificFields(event);

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

// UPDATED: populateTypeSpecificFields - Now creates QDateTime objects
void AddEventDialog::populateTypeSpecificFields(TimelineEvent& event) const
{
    switch (event.type)
    {
    case TimelineEventType_Meeting:
    {
        QTime startTime = meetingAllDayCheckbox_->isChecked() ? QTime(0, 0, 0) : meetingStartTime_->time();
        QTime endTime = meetingAllDayCheckbox_->isChecked() ? QTime(23, 59, 59) : meetingEndTime_->time();

        // NEW: Create QDateTime objects combining date and time
        event.startDate = QDateTime(meetingStartDate_->date(), startTime);
        event.endDate = QDateTime(meetingEndDate_->date(), endTime);

        // Legacy fields for backward compatibility
        event.startTime = startTime;
        event.endTime = endTime;

        event.location = locationEdit_->text().trimmed();
        event.participants = participantsEdit_->toPlainText().trimmed();
        break;
    }

    case TimelineEventType_Action:
    {
        QTime startTime = actionAllDayCheckbox_->isChecked() ? QTime(0, 0, 0) : actionStartTime_->time();
        QTime dueTime = actionAllDayCheckbox_->isChecked() ? QTime(23, 59, 59) : actionDueTime_->time();

        // NEW: Create QDateTime objects
        event.startDate = QDateTime(actionStartDate_->date(), startTime);
        event.dueDateTime = QDateTime(actionDueDate_->date(), dueTime);

        // Set endDate to dueDateTime for timeline rendering
        event.endDate = event.dueDateTime;

        // Legacy fields
        event.startTime = startTime;
        event.endTime = dueTime;

        event.status = statusCombo_->currentText();
        break;
    }

    case TimelineEventType_TestEvent:
    {
        QTime startTime = testAllDayCheckbox_->isChecked() ? QTime(0, 0, 0) : testStartTime_->time();
        QTime endTime = testAllDayCheckbox_->isChecked() ? QTime(23, 59, 59) : testEndTime_->time();

        // NEW: Create QDateTime objects
        event.startDate = QDateTime(testStartDate_->date(), startTime);
        event.endDate = QDateTime(testEndDate_->date(), endTime);

        // Legacy fields
        event.startTime = startTime;
        event.endTime = endTime;

        event.testCategory = testCategoryCombo_->currentText();

        // Collect checklist items
        for (auto it = checklistItems_.begin(); it != checklistItems_.end(); ++it)
        {
            event.preparationChecklist[it.key()] = it.value()->isChecked();
        }
        break;
    }

    case TimelineEventType_Reminder:
    {
        QTime startTime = reminderAllDayCheckbox_->isChecked() ? QTime(9, 0, 0) : reminderTime_->time();
        QTime endTime = reminderAllDayCheckbox_->isChecked() ? QTime(23, 59, 59) : reminderEndTime_->time();

        event.reminderDateTime = QDateTime(reminderDate_->date(), startTime);

        // NEW: Set startDate and endDate for timeline rendering
        event.startDate = event.reminderDateTime;
        event.endDate = QDateTime(reminderEndDate_->date(), endTime);

        // Legacy field
        event.endTime = endTime;

        event.recurringRule = recurringRuleCombo_->currentText();
        break;
    }

    case TimelineEventType_JiraTicket:
    {
        QTime startTime = jiraAllDayCheckbox_->isChecked() ? QTime(0, 0, 0) : jiraStartTime_->time();
        QTime endTime = jiraAllDayCheckbox_->isChecked() ? QTime(23, 59, 59) : jiraDueTime_->time();

        // NEW: Create QDateTime objects
        event.startDate = QDateTime(jiraStartDate_->date(), startTime);
        event.endDate = QDateTime(jiraDueDate_->date(), endTime);

        // Legacy fields
        event.startTime = startTime;
        event.endTime = endTime;

        event.jiraKey = jiraKeyEdit_->text().trimmed();
        event.jiraSummary = jiraSummaryEdit_->text().trimmed();
        event.jiraType = jiraTypeCombo_->currentText();
        event.jiraStatus = jiraStatusCombo_->currentText();
        break;
    }
    }
}

// All-day checkbox handlers remain the same
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
    reminderEndTime_->setEnabled(!checked);
}

void AddEventDialog::onJiraAllDayChanged(bool checked)
{
    jiraStartTime_->setEnabled(!checked);
    jiraDueTime_->setEnabled(!checked);
}
