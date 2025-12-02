// AutoSaveManager.cpp


#include "AutoSaveManager.h"
#include "TimelineModel.h"
#include "TimelineSerializer.h"
#include <QDateTime>
#include <QDebug>


AutoSaveManager::AutoSaveManager(TimelineModel* model,
                                 const QString& saveFilePath,
                                 QObject* parent)
    : QObject(parent)
    , model_(model)
    , saveFilePath_(saveFilePath)
    , hasUnsavedChanges_(false)
{
    // Create timer
    autoSaveTimer_ = new QTimer(this);
    connect(autoSaveTimer_, &QTimer::timeout, this, &AutoSaveManager::onAutoSaveTimer);

    // Connect to model signals to track changes
    connect(model_, &TimelineModel::eventAdded, this, &AutoSaveManager::onModelChanged);
    connect(model_, &TimelineModel::eventRemoved, this, &AutoSaveManager::onModelChanged);
    connect(model_, &TimelineModel::eventUpdated, this, &AutoSaveManager::onModelChanged);
    connect(model_, &TimelineModel::eventsCleared, this, &AutoSaveManager::onModelChanged);
    connect(model_, &TimelineModel::versionDatesChanged, this, &AutoSaveManager::onModelChanged);
}

void AutoSaveManager::startAutoSave(int intervalMs)
{
    if (intervalMs < 1000)
    {
        qWarning() << "Auto-save interval too short, using 1 second minimum";
        intervalMs = 1000;
    }

    autoSaveTimer_->setInterval(intervalMs);
    autoSaveTimer_->start();

    qDebug() << "Auto-save started with interval:" << intervalMs << "ms";
}

void AutoSaveManager::stopAutoSave()
{
    autoSaveTimer_->stop();
    qDebug() << "Auto-save stopped";
}

void AutoSaveManager::setSaveInterval(int intervalMs)
{
    bool wasActive = autoSaveTimer_->isActive();
    autoSaveTimer_->setInterval(intervalMs);

    if (wasActive && !autoSaveTimer_->isActive())
    {
        autoSaveTimer_->start();
    }

    qDebug() << "Auto-save interval changed to:" << intervalMs << "ms";
}

bool AutoSaveManager::saveNow()
{
    qDebug() << "Manual save triggered";

    bool success = TimelineSerializer::saveToFile(model_, saveFilePath_);

    if (success)
    {
        lastSaveTime_ = QDateTime::currentDateTime();
        markClean();
        emit autoSaveCompleted(saveFilePath_);
    }
    else
    {
        emit autoSaveFailed("Failed to save timeline data");
    }

    return success;
}

void AutoSaveManager::markDirty()
{
    if (!hasUnsavedChanges_)
    {
        hasUnsavedChanges_ = true;
        emit unsavedChangesChanged(true);
    }
}

void AutoSaveManager::markClean()
{
    if (hasUnsavedChanges_)
    {
        hasUnsavedChanges_ = false;
        emit unsavedChangesChanged(false);
    }
}

void AutoSaveManager::onAutoSaveTimer()
{
    // Only save if there are unsaved changes
    if (!hasUnsavedChanges_)
    {
        qDebug() << "Auto-save skipped - no changes since last save";
        return;
    }

    qDebug() << "Auto-save triggered";

    bool success = TimelineSerializer::saveToFile(model_, saveFilePath_);

    if (success)
    {
        lastSaveTime_ = QDateTime::currentDateTime();
        markClean();
        emit autoSaveCompleted(saveFilePath_);
        qDebug() << "Auto-save completed successfully";
    }
    else
    {
        emit autoSaveFailed("Auto-save failed");
        qWarning() << "Auto-save failed";
    }
}

void AutoSaveManager::onModelChanged()
{
    markDirty();
}
