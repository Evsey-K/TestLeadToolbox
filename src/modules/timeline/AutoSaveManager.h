// AutoSaveManager.h


#pragma once
#include <QObject>
#include <QTimer>
#include <QString>
#include <QDateTime>


class TimelineModel;


/**
 * @class AutoSaveManager
 * @brief Manages automatic saving of timeline data at regular intervals
 *
 * Features:
 * - Configurable save interval (default: 5 minutes)
 * - Tracks if model has unsaved changes
 * - Only saves when data has changed
 * - Provides manual save trigger
 * - Emits signals on save success/failure
 */
class AutoSaveManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Construct auto-save manager
     * @param model Timeline model to save
     * @param saveFilePath Path where data should be saved
     * @param parent Qt parent object
     */
    explicit AutoSaveManager(TimelineModel* model,
                             const QString& saveFilePath,
                             QObject* parent = nullptr);

    /**
     * @brief Start auto-save timer
     * @param intervalMs Save interval in milliseconds (default: 5 minutes)
     */
    void startAutoSave(int intervalMs = 300000);

    /**
     * @brief Stop auto-save timer
     */
    void stopAutoSave();

    /**
     * @brief Check if auto-save is currently enabled
     */
    bool isAutoSaveEnabled() const
    {
        return autoSaveTimer_->isActive();
    }

    /**
     * @brief Get current save interval in milliseconds
     */
    int saveInterval() const
    {
        return autoSaveTimer_->interval();
    }

    /**
     * @brief Set save interval
     * @param intervalMs New interval in milliseconds
     */
    void setSaveInterval(int intervalMs);

    /**
     * @brief Check if model has unsaved changes
     */
    bool hasUnsavedChanges() const
    {
        return hasUnsavedChanges_;
    }

    /**
     * @brief Set the save file path
     */
    void setSaveFilePath(const QString& filePath)
    {
        saveFilePath_ = filePath;
    }

    /**
     * @brief Get current save file path
     */
    QString saveFilePath() const
    {
        return saveFilePath_;
    }

public slots:
    /**
     * @brief Manually trigger save
     * @return true if save succeeded
     */
    bool saveNow();

    /**
     * @brief Mark model as having unsaved changes
     */
    void markDirty();

    /**
     * @brief Clear unsaved changes flag (call after manual save)
     */
    void markClean();

signals:
    /**
     * @brief Emitted when auto-save completes successfully
     */
    void autoSaveCompleted(const QString& filePath);

    /**
     * @brief Emitted when auto-save fails
     */
    void autoSaveFailed(const QString& error);

    /**
     * @brief Emitted when unsaved changes state changes
     */
    void unsavedChangesChanged(bool hasUnsavedChanges);

private slots:
    /**
     * @brief Handle auto-save timer timeout
     */
    void onAutoSaveTimer();

    /**
     * @brief Handle model changes that require saving
     */
    void onModelChanged();

private:
    TimelineModel* model_;              ///< Model to save (not owned)
    QString saveFilePath_;              ///< Path to save file
    QTimer* autoSaveTimer_;             ///< Timer for periodic saves
    bool hasUnsavedChanges_;            ///< Tracks if save is needed
    QDateTime lastSaveTime_;            ///< Timestamp of last successful save
};
