// TimelineSerializer.h

#pragma once
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "TimelineModel.h"

/**
 * @class TimelineSerializer
 * @brief Handles serialization and deserialization of timeline data to/from JSON
 *
 * Provides functionality to:
 * - Save timeline model to JSON file
 * - Load timeline model from JSON file
 * - Auto-save with configurable intervals
 * - Backup management
 */
class TimelineSerializer
{
public:
    /**
     * @brief Save timeline model to JSON file
     * @param model Timeline model to save
     * @param filePath Full path to save location
     * @return true if save succeeded, false otherwise
     */
    static bool saveToFile(const TimelineModel* model, const QString& filePath);

    /**
     * @brief Load timeline model from JSON file
     * @param model Timeline model to populate (must be non-null)
     * @param filePath Full path to load from
     * @return true if load succeeded, false otherwise
     */
    static bool loadFromFile(TimelineModel* model, const QString& filePath);


    /**
     * @brief Serialize model to JSON object
     * @param model Timeline model to serialize
     * @return QJsonObject containing all model data
     */
    static QJsonObject serializeModel(const TimelineModel* model);

    /**
     * @brief Deserialize JSON object to model
     * @param model Timeline model to populate
     * @param json JSON object containing model data
     * @return true if deserialization succeeded
     */
    static bool deserializeModel(TimelineModel* model, const QJsonObject& json);

    /**
     * @brief Get default save location for timeline data
     * @return Full path to default timeline file
     */
    static QString getDefaultSaveLocation();

    /**
     * @brief Create backup of existing file
     * @param filePath Original file path
     * @return true if backup created successfully
     */
    static bool createBackup(const QString& filePath);

private:
    /**
     * @brief Serialize a single event to JSON
     */
    static QJsonObject serializeEvent(const TimelineEvent& event);

    /**
     * @brief Deserialize a single event from JSON
     */
    static TimelineEvent deserializeEvent(const QJsonObject& json);

    /**
     * @brief Convert event type enum to string
     */
    static QString eventTypeToString(TimelineEventType type);

    /**
     * @brief Convert string to event type enum
     */
    static TimelineEventType stringToEventType(const QString& typeStr);
};
