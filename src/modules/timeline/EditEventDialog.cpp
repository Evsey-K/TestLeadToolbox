// EditEventDialog.cpp


#include "EditEventDialog.h"
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

    typeCombo_ = new QComboBox();
    typeCombo_->addItem("Meeting", TimelineEventType_Meeting);
    typeCombo_->addItem("Action", TimelineEventType_Action);
    typeCombo_->addItem("Test Event", TimelineEventType_TestEvent);
    typeCombo_->addItem("Reminder", TimelineEventType_Reminder);
    typeCombo_->addItem("Jira Ticket", TimelineEventType_JiraTicket);
    commonLayout->addRow("Type:", typeCombo_);

    titleEdit_ = new QLineEdit();
    titleEdit_->setPlaceholderText("Enter event title...");
    commonLayout->addRow("Title:", titleEdit_);

    prioritySpinner_ = new QSpinBox();
    prioritySpinner_->setRange(0, 5);
    prioritySpinner_->setToolTip("0 = Highest priority, 5 = Lowest priority");
    commonLayout->addRow("Priority:", prioritySpinner_);

    descriptionEdit_ = new QTextEdit();
    descriptionEdit_->setPlaceholderText("Enter description...");
    descriptionEdit_->setMaximumHeight(100);
    commonLayout->addRow("Description:", descriptionEdit_);

    gridLayout->addWidget(commonGroup, 0, 0);

    // Set width for left column (70% of 850px = ~595px)
    commonGroup->setMinimumWidth(560);
    commonGroup->setMaximumWidth(560);

    // ========== TOP RIGHT: ATTACHMENTS ==========
    QGroupBox* attachmentGroup = new QGroupBox("Attachments");
    attachmentGroup->setMinimumWidth(260);
    attachmentGroup->setMaximumWidth(260);
    QVBoxLayout* attachmentLayout = new QVBoxLayout(attachmentGroup);
    attachmentLayout->setSpacing(5);
    attachmentLayout->setContentsMargins(5, 10, 5, 5);

    attachmentWidget_ = new AttachmentListWidget(contentWidget);
    attachmentWidget_->setEventId(eventId_);
    attachmentWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    attachmentLayout->addWidget(attachmentWidget_);
    gridLayout->addWidget(attachmentGroup, 0, 1);

    qDebug() << "═══════════════════════════════════════════════════════";
    qDebug() << "EditEventDialog::setupUi() - Attachment Widget Setup";
    qDebug() << "  Event ID (UUID):" << eventId_;
    qDebug() << "  Numeric Hash:" << qHash(eventId_);
    qDebug() << "  Attachment Count:" << AttachmentManager::instance().getAttachmentCount(qHash(eventId_));
    qDebug() << "═══════════════════════════════════════════════════════";

    connect(attachmentWidget_, &AttachmentListWidget::attachmentsChanged, this, &EditEventDialog::onAttachmentsChanged);

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
    laneControlGroup->setMinimumWidth(260);
    laneControlGroup->setMaximumWidth(260);
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

    // ========== LOCK STATE CONTROLS ==========
    laneControlLayout->addSpacing(15);

    fixedCheckbox_ = new QCheckBox("Fixed");
    fixedCheckbox_->setToolTip("Prevent dragging and resizing in timeline view");
    laneControlLayout->addWidget(fixedCheckbox_);

    lockedCheckbox_ = new QCheckBox("Locked");
    lockedCheckbox_->setToolTip("Prevent all editing (includes Fixed)");
    laneControlLayout->addWidget(lockedCheckbox_);

    laneControlLayout->addStretch();

    gridLayout->addWidget(laneControlGroup, 1, 1);

    connect(laneControlCheckbox_, &QCheckBox::toggled, [this](bool checked) {
        manualLaneSpinner_->setEnabled(checked);
    });

    // Set content widget layout
    contentWidget->setLayout(gridLayout);

    // ========== BUTTON BOX ==========

    // Custom horizontal layout for buttons instead of QDialogButtonBox
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(10, 0, 10, 0);

    // Delete button on the LEFT side
    deleteButton_ = new QPushButton("Delete");
    deleteButton_->setStyleSheet(
        "QPushButton { "
        "    background-color: #dc3545; "
        "    color: white; "
        "    font-weight: bold; "
        "    padding: 6px 12px; "
        "} "
        "QPushButton:hover { "
        "    background-color: #c82333; "
        "} "
        "QPushButton:pressed { "
        "    background-color: #bd2130; "
        "}"
        );

    buttonLayout->addWidget(deleteButton_);
    buttonLayout->addStretch();  // Push remaining buttons to the right

    // Apply, Save, and Cancel buttons on the RIGHT side
    applyButton_ = new QPushButton("Apply");
    applyButton_->setToolTip("Apply changes and keep dialog open");

    QPushButton* saveButton = new QPushButton("Save");
    saveButton->setDefault(true);
    saveButton->setToolTip("Apply changes and close dialog");

    QPushButton* cancelButton = new QPushButton("Cancel");
    cancelButton->setToolTip("Discard changes and close dialog");

    buttonLayout->addWidget(applyButton_);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);

    // ========== MAIN DIALOG LAYOUT ==========
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 15);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(contentWidget);
    mainLayout->addLayout(buttonLayout);

    // ========== CONNECTIONS ==========
    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EditEventDialog::onTypeChanged);
    connect(applyButton_, &QPushButton::clicked, this, &EditEventDialog::onApplyClicked);
    connect(saveButton, &QPushButton::clicked, this, &EditEventDialog::validateAndAccept);
    connect(deleteButton_, &QPushButton::clicked, this, &EditEventDialog::onDeleteClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(laneControlCheckbox_, &QCheckBox::toggled, [this](bool checked) { manualLaneSpinner_->setEnabled(checked); });

    // Lock state checkbox connections
    connect(lockedCheckbox_, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            // Auto-check fixed when locked
            fixedCheckbox_->setChecked(true);
            fixedCheckbox_->setEnabled(false);
        } else {
            // Re-enable fixed checkbox when unlocked
            fixedCheckbox_->setEnabled(true);
        }
        updateFieldsEnabledState();
    });

    connect(fixedCheckbox_, &QCheckBox::toggled, this, [this](bool checked) {
        Q_UNUSED(checked);
        // Fixed state doesn't affect other fields, only view manipulation
    });

    // Let Qt calculate optimal size based on content
    adjustSize();

    // Ensure dialog fits on screen and is positioned properly
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect availableGeometry = screen->availableGeometry();
    int maxHeight = static_cast<int>(availableGeometry.height() * 0.9);
    int maxWidth = static_cast<int>(availableGeometry.width() * 0.9);

    if (height() > maxHeight) {
        resize(width(), maxHeight);
    }
    if (width() > maxWidth) {
        resize(maxWidth, height());
    }

    // Center the dialog on screen to ensure it's fully visible
    QRect dialogGeometry = geometry();
    dialogGeometry.moveCenter(availableGeometry.center());

    // Ensure top is not off-screen
    if (dialogGeometry.top() < availableGeometry.top()) {
        dialogGeometry.moveTop(availableGeometry.top());
    }

    // Ensure bottom is not off-screen
    if (dialogGeometry.bottom() > availableGeometry.bottom()) {
        dialogGeometry.moveBottom(availableGeometry.bottom());
    }

    setGeometry(dialogGeometry);
}


void EditEventDialog::createFieldGroups()
{
    fieldStack_->addWidget(createMeetingFields());
    fieldStack_->addWidget(createActionFields());
    fieldStack_->addWidget(createTestEventFields());
    fieldStack_->addWidget(createReminderFields());
    fieldStack_->addWidget(createJiraTicketFields());
}


QWidget* EditEventDialog::createMeetingFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setVerticalSpacing(6);
    layout->setHorizontalSpacing(10);

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

    locationEdit_ = new QLineEdit();
    locationEdit_->setPlaceholderText("Physical location or virtual link");
    layout->addRow("Location:", locationEdit_);

    participantsEdit_ = new QTextEdit();
    participantsEdit_->setPlaceholderText("Enter attendee names/emails...");
    participantsEdit_->setMaximumHeight(60);
    participantsEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addRow("Participants:", participantsEdit_);

    connect(meetingStartDate_, &QDateEdit::dateChanged, [this](const QDate& date) {
        if (meetingEndDate_->date() < date)
            meetingEndDate_->setDate(date);
    });
    connect(meetingAllDayCheckbox_, &QCheckBox::toggled, this, &EditEventDialog::onMeetingAllDayChanged);

    return widget;
}


QWidget* EditEventDialog::createActionFields()
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

    connect(actionAllDayCheckbox_, &QCheckBox::toggled, this, &EditEventDialog::onActionAllDayChanged);

    return widget;
}


QWidget* EditEventDialog::createTestEventFields()
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
    connect(testAllDayCheckbox_, &QCheckBox::toggled, this, &EditEventDialog::onTestAllDayChanged);

    return widget;
}


QWidget* EditEventDialog::createReminderFields()
{
    QWidget* widget = new QWidget();
    QFormLayout* layout = new QFormLayout(widget);

    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setVerticalSpacing(6);
    layout->setHorizontalSpacing(10);

    QHBoxLayout* startLayout = new QHBoxLayout();
    startLayout->setSpacing(5);
    startLayout->setContentsMargins(0, 0, 0, 0);
    reminderDate_ = new QDateEdit(QDateTime::currentDateTime().addDays(1).date());
    reminderDate_->setCalendarPopup(true);
    reminderDate_->setMinimumDate(versionStart_);
    reminderDate_->setMaximumDate(versionEnd_);
    reminderTime_ = new QTimeEdit(QDateTime::currentDateTime().time());
    startLayout->addWidget(reminderDate_);
    startLayout->addWidget(reminderTime_);
    layout->addRow("Start:", startLayout);

    QHBoxLayout* endLayout = new QHBoxLayout();
    endLayout->setSpacing(5);
    endLayout->setContentsMargins(0, 0, 0, 0);
    reminderEndDate_ = new QDateEdit(QDateTime::currentDateTime().addDays(1).date());
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

    connect(reminderAllDayCheckbox_, &QCheckBox::toggled, this, &EditEventDialog::onReminderAllDayChanged);

    return widget;
}


QWidget* EditEventDialog::createJiraTicketFields()
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
    connect(jiraAllDayCheckbox_, &QCheckBox::toggled, this, &EditEventDialog::onJiraAllDayChanged);

    return widget;
}


// PopulateFromEvent - Extract time from QDateTime fields
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
    descriptionEdit_->setPlainText(event.description);

    // Load lock state
    fixedCheckbox_->setChecked(event.isFixed);
    lockedCheckbox_->setChecked(event.isLocked);

    // Update field states based on lock status
    updateFieldsEnabledState();

    // Set type-specific fields
    switch (event.type)
    {
    case TimelineEventType_Meeting:
    {
        // UPDATED: Extract date and time from QDateTime
        meetingStartDate_->setDate(event.startDate.date());
        meetingStartTime_->setTime(event.startDate.time());
        meetingEndDate_->setDate(event.endDate.date());
        meetingEndTime_->setTime(event.endDate.time());

        locationEdit_->setText(event.location);
        participantsEdit_->setPlainText(event.participants);

        // Set all-day checkbox based on time values
        bool isAllDay = (event.startDate.time() == QTime(0, 0, 0) &&
                         event.endDate.time() == QTime(23, 59, 59));
        meetingAllDayCheckbox_->setChecked(isAllDay);
        break;
    }

    case TimelineEventType_Action:
    {
        // UPDATED: Extract date and time from QDateTime
        actionStartDate_->setDate(event.startDate.date());
        actionStartTime_->setTime(event.startDate.time());

        if (event.dueDateTime.isValid())
        {
            actionDueDate_->setDate(event.dueDateTime.date());
            actionDueTime_->setTime(event.dueDateTime.time());
        }

        // Set status combo
        for (int i = 0; i < statusCombo_->count(); ++i)
        {
            if (statusCombo_->itemText(i) == event.status)
            {
                statusCombo_->setCurrentIndex(i);
                break;
            }
        }

        // Set all-day checkbox
        bool isActionAllDay = (event.startDate.time() == QTime(0, 0, 0));
        actionAllDayCheckbox_->setChecked(isActionAllDay);
        break;
    }

    case TimelineEventType_TestEvent:
    {
        // UPDATED: Extract date and time from QDateTime
        testStartDate_->setDate(event.startDate.date());
        testStartTime_->setTime(event.startDate.time());
        testEndDate_->setDate(event.endDate.date());
        testEndTime_->setTime(event.endDate.time());

        // Set category combo
        for (int i = 0; i < testCategoryCombo_->count(); ++i)
        {
            if (testCategoryCombo_->itemText(i) == event.testCategory)
            {
                testCategoryCombo_->setCurrentIndex(i);
                break;
            }
        }

        // Populate checklist
        for (auto it = event.preparationChecklist.begin(); it != event.preparationChecklist.end(); ++it)
        {
            if (checklistItems_.contains(it.key()))
            {
                checklistItems_[it.key()]->setChecked(it.value());
            }
        }

        // Set all-day checkbox
        bool isTestAllDay = (event.startDate.time() == QTime(0, 0, 0) &&
                             event.endDate.time() == QTime(23, 59, 59));
        testAllDayCheckbox_->setChecked(isTestAllDay);
        break;
    }

    case TimelineEventType_Reminder:
    {
        // UPDATED: Extract date and time from reminderDateTime
        reminderDate_->setDate(event.reminderDateTime.date());
        reminderTime_->setTime(event.reminderDateTime.time());

        // Extract end date/time from endDate QDateTime
        reminderEndDate_->setDate(event.endDate.date());
        reminderEndTime_->setTime(event.endDate.time());

        // Populate recurring rule
        recurringRuleCombo_->setCurrentText(event.recurringRule);

        // Set all-day checkbox based on time
        bool isReminderAllDay = (event.reminderDateTime.time() == QTime(9, 0) ||
                                 event.reminderDateTime.time() == QTime(0, 0));
        reminderAllDayCheckbox_->setChecked(isReminderAllDay);
        break;
    }

    case TimelineEventType_JiraTicket:
    {
        // UPDATED: Extract date and time from QDateTime
        jiraStartDate_->setDate(event.startDate.date());
        jiraStartTime_->setTime(event.startDate.time());
        jiraDueDate_->setDate(event.endDate.date());
        jiraDueTime_->setTime(event.endDate.time());

        jiraKeyEdit_->setText(event.jiraKey);
        jiraSummaryEdit_->setText(event.jiraSummary);

        // Set type combo
        for (int i = 0; i < jiraTypeCombo_->count(); ++i)
        {
            if (jiraTypeCombo_->itemText(i) == event.jiraType)
            {
                jiraTypeCombo_->setCurrentIndex(i);
                break;
            }
        }

        // Set status combo
        for (int i = 0; i < jiraStatusCombo_->count(); ++i)
        {
            if (jiraStatusCombo_->itemText(i) == event.jiraStatus)
            {
                jiraStatusCombo_->setCurrentIndex(i);
                break;
            }
        }

        // Set all-day checkbox
        bool isJiraAllDay = (event.startDate.time() == QTime(0, 0, 0) &&
                             event.endDate.time() == QTime(23, 59, 59));
        jiraAllDayCheckbox_->setChecked(isJiraAllDay);
        break;
    }
    }
}


void EditEventDialog::onTypeChanged(int index)
{
    TimelineEventType newType = static_cast<TimelineEventType>(
        typeCombo_->itemData(index).toInt());

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

    // Resize dialog to fit new content
    QTimer::singleShot(0, this, [this]() {
        adjustSize();

        QScreen* screen = QGuiApplication::primaryScreen();
        QRect availableGeometry = screen->availableGeometry();
        int maxHeight = static_cast<int>(availableGeometry.height() * 0.9);
        int maxWidth = static_cast<int>(availableGeometry.width() * 0.9);

        if (height() > maxHeight) {
            resize(width(), maxHeight);
        }
        if (width() > maxWidth) {
            resize(maxWidth, height());
        }

        // Center the dialog on screen to ensure it's fully visible
        QRect dialogGeometry = geometry();
        dialogGeometry.moveCenter(availableGeometry.center());

        // Ensure top is not off-screen
        if (dialogGeometry.top() < availableGeometry.top()) {
            dialogGeometry.moveTop(availableGeometry.top());
        }

        // Ensure bottom is not off-screen
        if (dialogGeometry.bottom() > availableGeometry.bottom()) {
            dialogGeometry.moveBottom(availableGeometry.bottom());
        }

        setGeometry(dialogGeometry);
    });
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
    {
        QDateTime startDateTime(actionStartDate_->date(), actionStartTime_->time());
        QDateTime dueDateTime(actionDueDate_->date(), actionDueTime_->time());

        if (dueDateTime < startDateTime)
        {
            QMessageBox::warning(this, "Validation Error",
                                 "Due date/time must be after start date/time.");
            return false;
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


void EditEventDialog::validateAndAccept()
{
    if (!validateCommonFields() || !validateTypeSpecificFields())
    {
        return;
    }

    // NEW: Check for lane conflicts if lane control is enabled
    if (laneControlCheckbox_->isChecked())
    {
        // Build a temporary event to get the updated dates/times
        TimelineEvent tempEvent;
        populateCommonFields(tempEvent);
        populateTypeSpecificFields(tempEvent);

        int manualLane = manualLaneSpinner_->value();

        bool hasConflict = model_->hasLaneConflict(
            tempEvent.startDate,
            tempEvent.endDate,
            manualLane,
            eventId_  // Exclude current event from conflict check
            );

        if (hasConflict)
        {
            QMessageBox::warning(
                this,
                "Lane Conflict",
                QString("Another manually-controlled event already occupies lane %1 "
                        "during this time period.\n\n"
                        "Please choose a different lane number or adjust the event dates.")
                    .arg(manualLane)
                );
            return;  // Don't accept the dialog
        }
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


void EditEventDialog::onApplyClicked()
{
    if (!validateCommonFields() || !validateTypeSpecificFields())
    {
        return;
    }

    // Check for lane conflicts if lane control is enabled
    if (laneControlCheckbox_->isChecked())
    {
        // Build a temporary event to get the updated dates/times
        TimelineEvent tempEvent;
        populateCommonFields(tempEvent);
        populateTypeSpecificFields(tempEvent);

        int manualLane = manualLaneSpinner_->value();

        bool hasConflict = model_->hasLaneConflict(
            tempEvent.startDate,
            tempEvent.endDate,
            manualLane,
            eventId_  // Exclude current event from conflict check
            );

        if (hasConflict)
        {
            QMessageBox::warning(
                this,
                "Lane Conflict",
                QString("Another manually-controlled event already occupies lane %1 "
                        "during this time period.\n\n"
                        "Please choose a different lane number or adjust the event dates.")
                    .arg(manualLane)
                );
            return;  // Don't apply changes
        }
    }

    // Emit signal with updated event data (keep dialog open)
    TimelineEvent updatedEvent = getEvent();
    emit applyRequested(updatedEvent);
}


TimelineEvent EditEventDialog::getEvent() const
{
    TimelineEvent event;

    event.id = eventId_;

    populateCommonFields(event);

    event.laneControlEnabled = laneControlCheckbox_->isChecked();
    event.manualLane = manualLaneSpinner_->value();

    // Save lock state
    event.isFixed = fixedCheckbox_->isChecked();
    event.isLocked = lockedCheckbox_->isChecked();

    // When lane control is enabled, set lane to match manualLane
    if (event.laneControlEnabled)
    {
        event.lane = event.manualLane;
    }

    populateTypeSpecificFields(event);

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


// UPDATED: populateTypeSpecificFields - Create QDateTime objects (same as AddEventDialog)
void EditEventDialog::populateTypeSpecificFields(TimelineEvent& event) const
{
    switch (event.type)
    {
    case TimelineEventType_Meeting:
    {
        QTime startTime = meetingAllDayCheckbox_->isChecked() ? QTime(0, 0, 0) : meetingStartTime_->time();
        QTime endTime = meetingAllDayCheckbox_->isChecked() ? QTime(23, 59, 59) : meetingEndTime_->time();

        // NEW: Create QDateTime objects
        event.startDate = QDateTime(meetingStartDate_->date(), startTime);
        event.endDate = QDateTime(meetingEndDate_->date(), endTime);

        // Legacy fields
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
        event.startDate = event.reminderDateTime;
        event.endDate = QDateTime(reminderEndDate_->date(), endTime);

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
void EditEventDialog::onMeetingAllDayChanged(bool checked)
{
    meetingStartTime_->setEnabled(!checked);
    meetingEndTime_->setEnabled(!checked);
}


void EditEventDialog::onActionAllDayChanged(bool checked)
{
    actionStartTime_->setEnabled(!checked);
    actionDueTime_->setEnabled(!checked);
}


void EditEventDialog::onTestAllDayChanged(bool checked)
{
    testStartTime_->setEnabled(!checked);
    testEndTime_->setEnabled(!checked);
}


void EditEventDialog::onReminderAllDayChanged(bool checked)
{
    reminderTime_->setEnabled(!checked);
    reminderEndTime_->setEnabled(!checked);
}


void EditEventDialog::onJiraAllDayChanged(bool checked)
{
    jiraStartTime_->setEnabled(!checked);
    jiraDueTime_->setEnabled(!checked);
}


void EditEventDialog::onAttachmentsChanged()
{
    qDebug() << "EditEventDialog: Attachments changed for event" << eventId_;

    // The attachment count will update automatically through the signal chain:
    // AttachmentManager -> TimelineModel -> TimelineScene -> TimelineItem
    // No additional action needed here
}


void EditEventDialog::updateFieldsEnabledState()
{
    bool locked = lockedCheckbox_->isChecked();
    bool enabled = !locked;

    // Disable all input fields when locked
    titleEdit_->setEnabled(enabled);
    typeCombo_->setEnabled(enabled);
    prioritySpinner_->setEnabled(enabled);
    descriptionEdit_->setEnabled(enabled);

    // Lane control fields
    laneControlCheckbox_->setEnabled(enabled);
    manualLaneSpinner_->setEnabled(enabled && laneControlCheckbox_->isChecked());

    // Type-specific fields - Meeting
    if (meetingStartDate_) {
        meetingStartDate_->setEnabled(enabled);
        meetingStartTime_->setEnabled(enabled);
        meetingEndDate_->setEnabled(enabled);
        meetingEndTime_->setEnabled(enabled);
        meetingAllDayCheckbox_->setEnabled(enabled);
        locationEdit_->setEnabled(enabled);
        participantsEdit_->setEnabled(enabled);
    }

    // Type-specific fields - Action
    if (actionStartDate_) {
        actionStartDate_->setEnabled(enabled);
        actionStartTime_->setEnabled(enabled);
        actionDueDate_->setEnabled(enabled);
        actionDueTime_->setEnabled(enabled);
        actionAllDayCheckbox_->setEnabled(enabled);
        statusCombo_->setEnabled(enabled);
    }

    // Type-specific fields - Test Event
    if (testStartDate_) {
        testStartDate_->setEnabled(enabled);
        testStartTime_->setEnabled(enabled);
        testEndDate_->setEnabled(enabled);
        testEndTime_->setEnabled(enabled);
        testAllDayCheckbox_->setEnabled(enabled);
        testCategoryCombo_->setEnabled(enabled);

        // Disable all checklist items
        for (auto* checkbox : checklistItems_) {
            checkbox->setEnabled(enabled);
        }
    }

    // Type-specific fields - Reminder
    if (reminderDate_) {
        reminderDate_->setEnabled(enabled);
        reminderTime_->setEnabled(enabled);
        reminderEndDate_->setEnabled(enabled);
        reminderEndTime_->setEnabled(enabled);
        reminderAllDayCheckbox_->setEnabled(enabled);
        recurringRuleCombo_->setEnabled(enabled);
    }

    // Type-specific fields - Jira Ticket
    if (jiraStartDate_) {
        jiraKeyEdit_->setEnabled(enabled);
        jiraSummaryEdit_->setEnabled(enabled);
        jiraTypeCombo_->setEnabled(enabled);
        jiraStatusCombo_->setEnabled(enabled);
        jiraStartDate_->setEnabled(enabled);
        jiraStartTime_->setEnabled(enabled);
        jiraDueDate_->setEnabled(enabled);
        jiraDueTime_->setEnabled(enabled);
        jiraAllDayCheckbox_->setEnabled(enabled);
    }

    // Attachment widget
    if (attachmentWidget_) {
        attachmentWidget_->setEnabled(enabled);
    }

    // Keep unlock checkboxes enabled
    lockedCheckbox_->setEnabled(true);
    fixedCheckbox_->setEnabled(!locked);

    // Apply and Delete buttons
    applyButton_->setEnabled(enabled);
    deleteButton_->setEnabled(enabled);
}
