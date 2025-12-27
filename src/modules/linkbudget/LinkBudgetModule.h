// src/modules/linkbudget/LinkBudgetModule.h
#pragma once

#include "shared/interfaces/IModule.h"
#include <QWidget>

class LinkBudgetModel;
class QFormLayout;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;
class QDoubleSpinBox;
class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTextEdit;

/**
 * @class LinkBudgetModule
 * @brief RF Link Budget Calculator module
 *
 * Provides comprehensive link budget analysis for RF communications systems,
 * including satellite, terrestrial, and other wireless links. Calculates
 * received power, link margin, and other critical parameters.
 */
class LinkBudgetModule : public QWidget, public IModule
{
    Q_OBJECT

public:
    explicit LinkBudgetModule(QWidget* parent = nullptr);
    ~LinkBudgetModule() override;

    // IModule interface implementation
    QWidget* createWidget(QWidget* parent = nullptr) override;
    QString name() const override;
    QString description() const override;
    QIcon icon() const override;
    QString moduleId() const override;
    void onActivate() override;
    void onDeactivate() override;
    bool hasUnsavedChanges() const override;
    bool save() override;

private:
    void setupUI();
    void createTransmitterSection();
    void createPathSection();
    void createReceiverSection();
    void createResultsSection();
    void createControlButtons();
    void connectSignals();

    void calculate();
    void clearAll();
    void loadPreset();
    void savePreset();
    void exportResults();

    void onParameterChanged();
    void updateResults();

    // Helper methods
    double calculateFreeSpaceLoss() const;
    double calculateAtmosphericLoss() const;
    double calculateRainLoss() const;
    double calculateReceivedPower() const;
    double calculateLinkMargin() const;
    double calculateEIRP() const;
    double calculateGT() const;

private:
    // Model
    LinkBudgetModel* model_;

    // Main layout sections
    QGroupBox* transmitterGroup_;
    QGroupBox* pathGroup_;
    QGroupBox* receiverGroup_;
    QGroupBox* resultsGroup_;

    // Transmitter parameters
    QDoubleSpinBox* txPowerSpin_;
    QDoubleSpinBox* txLineLossSpin_;
    QDoubleSpinBox* txAntennaGainSpin_;
    QComboBox* txPolarizationCombo_;

    // Path parameters
    QDoubleSpinBox* frequencySpin_;
    QComboBox* frequencyUnitCombo_;
    QDoubleSpinBox* distanceSpin_;
    QComboBox* distanceUnitCombo_;
    QDoubleSpinBox* atmosphericLossSpin_;
    QDoubleSpinBox* rainLossSpin_;
    QDoubleSpinBox* polarizationLossSpin_;
    QDoubleSpinBox* miscLossSpin_;

    // Receiver parameters
    QDoubleSpinBox* rxAntennaGainSpin_;
    QDoubleSpinBox* rxLineLossSpin_;
    QDoubleSpinBox* systemNoiseTempSpin_;
    QDoubleSpinBox* noiseFigureSpin_;
    QDoubleSpinBox* bandwidthSpin_;
    QComboBox* bandwidthUnitCombo_;
    QDoubleSpinBox* requiredSNRSpin_;
    QDoubleSpinBox* implementationLossSpin_;

    // Results display
    QLabel* eirpLabel_;
    QLabel* freeSpaceLossLabel_;
    QLabel* totalPathLossLabel_;
    QLabel* rxPowerLabel_;
    QLabel* noisePowerLabel_;
    QLabel* cnrLabel_;
    QLabel* gtLabel_;
    QLabel* linkMarginLabel_;
    QTableWidget* budgetTable_;

    // Control buttons
    QPushButton* calculateButton_;
    QPushButton* clearButton_;
    QPushButton* loadPresetButton_;
    QPushButton* savePresetButton_;
    QPushButton* exportButton_;

    // State tracking
    bool hasUnsavedChanges_;
};
