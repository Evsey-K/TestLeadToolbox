// ScrollToDateDialog.cpp

#include "ScrollToDateDialog.h"
#include "ui_ScrollToDateDialog.h"
#include <QSettings>

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

    // Load saved preferences from previous session
    loadSettings();

    // Setup signal/slot connections
    setupConnections();
}

ScrollToDateDialog::~ScrollToDateDialog()
{
    // Save settings when dialog closes
    saveSettings();
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

void ScrollToDateDialog::loadSettings()
{
    QSettings settings("TestLeadToolbox", "Timeline");

    // Load animation preference (default: true)
    bool animate = settings.value("ScrollToDate/Animate", true).toBool();
    ui->animateCheckbox->setChecked(animate);

    // Load highlight preference (default: false)
    bool highlight = settings.value("ScrollToDate/Highlight", false).toBool();
    ui->highlightCheckbox->setChecked(highlight);

    // Load highlight range (default: 7 days)
    int rangeDays = settings.value("ScrollToDate/HighlightRange", 7).toInt();
    ui->highlightRangeSpinner->setValue(rangeDays);

    // This is necessary because the signal connection hasn't been set up yet
    ui->highlightRangeSpinner->setEnabled(highlight);
}

void ScrollToDateDialog::saveSettings()
{
    QSettings settings("TestLeadToolbox", "Timeline");

    settings.setValue("ScrollToDate/Animate", ui->animateCheckbox->isChecked());
    settings.setValue("ScrollToDate/Highlight", ui->highlightCheckbox->isChecked());
    settings.setValue("ScrollToDate/HighlightRange", ui->highlightRangeSpinner->value());
}
