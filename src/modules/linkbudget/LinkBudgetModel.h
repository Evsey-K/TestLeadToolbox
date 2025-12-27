// src/modules/linkbudget/LinkBudgetModel.h
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

/**
 * @class LinkBudgetModel
 * @brief Data model for link budget calculations
 *
 * Stores all link budget parameters and provides serialization/deserialization
 * for saving and loading link budget configurations.
 */
class LinkBudgetModel : public QObject
{
    Q_OBJECT

public:
    explicit LinkBudgetModel(QObject* parent = nullptr);
    ~LinkBudgetModel();

    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

    // Transmitter parameters
    void setTxPower(double power) { txPower_ = power; emit modelChanged(); }
    double txPower() const { return txPower_; }

    void setTxLineLoss(double loss) { txLineLoss_ = loss; emit modelChanged(); }
    double txLineLoss() const { return txLineLoss_; }

    void setTxAntennaGain(double gain) { txAntennaGain_ = gain; emit modelChanged(); }
    double txAntennaGain() const { return txAntennaGain_; }

    void setTxPolarization(const QString& pol) { txPolarization_ = pol; emit modelChanged(); }
    QString txPolarization() const { return txPolarization_; }

    // Path parameters
    void setFrequency(double freq) { frequency_ = freq; emit modelChanged(); }
    double frequency() const { return frequency_; }

    void setFrequencyUnit(const QString& unit) { frequencyUnit_ = unit; emit modelChanged(); }
    QString frequencyUnit() const { return frequencyUnit_; }

    void setDistance(double dist) { distance_ = dist; emit modelChanged(); }
    double distance() const { return distance_; }

    void setDistanceUnit(const QString& unit) { distanceUnit_ = unit; emit modelChanged(); }
    QString distanceUnit() const { return distanceUnit_; }

    void setAtmosphericLoss(double loss) { atmosphericLoss_ = loss; emit modelChanged(); }
    double atmosphericLoss() const { return atmosphericLoss_; }

    void setRainLoss(double loss) { rainLoss_ = loss; emit modelChanged(); }
    double rainLoss() const { return rainLoss_; }

    void setPolarizationLoss(double loss) { polarizationLoss_ = loss; emit modelChanged(); }
    double polarizationLoss() const { return polarizationLoss_; }

    void setMiscLoss(double loss) { miscLoss_ = loss; emit modelChanged(); }
    double miscLoss() const { return miscLoss_; }

    // Receiver parameters
    void setRxAntennaGain(double gain) { rxAntennaGain_ = gain; emit modelChanged(); }
    double rxAntennaGain() const { return rxAntennaGain_; }

    void setRxLineLoss(double loss) { rxLineLoss_ = loss; emit modelChanged(); }
    double rxLineLoss() const { return rxLineLoss_; }

    void setSystemNoiseTemp(double temp) { systemNoiseTemp_ = temp; emit modelChanged(); }
    double systemNoiseTemp() const { return systemNoiseTemp_; }

    void setNoiseFigure(double nf) { noiseFigure_ = nf; emit modelChanged(); }
    double noiseFigure() const { return noiseFigure_; }

    void setBandwidth(double bw) { bandwidth_ = bw; emit modelChanged(); }
    double bandwidth() const { return bandwidth_; }

    void setBandwidthUnit(const QString& unit) { bandwidthUnit_ = unit; emit modelChanged(); }
    QString bandwidthUnit() const { return bandwidthUnit_; }

    void setRequiredSNR(double snr) { requiredSNR_ = snr; emit modelChanged(); }
    double requiredSNR() const { return requiredSNR_; }

    void setImplementationLoss(double loss) { implementationLoss_ = loss; emit modelChanged(); }
    double implementationLoss() const { return implementationLoss_; }

    // Metadata
    void setName(const QString& name) { name_ = name; emit modelChanged(); }
    QString name() const { return name_; }

    void setDescription(const QString& desc) { description_ = desc; emit modelChanged(); }
    QString description() const { return description_; }

signals:
    void modelChanged();

private:
    // Transmitter
    double txPower_;
    double txLineLoss_;
    double txAntennaGain_;
    QString txPolarization_;

    // Path
    double frequency_;
    QString frequencyUnit_;
    double distance_;
    QString distanceUnit_;
    double atmosphericLoss_;
    double rainLoss_;
    double polarizationLoss_;
    double miscLoss_;

    // Receiver
    double rxAntennaGain_;
    double rxLineLoss_;
    double systemNoiseTemp_;
    double noiseFigure_;
    double bandwidth_;
    QString bandwidthUnit_;
    double requiredSNR_;
    double implementationLoss_;

    // Metadata
    QString name_;
    QString description_;
};
