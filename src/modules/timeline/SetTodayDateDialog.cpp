// SetTodayDateDialog.cpp


#include "SetTodayDateDialog.h"
#include "ui_SetTodayDateDialog.h"
#include <QDate>


SetTodayDateDialog::SetTodayDateDialog(const QDate& currentDate,
                                       const QDate& minDate,
                                       const QDate& maxDate,
                                       QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SetTodayDateDialog)
    , minDate_(minDate)
    , maxDate_(maxDate)
{
    ui->setupUi(this);

    configureDateEdit();

    // Set the current date
    ui->dateEdit->setDate(currentDate);

    setupConnections();
}


SetTodayDateDialog::~SetTodayDateDialog()
{
    delete ui;
}


void SetTodayDateDialog::configureDateEdit()
{
    // Set date range constraints
    ui->dateEdit->setMinimumDate(minDate_);
    ui->dateEdit->setMaximumDate(maxDate_);

    // Already set in .ui file, but ensure they're correct:
    ui->dateEdit->setDisplayFormat("yyyy-MM-dd");
    ui->dateEdit->setCalendarPopup(true);
}


void SetTodayDateDialog::setupConnections()
{
    // Connect buttons
    connect(ui->okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Connect quick-set button
    connect(ui->setToCurrentButton, &QPushButton::clicked,
            this, &SetTodayDateDialog::onSetToCurrentClicked);
}


void SetTodayDateDialog::onSetToCurrentClicked()
{
    ui->dateEdit->setDate(QDate::currentDate());
}


QDate SetTodayDateDialog::selectedDate() const
{
    return ui->dateEdit->date();
}
