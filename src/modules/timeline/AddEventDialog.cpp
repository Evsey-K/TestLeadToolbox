// AddEventDialog.cpp


#include "AddEventDialog.h"
#include "ui_AddEventDialog.h"
#include "TimelineModel.h"
#include <QMessageBox>


AddEventDialog::AddEventDialog(const QDate& versionStart,
                               const QDate& versionEnd,
                               QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AddEventDialog)
    , versionStart_(versionStart)
    , versionEnd_(versionEnd)
{
    // Initialize the UI from the .ui file - creates all widgets and layouts
    ui->setupUi(this);

    // Populate the type combo box with all event types
    // Must be done in code because we need to attach enum values as UserRole data
    ui->typeCombo->addItem("Meeting", static_cast<int>(TimelineObjectType::Meeting));
    ui->typeCombo->addItem("Action", static_cast<int>(TimelineObjectType::Action));
    ui->typeCombo->addItem("Test Event", static_cast<int>(TimelineObjectType::TestEvent));
    ui->typeCombo->addItem("Deadline", static_cast<int>(TimelineObjectType::DueDate));
    ui->typeCombo->addItem("Reminder", static_cast<int>(TimelineObjectType::Reminder));

    // Configure start date picker with validation boundaries
    // Default to today's date for convenience
    // NOTE: calendarPopup and displayFormat should be set in .ui file properties
    // Only set here if not already configured in Qt Designer
    ui->startDateEdit->setDate(QDate::currentDate());
    ui->startDateEdit->setMinimumDate(versionStart_);
    ui->startDateEdit->setMaximumDate(versionEnd_);

    // Configure end date picker with same boundaries
    // Initially set to same date as start date
    // NOTE: calendarPopup and displayFormat should be set in .ui file properties
    // Only set here if not already configured in Qt Designer
    ui->endDateEdit->setDate(QDate::currentDate());
    ui->endDateEdit->setMinimumDate(versionStart_);
    ui->endDateEdit->setMaximumDate(versionEnd_);

    // Connect button signals to dialog slots
    // Cancel button triggers standard QDialog rejection
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Add button triggers custom validation before acceptance
    connect(ui->addButton, &QPushButton::clicked, this, &AddEventDialog::validateAndAccept);

    // Connect start date changes to handler that maintains date consistency
    connect(ui->startDateEdit, &QDateEdit::dateChanged, this, &AddEventDialog::onStartDateChanged);
}


AddEventDialog::~AddEventDialog()
{
    // Clean up the UI pointer allocated in the constructor
    // Qt's parent-child hierarchy handles widget cleanup, but ui itself must be deleted
    delete ui;
}


void AddEventDialog::onStartDateChanged(const QDate& date)
{
    // Update end date minimum to match the new start date
    // This prevents end date from being earlier than start date
    ui->endDateEdit->setMinimumDate(date);

    // If the current end date is now invalid (earlier than new start date),
    // automatically adjust it to match the start date
    if(ui->endDateEdit->date() < date)
    {
        ui->endDateEdit->setDate(date);
    }
}


void AddEventDialog::validateAndAccept()
{
    // Validate title field - must not be empty or whitespace-only
    if(ui->titleEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Please enter a title for the event.");
        ui->titleEdit->setFocus();

        return;
    }

    // Validate date range (redundant check, but good practice)
    if(ui->startDateEdit->date() > ui->endDateEdit->date())
    {
        QMessageBox::warning(this, "Validation Error", "Start date must be before or equal to end date.");
        ui->startDateEdit->setFocus();

        return;
    }

    // All validation passed - accept the dialog with QDialog::Accepted status
    accept();
}


TimelineObject AddEventDialog::getEvent() const
{
    TimelineObject event;

    // Extract type from combo box user data (stored as integer enum value)
    int typeValue = ui->typeCombo->currentData().toInt();
    event.type = static_cast<TimelineObjectType>(typeValue);

    // Extract and trim title text to remove leading/trailing whitespace
    event.title = ui->titleEdit->text().trimmed();

    // Extract date range from date picker widgets
    event.startDate = ui->startDateEdit->date();
    event.endDate = ui->endDateEdit->date();

    // Extract and trim description - may be empty if user left it blank
    event.description = ui->descriptionEdit->toPlainText().trimmed();

    // Extract priority value from spinner (0-5 range)
    event.priority = ui->prioritySpinner->value();

    // Assign color based on event type using model's color mapping
    // This ensures consistent color coding across the application
    event.color = TimelineModel::colorForType(event.type);

    // Leave ID empty - the model will generate a unique ID when adding the event
    event.id = QString();

    return event;
}










