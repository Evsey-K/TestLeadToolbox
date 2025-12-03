// EditEventDialog.cpp


#include "EditEventDialog.h"
#include "ui_EditEventDialog.h"
#include <QMessageBox>


EditEventDialog::EditEventDialog(const QString& eventId,
                                 TimelineModel* model,
                                 const QDate& versionStart,
                                 const QDate& versionEnd,
                                 QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditEventDialog)
    , eventId_(eventId)
    , model_(model)
    , versionStart_(versionStart)
    , versionEnd_(versionEnd)
{
    ui->setupUi(this);

    // Populate type combo box
    ui->typeCombo->addItem("Meeting", static_cast<int>(TimelineEventType_Meeting));
    ui->typeCombo->addItem("Action", static_cast<int>(TimelineEventType_Action));
    ui->typeCombo->addItem("Test Event", static_cast<int>(TimelineEventType_TestEvent));
    ui->typeCombo->addItem("Due Date", static_cast<int>(TimelineEventType_DueDate));
    ui->typeCombo->addItem("Reminder", static_cast<int>(TimelineEventType_Reminder));

    // Configure date pickers
    ui->startDateEdit->setMinimumDate(versionStart_);
    ui->startDateEdit->setMaximumDate(versionEnd_);
    ui->endDateEdit->setMinimumDate(versionStart_);
    ui->endDateEdit->setMaximumDate(versionEnd_);

    // Load existing event data
    const TimelineEvent* event = model_->getEvent(eventId_);
    if (event)
    {
        populateFromEvent(*event);
    }
    else
    {
        QMessageBox::critical(this, "Error", "Event not found!");
        reject();
        return;
    }

    // Connect signals
    connect(ui->saveButton, &QPushButton::clicked, this, &EditEventDialog::validateAndAccept);
    connect(ui->deleteButton, &QPushButton::clicked, this, &EditEventDialog::onDeleteClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->startDateEdit, &QDateEdit::dateChanged, this, &EditEventDialog::onStartDateChanged);
}


EditEventDialog::~EditEventDialog()
{
    delete ui;
}


void EditEventDialog::populateFromEvent(const TimelineEvent& event)
{
    // Set type combo
    for (int i = 0; i < ui->typeCombo->count(); ++i)
    {
        if (ui->typeCombo->itemData(i).toInt() == static_cast<int>(event.type))
        {
            ui->typeCombo->setCurrentIndex(i);
            break;
        }
    }

    // Set other fields
    ui->titleEdit->setText(event.title);
    ui->startDateEdit->setDate(event.startDate);
    ui->endDateEdit->setDate(event.endDate);
    ui->prioritySpinner->setValue(event.priority);
    ui->descriptionEdit->setPlainText(event.description);
}


void EditEventDialog::validateAndAccept()
{
    // Validate title
    if (ui->titleEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Please enter a title for the event.");
        ui->titleEdit->setFocus();
        return;
    }

    // Validate date range
    if (ui->startDateEdit->date() > ui->endDateEdit->date())
    {
        QMessageBox::warning(this, "Validation Error",
                             "Start date must be before or equal to end date.");
        ui->startDateEdit->setFocus();
        return;
    }

    accept();
}


void EditEventDialog::onDeleteClicked()
{
    // Get event for confirmation message
    const TimelineEvent* event = model_->getEvent(eventId_);
    if (!event)
    {
        QMessageBox::critical(this, "Error", "Event not found!");
        reject();
        return;
    }

    // Show confirmation dialog
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

    if (result == QMessageBox::Yes)
    {
        done(DeleteRequested);
    }
}


void EditEventDialog::onStartDateChanged(const QDate& date)
{
    ui->endDateEdit->setMinimumDate(date);
    if (ui->endDateEdit->date() < date)
    {
        ui->endDateEdit->setDate(date);
    }
}


TimelineEvent EditEventDialog::getEvent() const
{
    TimelineEvent event;

    int typeValue = ui->typeCombo->currentData().toInt();
    event.type = static_cast<TimelineEventType>(typeValue);
    event.title = ui->titleEdit->text().trimmed();
    event.startDate = ui->startDateEdit->date();
    event.endDate = ui->endDateEdit->date();
    event.description = ui->descriptionEdit->toPlainText().trimmed();
    event.priority = ui->prioritySpinner->value();
    event.color = TimelineModel::colorForType(event.type);
    event.id = eventId_;

    return event;
}
