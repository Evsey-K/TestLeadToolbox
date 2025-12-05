// SetLookaheadRangeDialog.cpp


#include "SetLookaheadRangeDialog.h"
#include "ui_SetLookaheadRangeDialog.h"
#include <QDate>


SetLookaheadRangeDialog::SetLookaheadRangeDialog(int currentDays, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SetLookaheadRangeDialog)
{
    ui->setupUi(this);

    // Set current value
    ui->daysSpinner->setValue(currentDays);

    setupConnections();

    // Trigger initial example update
    onDaysValueChanged(currentDays);
}


SetLookaheadRangeDialog::~SetLookaheadRangeDialog()
{
    delete ui;
}


void SetLookaheadRangeDialog::setupConnections()
{
    // Connect buttons
    connect(ui->okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Connect spinner value change to update example
    connect(ui->daysSpinner, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SetLookaheadRangeDialog::onDaysValueChanged);
}


void SetLookaheadRangeDialog::onDaysValueChanged(int days)
{
    // Calculate date range for live preview
    QDate today = QDate::currentDate();
    QDate endDate = today.addDays(days);

    // Update example label with actual dates
    QString exampleText = QString(
                              "📅 <b>Example:</b> With %1 days, you'll see events from <b>%2</b> to <b>%3</b>"
                              ).arg(days)
                              .arg(today.toString("MMM dd, yyyy"))
                              .arg(endDate.toString("MMM dd, yyyy"));

    ui->exampleLabel->setText(exampleText);
}


int SetLookaheadRangeDialog::selectedDays() const
{
    return ui->daysSpinner->value();
}
