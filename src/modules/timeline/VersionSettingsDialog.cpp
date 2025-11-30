// VersionSettingsDialog.cpp
// Qt Designer Integration Version


#include "VersionSettingsDialog.h"
#include "ui_VersionSettingsDialog.h"  // Auto-generated from .ui file
#include <QMessageBox>


VersionSettingsDialog::VersionSettingsDialog(const QDate& currentStart,
                                             const QDate& currentEnd,
                                             QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::VersionSettingsDialog)  // Create UI from Qt Designer file
    , currentStart_(currentStart)
    , currentEnd_(currentEnd)
{
    // Initialize the UI - this creates all widgets defined in Qt Designer
    ui->setupUi(this);

    // Set current dates in the date pickers
    ui->startDateEdit->setDate(currentStart_);
    ui->endDateEdit->setDate(currentEnd_);

    // Set minimum end date to match start date (prevents invalid ranges)
    ui->endDateEdit->setMinimumDate(currentStart_);

    // Optional: Set reasonable date range limits
    QDate minDate = QDate::currentDate().addYears(-2);
    QDate maxDate = QDate::currentDate().addYears(5);

    ui->startDateEdit->setMinimumDate(minDate);
    ui->startDateEdit->setMaximumDate(maxDate);
    ui->endDateEdit->setMaximumDate(maxDate);

    // Setup all signal/slot connections
    setupConnections();

    // Set focus to version name field for convenience
    ui->versionNameEdit->setFocus();
}


VersionSettingsDialog::~VersionSettingsDialog()
{
    // Clean up the UI pointer
    // Qt's parent-child hierarchy will delete all child widgets,
    // but we need to explicitly delete the ui wrapper
    delete ui;
}


void VersionSettingsDialog::setupConnections()
{
    // Connect OK button to validation
    connect(ui->okButton, &QPushButton::clicked,
            this, &VersionSettingsDialog::validateAndAccept);

    // Connect Cancel button to standard rejection
    connect(ui->cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);

    // Connect start date changes to handler
    connect(ui->startDateEdit, &QDateEdit::dateChanged,
            this, &VersionSettingsDialog::onStartDateChanged);
}


QDate VersionSettingsDialog::startDate() const
{
    return ui->startDateEdit->date();
}


QDate VersionSettingsDialog::endDate() const
{
    return ui->endDateEdit->date();
}


QString VersionSettingsDialog::versionName() const
{
    // Trim whitespace from version name
    return ui->versionNameEdit->text().trimmed();
}


void VersionSettingsDialog::validateAndAccept()
{
    QDate start = ui->startDateEdit->date();
    QDate end = ui->endDateEdit->date();

    // Validation 1: Ensure start <= end
    if (start > end)
    {
        QMessageBox::warning(
            this,
            "Invalid Date Range",
            "Start date must be before or equal to end date.\n\n"
            "Please adjust the dates and try again."
            );
        ui->startDateEdit->setFocus();
        return;
    }

    // Validation 2: Check for very short timelines (warn, but allow)
    int daysInRange = start.daysTo(end);
    if (daysInRange < 7)
    {
        auto result = QMessageBox::question(
            this,
            "Short Timeline",
            QString("The timeline is only %1 day(s) long.\n\n"
                    "This may be too short for practical use.\n\n"
                    "Do you want to continue anyway?")
                .arg(daysInRange),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No  // Default to No
            );

        if (result != QMessageBox::Yes)
        {
            ui->endDateEdit->setFocus();
            return;
        }
    }

    // Validation 3: Optional - warn if timeline is extremely long (> 2 years)
    if (daysInRange > 730)  // 2 years
    {
        auto result = QMessageBox::question(
            this,
            "Long Timeline",
            QString("The timeline is %1 days long (over 2 years).\n\n"
                    "Very long timelines may impact performance.\n\n"
                    "Do you want to continue?")
                .arg(daysInRange),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes  // Default to Yes for long timelines
            );

        if (result != QMessageBox::Yes)
        {
            return;
        }
    }

    // All validations passed - accept the dialog
    accept();
}


void VersionSettingsDialog::onStartDateChanged(const QDate& date)
{
    // Update end date minimum to ensure it's never before start date
    ui->endDateEdit->setMinimumDate(date);

    // If current end date is now invalid (before new start date),
    // automatically adjust it to match the start date
    if (ui->endDateEdit->date() < date)
    {
        ui->endDateEdit->setDate(date);
    }
}
