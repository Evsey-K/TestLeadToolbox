// ScrollToDateDialog.cpp

#include "ScrollToDateDialog.h"
#include "ui_ScrollToDateDialog.h"

ScrollToDateDialog::ScrollToDateDialog(const QDate& currentDate,
                                       const QDate& minDate,
                                       const QDate& maxDate,
                                       QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ScrollToDateDialog)
    , currentDate_(currentDate)
    , minDate_(minDate)
    , maxDate_(maxDate)
{
    // Initialize UI from Qt Designer file
    ui->setupUi(this);

    // Configure date edit
    ui->dateEdit->setDate(currentDate_);
    ui->dateEdit->setMinimumDate(minDate_);
    ui->dateEdit->setMaximumDate(maxDate_);

    // Setup signal/slot connections
    setupConnections();
}

ScrollToDateDialog::~ScrollToDateDialog()
{
    delete ui;
}

void ScrollToDateDialog::setupConnections()
{
    // Button connections
    connect(ui->goButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Highlight checkbox connection
    connect(ui->highlightCheckbox, &QCheckBox::toggled,
            this, &ScrollToDateDialog::onHighlightToggled);
}

QDate ScrollToDateDialog::targetDate() const
{
    return ui->dateEdit->date();
}

bool ScrollToDateDialog::shouldAnimate() const
{
    return ui->animateCheckbox->isChecked();
}

bool ScrollToDateDialog::shouldHighlight() const
{
    return ui->highlightCheckbox->isChecked();
}

int ScrollToDateDialog::highlightRangeDays() const
{
    return ui->highlightRangeSpinner->value();
}

void ScrollToDateDialog::onHighlightToggled(bool checked)
{
    ui->highlightRangeSpinner->setEnabled(checked);
}
