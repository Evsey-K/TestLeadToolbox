// src/modules/linkbudget/LinkBudgetModel.cpp

#include "LinkBudgetModel.h"
#include <QJsonObject>
#include <QDebug>

LinkBudgetModel::LinkBudgetModel(QObject* parent)
    : QObject(parent)
    , txPower_(10.0)
    , txLineLoss_(0.5)
    , txAntennaGain_(20.0)
    , txPolarization_("Linear")
    , frequency_(2.4)
    , frequencyUnit_("GHz")
    , distance_(100.0)
    , distanceUnit_("km")
    , atmosphericLoss_(0.2)
    , rainLoss_(0.0)
    , polarizationLoss_(0.0)
    , miscLoss_(0.0)
    , rxAntennaGain_(15.0)
    , rxLineLoss_(0.5)
    , systemNoiseTemp_(290.0)
    , noiseFigure_(3.0)
    , bandwidth_(1.0)
    , bandwidthUnit_("MHz")
    , requiredSNR_(10.0)
    , implementationLoss_(1.0)
    , name_("Untitled Link Budget")
    , description_("")
{
}

LinkBudgetModel::~LinkBudgetModel()
{
}

QJsonObject LinkBudgetModel::toJson() const
{
    QJsonObject json;

    // Metadata
    json["name"] = name_;
    json["description"] = description_;
    json["version"] = "1.0";

    // Transmitter
    QJsonObject tx;
    tx["power"] = txPower_;
    tx["lineLoss"] = txLineLoss_;
    tx["antennaGain"] = txAntennaGain_;
    tx["polarization"] = txPolarization_;
    json["transmitter"] = tx;

    // Path
    QJsonObject path;
    path["frequency"] = frequency_;
    path["frequencyUnit"] = frequencyUnit_;
    path["distance"] = distance_;
    path["distanceUnit"] = distanceUnit_;
    path["atmosphericLoss"] = atmosphericLoss_;
    path["rainLoss"] = rainLoss_;
    path["polarizationLoss"] = polarizationLoss_;
    path["miscLoss"] = miscLoss_;
    json["path"] = path;

    // Receiver
    QJsonObject rx;
    rx["antennaGain"] = rxAntennaGain_;
    rx["lineLoss"] = rxLineLoss_;
    rx["systemNoiseTemp"] = systemNoiseTemp_;
    rx["noiseFigure"] = noiseFigure_;
    rx["bandwidth"] = bandwidth_;
    rx["bandwidthUnit"] = bandwidthUnit_;
    rx["requiredSNR"] = requiredSNR_;
    rx["implementationLoss"] = implementationLoss_;
    json["receiver"] = rx;

    return json;
}

void LinkBudgetModel::fromJson(const QJsonObject& json)
{
    // Metadata
    name_ = json.value("name").toString("Untitled Link Budget");
    description_ = json.value("description").toString("");

    // Transmitter
    QJsonObject tx = json.value("transmitter").toObject();
    txPower_ = tx.value("power").toDouble(10.0);
    txLineLoss_ = tx.value("lineLoss").toDouble(0.5);
    txAntennaGain_ = tx.value("antennaGain").toDouble(20.0);
    txPolarization_ = tx.value("polarization").toString("Linear");

    // Path
    QJsonObject path = json.value("path").toObject();
    frequency_ = path.value("frequency").toDouble(2.4);
    frequencyUnit_ = path.value("frequencyUnit").toString("GHz");
    distance_ = path.value("distance").toDouble(100.0);
    distanceUnit_ = path.value("distanceUnit").toString("km");
    atmosphericLoss_ = path.value("atmosphericLoss").toDouble(0.2);
    rainLoss_ = path.value("rainLoss").toDouble(0.0);
    polarizationLoss_ = path.value("polarizationLoss").toDouble(0.0);
    miscLoss_ = path.value("miscLoss").toDouble(0.0);

    // Receiver
    QJsonObject rx = json.value("receiver").toObject();
    rxAntennaGain_ = rx.value("antennaGain").toDouble(15.0);
    rxLineLoss_ = rx.value("lineLoss").toDouble(0.5);
    systemNoiseTemp_ = rx.value("systemNoiseTemp").toDouble(290.0);
    noiseFigure_ = rx.value("noiseFigure").toDouble(3.0);
    bandwidth_ = rx.value("bandwidth").toDouble(1.0);
    bandwidthUnit_ = rx.value("bandwidthUnit").toString("MHz");
    requiredSNR_ = rx.value("requiredSNR").toDouble(10.0);
    implementationLoss_ = rx.value("implementationLoss").toDouble(1.0);

    emit modelChanged();

    qDebug() << "LinkBudgetModel: Loaded from JSON -" << name_;
}
