// src/modules/linkbudget/LinkBudgetModule.cpp

#include "LinkBudgetModule.h"
#include "LinkBudgetModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QScrollArea>
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QtMath>
#include <QDebug>

LinkBudgetModule::LinkBudgetModule(QWidget* parent)
    : QWidget(parent)
    , model_(nullptr)
    , hasUnsavedChanges_(false)
{
    model_ = new LinkBudgetModel(this);
    setupUI();
    connectSignals();
}

LinkBudgetModule::~LinkBudgetModule()
{
}

// IModule interface implementation
QWidget* LinkBudgetModule::createWidget(QWidget* parent)
{
    if (parent && parent != parentWidget()) {
        setParent(parent);
    }
    return this;
}

QString LinkBudgetModule::name() const
{
    return "Link Budget Calculator";
}

QString LinkBudgetModule::description() const
{
    return "Calculate RF link budgets for satellite, terrestrial, and wireless communication systems";
}

QIcon LinkBudgetModule::icon() const
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw antenna on left
    painter.setPen(QPen(QColor(40, 167, 69), 2));
    painter.drawLine(6, 26, 6, 16);
    painter.drawArc(2, 12, 8, 8, 0, 180 * 16);

    // Draw signal waves
    painter.setPen(QPen(QColor(0, 123, 255), 1));
    for (int i = 0; i < 3; ++i) {
        painter.drawArc(6 - i*2, 16 - i*2, i*4, i*4, 0, 180 * 16);
    }

    // Draw antenna on right
    painter.setPen(QPen(QColor(40, 167, 69), 2));
    painter.drawLine(26, 26, 26, 16);
    painter.drawArc(22, 12, 8, 8, 0, 180 * 16);

    return QIcon(pixmap);
}

QString LinkBudgetModule::moduleId() const
{
    return "link_budget";
}

void LinkBudgetModule::onActivate()
{
    qDebug() << "LinkBudgetModule: Activated";
}

void LinkBudgetModule::onDeactivate()
{
    qDebug() << "LinkBudgetModule: Deactivated";
}

bool LinkBudgetModule::hasUnsavedChanges() const
{
    return hasUnsavedChanges_;
}

bool LinkBudgetModule::save()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Link Budget",
        QString(),
        "Link Budget Files (*.lnk);;JSON Files (*.json);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        return false;
    }

    // Save logic would go here
    QJsonObject json = model_->toJson();
    QJsonDocument doc(json);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Save Failed", "Could not open file for writing");
        return false;
    }

    file.write(doc.toJson());
    file.close();

    hasUnsavedChanges_ = false;
    return true;
}

void LinkBudgetModule::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Title
    auto* titleLabel = new QLabel("RF Link Budget Calculator", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Create scrollable area for parameters
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget();
    auto* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(10);

    // Create parameter sections
    createTransmitterSection();
    createPathSection();
    createReceiverSection();
    createResultsSection();

    scrollLayout->addWidget(transmitterGroup_);
    scrollLayout->addWidget(pathGroup_);
    scrollLayout->addWidget(receiverGroup_);
    scrollLayout->addWidget(resultsGroup_);
    scrollLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

    // Control buttons at bottom
    createControlButtons();
    mainLayout->addLayout(dynamic_cast<QHBoxLayout*>(children().last()));
}

void LinkBudgetModule::createTransmitterSection()
{
    transmitterGroup_ = new QGroupBox("Transmitter Parameters", this);
    auto* layout = new QFormLayout(transmitterGroup_);

    // Transmit Power
    txPowerSpin_ = new QDoubleSpinBox(this);
    txPowerSpin_->setRange(-100.0, 100.0);
    txPowerSpin_->setValue(10.0);
    txPowerSpin_->setSuffix(" dBW");
    txPowerSpin_->setDecimals(2);
    txPowerSpin_->setToolTip("Transmitter output power in dBW");
    layout->addRow("Tx Power:", txPowerSpin_);

    // TX Line Loss
    txLineLossSpin_ = new QDoubleSpinBox(this);
    txLineLossSpin_->setRange(0.0, 50.0);
    txLineLossSpin_->setValue(0.5);
    txLineLossSpin_->setSuffix(" dB");
    txLineLossSpin_->setDecimals(2);
    txLineLossSpin_->setToolTip("Loss between transmitter and antenna");
    layout->addRow("Tx Line Loss:", txLineLossSpin_);

    // TX Antenna Gain
    txAntennaGainSpin_ = new QDoubleSpinBox(this);
    txAntennaGainSpin_->setRange(-10.0, 100.0);
    txAntennaGainSpin_->setValue(20.0);
    txAntennaGainSpin_->setSuffix(" dBi");
    txAntennaGainSpin_->setDecimals(2);
    txAntennaGainSpin_->setToolTip("Transmit antenna gain");
    layout->addRow("Tx Antenna Gain:", txAntennaGainSpin_);

    // Polarization
    txPolarizationCombo_ = new QComboBox(this);
    txPolarizationCombo_->addItems({"Linear", "Circular (RHCP)", "Circular (LHCP)"});
    txPolarizationCombo_->setToolTip("Transmit antenna polarization");
    layout->addRow("Polarization:", txPolarizationCombo_);
}

void LinkBudgetModule::createPathSection()
{
    pathGroup_ = new QGroupBox("Propagation Path Parameters", this);
    auto* layout = new QFormLayout(pathGroup_);

    // Frequency
    auto* freqLayout = new QHBoxLayout();
    frequencySpin_ = new QDoubleSpinBox(this);
    frequencySpin_->setRange(0.001, 1000.0);
    frequencySpin_->setValue(2.4);
    frequencySpin_->setDecimals(3);
    frequencySpin_->setToolTip("Operating frequency");

    frequencyUnitCombo_ = new QComboBox(this);
    frequencyUnitCombo_->addItems({"MHz", "GHz"});
    frequencyUnitCombo_->setCurrentText("GHz");

    freqLayout->addWidget(frequencySpin_, 1);
    freqLayout->addWidget(frequencyUnitCombo_);
    layout->addRow("Frequency:", freqLayout);

    // Distance
    auto* distLayout = new QHBoxLayout();
    distanceSpin_ = new QDoubleSpinBox(this);
    distanceSpin_->setRange(0.001, 1000000.0);
    distanceSpin_->setValue(100.0);
    distanceSpin_->setDecimals(3);
    distanceSpin_->setToolTip("Path distance");

    distanceUnitCombo_ = new QComboBox(this);
    distanceUnitCombo_->addItems({"m", "km", "mi", "nmi"});
    distanceUnitCombo_->setCurrentText("km");

    distLayout->addWidget(distanceSpin_, 1);
    distLayout->addWidget(distanceUnitCombo_);
    layout->addRow("Distance:", distLayout);

    // Atmospheric Loss
    atmosphericLossSpin_ = new QDoubleSpinBox(this);
    atmosphericLossSpin_->setRange(0.0, 50.0);
    atmosphericLossSpin_->setValue(0.2);
    atmosphericLossSpin_->setSuffix(" dB");
    atmosphericLossSpin_->setDecimals(2);
    atmosphericLossSpin_->setToolTip("Atmospheric absorption loss");
    layout->addRow("Atmospheric Loss:", atmosphericLossSpin_);

    // Rain Loss
    rainLossSpin_ = new QDoubleSpinBox(this);
    rainLossSpin_->setRange(0.0, 50.0);
    rainLossSpin_->setValue(0.0);
    rainLossSpin_->setSuffix(" dB");
    rainLossSpin_->setDecimals(2);
    rainLossSpin_->setToolTip("Rain attenuation");
    layout->addRow("Rain Loss:", rainLossSpin_);

    // Polarization Mismatch Loss
    polarizationLossSpin_ = new QDoubleSpinBox(this);
    polarizationLossSpin_->setRange(0.0, 30.0);
    polarizationLossSpin_->setValue(0.0);
    polarizationLossSpin_->setSuffix(" dB");
    polarizationLossSpin_->setDecimals(2);
    polarizationLossSpin_->setToolTip("Loss due to polarization mismatch");
    layout->addRow("Polarization Loss:", polarizationLossSpin_);

    // Miscellaneous Losses
    miscLossSpin_ = new QDoubleSpinBox(this);
    miscLossSpin_->setRange(0.0, 50.0);
    miscLossSpin_->setValue(0.0);
    miscLossSpin_->setSuffix(" dB");
    miscLossSpin_->setDecimals(2);
    miscLossSpin_->setToolTip("Additional path losses (pointing, fading margin, etc.)");
    layout->addRow("Misc. Losses:", miscLossSpin_);
}

void LinkBudgetModule::createReceiverSection()
{
    receiverGroup_ = new QGroupBox("Receiver Parameters", this);
    auto* layout = new QFormLayout(receiverGroup_);

    // RX Antenna Gain
    rxAntennaGainSpin_ = new QDoubleSpinBox(this);
    rxAntennaGainSpin_->setRange(-10.0, 100.0);
    rxAntennaGainSpin_->setValue(15.0);
    rxAntennaGainSpin_->setSuffix(" dBi");
    rxAntennaGainSpin_->setDecimals(2);
    rxAntennaGainSpin_->setToolTip("Receive antenna gain");
    layout->addRow("Rx Antenna Gain:", rxAntennaGainSpin_);

    // RX Line Loss
    rxLineLossSpin_ = new QDoubleSpinBox(this);
    rxLineLossSpin_->setRange(0.0, 50.0);
    rxLineLossSpin_->setValue(0.5);
    rxLineLossSpin_->setSuffix(" dB");
    rxLineLossSpin_->setDecimals(2);
    rxLineLossSpin_->setToolTip("Loss between antenna and receiver");
    layout->addRow("Rx Line Loss:", rxLineLossSpin_);

    // System Noise Temperature
    systemNoiseTempSpin_ = new QDoubleSpinBox(this);
    systemNoiseTempSpin_->setRange(1.0, 10000.0);
    systemNoiseTempSpin_->setValue(290.0);
    systemNoiseTempSpin_->setSuffix(" K");
    systemNoiseTempSpin_->setDecimals(1);
    systemNoiseTempSpin_->setToolTip("System noise temperature");
    layout->addRow("System Noise Temp:", systemNoiseTempSpin_);

    // Noise Figure (alternative to noise temp)
    noiseFigureSpin_ = new QDoubleSpinBox(this);
    noiseFigureSpin_->setRange(0.0, 30.0);
    noiseFigureSpin_->setValue(3.0);
    noiseFigureSpin_->setSuffix(" dB");
    noiseFigureSpin_->setDecimals(2);
    noiseFigureSpin_->setToolTip("Receiver noise figure (alternative to noise temp)");
    layout->addRow("Noise Figure:", noiseFigureSpin_);

    // Bandwidth
    auto* bwLayout = new QHBoxLayout();
    bandwidthSpin_ = new QDoubleSpinBox(this);
    bandwidthSpin_->setRange(0.001, 1000000.0);
    bandwidthSpin_->setValue(1.0);
    bandwidthSpin_->setDecimals(3);
    bandwidthSpin_->setToolTip("Signal bandwidth");

    bandwidthUnitCombo_ = new QComboBox(this);
    bandwidthUnitCombo_->addItems({"Hz", "kHz", "MHz", "GHz"});
    bandwidthUnitCombo_->setCurrentText("MHz");

    bwLayout->addWidget(bandwidthSpin_, 1);
    bwLayout->addWidget(bandwidthUnitCombo_);
    layout->addRow("Bandwidth:", bwLayout);

    // Required SNR
    requiredSNRSpin_ = new QDoubleSpinBox(this);
    requiredSNRSpin_->setRange(-20.0, 50.0);
    requiredSNRSpin_->setValue(10.0);
    requiredSNRSpin_->setSuffix(" dB");
    requiredSNRSpin_->setDecimals(2);
    requiredSNRSpin_->setToolTip("Required signal-to-noise ratio for desired performance");
    layout->addRow("Required SNR:", requiredSNRSpin_);

    // Implementation Loss
    implementationLossSpin_ = new QDoubleSpinBox(this);
    implementationLossSpin_->setRange(0.0, 10.0);
    implementationLossSpin_->setValue(1.0);
    implementationLossSpin_->setSuffix(" dB");
    implementationLossSpin_->setDecimals(2);
    implementationLossSpin_->setToolTip("Implementation losses (ADC, filters, etc.)");
    layout->addRow("Implementation Loss:", implementationLossSpin_);
}

void LinkBudgetModule::createResultsSection()
{
    resultsGroup_ = new QGroupBox("Link Budget Results", this);
    resultsGroup_->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* layout = new QVBoxLayout(resultsGroup_);

    // Key results in large labels
    auto* keyResultsLayout = new QFormLayout();

    eirpLabel_ = new QLabel("-- dBW", this);
    eirpLabel_->setStyleSheet("font-weight: normal; font-size: 11pt;");
    keyResultsLayout->addRow("EIRP:", eirpLabel_);

    freeSpaceLossLabel_ = new QLabel("-- dB", this);
    freeSpaceLossLabel_->setStyleSheet("font-weight: normal; font-size: 11pt;");
    keyResultsLayout->addRow("Free Space Loss:", freeSpaceLossLabel_);

    totalPathLossLabel_ = new QLabel("-- dB", this);
    totalPathLossLabel_->setStyleSheet("font-weight: normal; font-size: 11pt;");
    keyResultsLayout->addRow("Total Path Loss:", totalPathLossLabel_);

    rxPowerLabel_ = new QLabel("-- dBW", this);
    rxPowerLabel_->setStyleSheet("font-weight: normal; font-size: 11pt;");
    keyResultsLayout->addRow("Received Power:", rxPowerLabel_);

    noisePowerLabel_ = new QLabel("-- dBW", this);
    noisePowerLabel_->setStyleSheet("font-weight: normal; font-size: 11pt;");
    keyResultsLayout->addRow("Noise Power:", noisePowerLabel_);

    cnrLabel_ = new QLabel("-- dB", this);
    cnrLabel_->setStyleSheet("font-weight: normal; font-size: 11pt;");
    keyResultsLayout->addRow("C/N Ratio:", cnrLabel_);

    gtLabel_ = new QLabel("-- dB/K", this);
    gtLabel_->setStyleSheet("font-weight: normal; font-size: 11pt;");
    keyResultsLayout->addRow("G/T:", gtLabel_);

    linkMarginLabel_ = new QLabel("-- dB", this);
    QFont marginFont = linkMarginLabel_->font();
    marginFont.setPointSize(12);
    marginFont.setBold(true);
    linkMarginLabel_->setFont(marginFont);
    keyResultsLayout->addRow("Link Margin:", linkMarginLabel_);

    layout->addLayout(keyResultsLayout);

    // Detailed budget table
    budgetTable_ = new QTableWidget(this);
    budgetTable_->setColumnCount(2);
    budgetTable_->setHorizontalHeaderLabels({"Parameter", "Value"});
    budgetTable_->horizontalHeader()->setStretchLastSection(true);
    budgetTable_->setMaximumHeight(200);
    budgetTable_->setAlternatingRowColors(true);
    layout->addWidget(budgetTable_);
}

void LinkBudgetModule::createControlButtons()
{
    auto* buttonLayout = new QHBoxLayout();

    calculateButton_ = new QPushButton("Calculate", this);
    calculateButton_->setDefault(true);
    calculateButton_->setToolTip("Perform link budget calculation");
    connect(calculateButton_, &QPushButton::clicked, this, &LinkBudgetModule::calculate);

    clearButton_ = new QPushButton("Clear", this);
    clearButton_->setToolTip("Reset all parameters to defaults");
    connect(clearButton_, &QPushButton::clicked, this, &LinkBudgetModule::clearAll);

    loadPresetButton_ = new QPushButton("Load Preset", this);
    loadPresetButton_->setToolTip("Load saved link budget preset");
    connect(loadPresetButton_, &QPushButton::clicked, this, &LinkBudgetModule::loadPreset);

    savePresetButton_ = new QPushButton("Save Preset", this);
    savePresetButton_->setToolTip("Save current configuration as preset");
    connect(savePresetButton_, &QPushButton::clicked, this, &LinkBudgetModule::savePreset);

    exportButton_ = new QPushButton("Export Report", this);
    exportButton_->setToolTip("Export detailed link budget report");
    connect(exportButton_, &QPushButton::clicked, this, &LinkBudgetModule::exportResults);

    buttonLayout->addWidget(calculateButton_);
    buttonLayout->addWidget(clearButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(loadPresetButton_);
    buttonLayout->addWidget(savePresetButton_);
    buttonLayout->addWidget(exportButton_);

    // Add to main layout (will be picked up by setupUI)
    auto* mainLayout = dynamic_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->addLayout(buttonLayout);
    }
}

void LinkBudgetModule::connectSignals()
{
    // Connect all spinboxes and combos to trigger calculation auto-update
    connect(txPowerSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &LinkBudgetModule::onParameterChanged);
    connect(txLineLossSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &LinkBudgetModule::onParameterChanged);
    connect(txAntennaGainSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &LinkBudgetModule::onParameterChanged);

    connect(frequencySpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &LinkBudgetModule::onParameterChanged);
    connect(frequencyUnitCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LinkBudgetModule::onParameterChanged);
    connect(distanceSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &LinkBudgetModule::onParameterChanged);
    connect(distanceUnitCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LinkBudgetModule::onParameterChanged);

    connect(rxAntennaGainSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &LinkBudgetModule::onParameterChanged);
    connect(rxLineLossSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &LinkBudgetModule::onParameterChanged);
    connect(bandwidthSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &LinkBudgetModule::onParameterChanged);
    connect(bandwidthUnitCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LinkBudgetModule::onParameterChanged);
}

void LinkBudgetModule::onParameterChanged()
{
    hasUnsavedChanges_ = true;
    // Auto-calculate on parameter change
    calculate();
}

void LinkBudgetModule::calculate()
{
    updateResults();
}

void LinkBudgetModule::clearAll()
{
    // Reset to default values
    txPowerSpin_->setValue(10.0);
    txLineLossSpin_->setValue(0.5);
    txAntennaGainSpin_->setValue(20.0);

    frequencySpin_->setValue(2.4);
    frequencyUnitCombo_->setCurrentText("GHz");
    distanceSpin_->setValue(100.0);
    distanceUnitCombo_->setCurrentText("km");

    atmosphericLossSpin_->setValue(0.2);
    rainLossSpin_->setValue(0.0);
    polarizationLossSpin_->setValue(0.0);
    miscLossSpin_->setValue(0.0);

    rxAntennaGainSpin_->setValue(15.0);
    rxLineLossSpin_->setValue(0.5);
    systemNoiseTempSpin_->setValue(290.0);
    noiseFigureSpin_->setValue(3.0);
    bandwidthSpin_->setValue(1.0);
    bandwidthUnitCombo_->setCurrentText("MHz");
    requiredSNRSpin_->setValue(10.0);
    implementationLossSpin_->setValue(1.0);

    hasUnsavedChanges_ = false;
}

void LinkBudgetModule::loadPreset()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load Link Budget Preset",
        QString(),
        "Link Budget Files (*.lnk);;JSON Files (*.json);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Load Failed", "Could not open file");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        QMessageBox::warning(this, "Load Failed", "Invalid file format");
        return;
    }

    model_->fromJson(doc.object());
    hasUnsavedChanges_ = false;
    calculate();
}

void LinkBudgetModule::savePreset()
{
    save();
}

void LinkBudgetModule::exportResults()
{
    QMessageBox::information(this, "Export", "Export feature coming soon!");
}

void LinkBudgetModule::updateResults()
{
    // Calculate all link budget parameters
    double eirp = calculateEIRP();
    double fsl = calculateFreeSpaceLoss();
    double totalLoss = fsl + atmosphericLossSpin_->value() + rainLossSpin_->value()
                       + polarizationLossSpin_->value() + miscLossSpin_->value();
    double rxPower = calculateReceivedPower();
    double gt = calculateGT();

    // Calculate noise power: N = k*T*B (in dBW)
    const double BOLTZMANN = 1.380649e-23; // J/K
    double bwHz = bandwidthSpin_->value();
    if (bandwidthUnitCombo_->currentText() == "kHz") bwHz *= 1e3;
    else if (bandwidthUnitCombo_->currentText() == "MHz") bwHz *= 1e6;
    else if (bandwidthUnitCombo_->currentText() == "GHz") bwHz *= 1e9;

    double noisePowerWatts = BOLTZMANN * systemNoiseTempSpin_->value() * bwHz;
    double noisePower_dBW = 10.0 * log10(noisePowerWatts);

    // C/N ratio
    double cnr = rxPower - noisePower_dBW;

    // Link margin
    double linkMargin = cnr - requiredSNRSpin_->value() - implementationLossSpin_->value();

    // Update result labels
    eirpLabel_->setText(QString::number(eirp, 'f', 2) + " dBW");
    freeSpaceLossLabel_->setText(QString::number(fsl, 'f', 2) + " dB");
    totalPathLossLabel_->setText(QString::number(totalLoss, 'f', 2) + " dB");
    rxPowerLabel_->setText(QString::number(rxPower, 'f', 2) + " dBW");
    noisePowerLabel_->setText(QString::number(noisePower_dBW, 'f', 2) + " dBW");
    cnrLabel_->setText(QString::number(cnr, 'f', 2) + " dB");
    gtLabel_->setText(QString::number(gt, 'f', 2) + " dB/K");

    // Color code link margin
    if (linkMargin >= 3.0) {
        linkMarginLabel_->setStyleSheet("color: green; font-weight: bold; font-size: 12pt;");
    } else if (linkMargin >= 0.0) {
        linkMarginLabel_->setStyleSheet("color: orange; font-weight: bold; font-size: 12pt;");
    } else {
        linkMarginLabel_->setStyleSheet("color: red; font-weight: bold; font-size: 12pt;");
    }
    linkMarginLabel_->setText(QString::number(linkMargin, 'f', 2) + " dB");

    // Update detailed table
    budgetTable_->setRowCount(0);
    auto addRow = [this](const QString& param, const QString& value) {
        int row = budgetTable_->rowCount();
        budgetTable_->insertRow(row);
        budgetTable_->setItem(row, 0, new QTableWidgetItem(param));
        budgetTable_->setItem(row, 1, new QTableWidgetItem(value));
    };

    addRow("Tx Power", QString::number(txPowerSpin_->value(), 'f', 2) + " dBW");
    addRow("Tx Line Loss", QString::number(-txLineLossSpin_->value(), 'f', 2) + " dB");
    addRow("Tx Antenna Gain", QString::number(txAntennaGainSpin_->value(), 'f', 2) + " dBi");
    addRow("EIRP", QString::number(eirp, 'f', 2) + " dBW");
    addRow("Free Space Loss", QString::number(-fsl, 'f', 2) + " dB");
    addRow("Atmospheric Loss", QString::number(-atmosphericLossSpin_->value(), 'f', 2) + " dB");
    addRow("Rain Loss", QString::number(-rainLossSpin_->value(), 'f', 2) + " dB");
    addRow("Polarization Loss", QString::number(-polarizationLossSpin_->value(), 'f', 2) + " dB");
    addRow("Misc. Losses", QString::number(-miscLossSpin_->value(), 'f', 2) + " dB");
    addRow("Rx Antenna Gain", QString::number(rxAntennaGainSpin_->value(), 'f', 2) + " dBi");
    addRow("Rx Line Loss", QString::number(-rxLineLossSpin_->value(), 'f', 2) + " dB");
    addRow("Received Power", QString::number(rxPower, 'f', 2) + " dBW");
    addRow("Noise Power", QString::number(noisePower_dBW, 'f', 2) + " dBW");
    addRow("C/N Ratio", QString::number(cnr, 'f', 2) + " dB");
    addRow("Required SNR", QString::number(requiredSNRSpin_->value(), 'f', 2) + " dB");
    addRow("Implementation Loss", QString::number(-implementationLossSpin_->value(), 'f', 2) + " dB");
    addRow("Link Margin", QString::number(linkMargin, 'f', 2) + " dB");
}

double LinkBudgetModule::calculateEIRP() const
{
    return txPowerSpin_->value() - txLineLossSpin_->value() + txAntennaGainSpin_->value();
}

double LinkBudgetModule::calculateFreeSpaceLoss() const
{
    // FSL = 20*log10(d) + 20*log10(f) + 20*log10(4*pi/c)
    // where d is distance in meters, f is frequency in Hz

    // Convert frequency to Hz
    double freqHz = frequencySpin_->value();
    if (frequencyUnitCombo_->currentText() == "MHz") {
        freqHz *= 1e6;
    } else if (frequencyUnitCombo_->currentText() == "GHz") {
        freqHz *= 1e9;
    }

    // Convert distance to meters
    double distMeters = distanceSpin_->value();
    if (distanceUnitCombo_->currentText() == "km") {
        distMeters *= 1000.0;
    } else if (distanceUnitCombo_->currentText() == "mi") {
        distMeters *= 1609.34;
    } else if (distanceUnitCombo_->currentText() == "nmi") {
        distMeters *= 1852.0;
    }

    const double SPEED_OF_LIGHT = 299792458.0; // m/s
    double fsl = 20.0 * log10(distMeters) + 20.0 * log10(freqHz) + 20.0 * log10(4.0 * M_PI / SPEED_OF_LIGHT);

    return fsl;
}

double LinkBudgetModule::calculateReceivedPower() const
{
    double eirp = calculateEIRP();
    double fsl = calculateFreeSpaceLoss();
    double totalLoss = fsl + atmosphericLossSpin_->value() + rainLossSpin_->value()
                       + polarizationLossSpin_->value() + miscLossSpin_->value();
    double rxPower = eirp - totalLoss + rxAntennaGainSpin_->value() - rxLineLossSpin_->value();
    return rxPower;
}

double LinkBudgetModule::calculateGT() const
{
    // G/T = Antenna Gain - 10*log10(System Noise Temp)
    double gt = rxAntennaGainSpin_->value() - 10.0 * log10(systemNoiseTempSpin_->value());
    return gt;
}

double LinkBudgetModule::calculateLinkMargin() const
{
    double rxPower = calculateReceivedPower();

    // Calculate noise power
    const double BOLTZMANN = 1.380649e-23;
    double bwHz = bandwidthSpin_->value();
    if (bandwidthUnitCombo_->currentText() == "kHz") bwHz *= 1e3;
    else if (bandwidthUnitCombo_->currentText() == "MHz") bwHz *= 1e6;
    else if (bandwidthUnitCombo_->currentText() == "GHz") bwHz *= 1e9;

    double noisePowerWatts = BOLTZMANN * systemNoiseTempSpin_->value() * bwHz;
    double noisePower_dBW = 10.0 * log10(noisePowerWatts);

    double cnr = rxPower - noisePower_dBW;
    double linkMargin = cnr - requiredSNRSpin_->value() - implementationLossSpin_->value();

    return linkMargin;
}

double LinkBudgetModule::calculateAtmosphericLoss() const
{
    return atmosphericLossSpin_->value();
}

double LinkBudgetModule::calculateRainLoss() const
{
    return rainLossSpin_->value();
}
