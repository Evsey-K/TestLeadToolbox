// TimelineCommands.cpp


#include "TimelineCommands.h"
#include <QDebug>


// ============================================================================
// AddEventCommand Implementation
// ============================================================================

AddEventCommand::AddEventCommand(TimelineModel* model,
                                 const TimelineEvent& event,
                                 QUndoCommand* parent)
    : QUndoCommand(parent)
    , model_(model)
    , event_(event)
{
    setText(QString("Add Event: %1").arg(event.title));
}


void AddEventCommand::redo()
{
    // Add event to model
    eventId_ = model_->addEvent(event_);

    // Update the event_ with the generated ID
    event_.id = eventId_;

    qDebug() << "AddEventCommand::redo() - Added event:" << eventId_;
}


void AddEventCommand::undo()
{
    // Remove event from model
    bool success = model_->removeEvent(eventId_);

    if (!success)
    {
        qWarning() << "AddEventCommand::undo() - Failed to remove event:" << eventId_;
    }

    qDebug() << "AddEventCommand::undo() - Removed event:" << eventId_;
}


// ============================================================================
// DeleteEventCommand Implementation
// ============================================================================

DeleteEventCommand::DeleteEventCommand(TimelineModel* model,
                                       const QString& eventId,
                                       bool softDelete,
                                       QUndoCommand* parent)
    : QUndoCommand(parent)
    , model_(model)
    , eventId_(eventId)
    , softDelete_(softDelete)
    , firstRun_(true)
{
    // Get event for backup
    const TimelineEvent* event = model_->getEvent(eventId_);

    if (event)
    {
        eventBackup_ = *event;

        if (softDelete_)
        {
            setText(QString("Archive Event: %1").arg(event->title));
        }
        else
        {
            setText(QString("Delete Event: %1").arg(event->title));
        }
    }
    else
    {
        setText("Delete Event");
    }
}


void DeleteEventCommand::redo()
{
    if (firstRun_)
    {
        firstRun_ = false;

        // On first execution, backup the event
        const TimelineEvent* event = model_->getEvent(eventId_);
        if (event)
        {
            eventBackup_ = *event;
        }
    }

    if (softDelete_)
    {
        // Archive the event
        bool success = model_->archiveEvent(eventId_);

        if (!success)
        {
            qWarning() << "DeleteEventCommand::redo() - Failed to archive event:" << eventId_;
        }

        qDebug() << "DeleteEventCommand::redo() - Archived event:" << eventId_;
    }
    else
    {
        // Hard delete the event
        bool success = model_->removeEvent(eventId_);

        if (!success)
        {
            qWarning() << "DeleteEventCommand::redo() - Failed to delete event:" << eventId_;
        }

        qDebug() << "DeleteEventCommand::redo() - Deleted event:" << eventId_;
    }
}


void DeleteEventCommand::undo()
{
    if (softDelete_)
    {
        // Restore from archive
        bool success = model_->restoreEvent(eventId_);

        if (!success)
        {
            qWarning() << "DeleteEventCommand::undo() - Failed to restore event:" << eventId_;
        }

        qDebug() << "DeleteEventCommand::undo() - Restored event:" << eventId_;
    }
    else
    {
        // Re-add the event (hard delete undo)
        QString newId = model_->addEvent(eventBackup_);

        if (newId.isEmpty())
        {
            qWarning() << "DeleteEventCommand::undo() - Failed to re-add event:" << eventId_;
        }

        qDebug() << "DeleteEventCommand::undo() - Re-added event:" << eventId_;
    }
}


// ============================================================================
// UpdateEventCommand Implementation
// ============================================================================

UpdateEventCommand::UpdateEventCommand(TimelineModel* model,
                                       const QString& eventId,
                                       const TimelineEvent& newEvent,
                                       QUndoCommand* parent)
    : QUndoCommand(parent)
    , model_(model)
    , eventId_(eventId)
    , newEvent_(newEvent)
    , firstRun_(true)
{
    // Get old event for backup
    const TimelineEvent* oldEvent = model_->getEvent(eventId_);

    if (oldEvent)
    {
        oldEvent_ = *oldEvent;

        // Determine what changed to create descriptive text
        bool datesChanged = (oldEvent->startDate != newEvent.startDate ||
                             oldEvent->endDate != newEvent.endDate);
        bool laneChanged = (oldEvent->lane != newEvent.lane);

        if (datesChanged && laneChanged)
        {
            setText(QString("Move Event: %1").arg(oldEvent->title));
        }
        else if (datesChanged)
        {
            setText(QString("Resize Event: %1").arg(oldEvent->title));
        }
        else
        {
            setText(QString("Edit Event: %1").arg(oldEvent->title));
        }
    }
    else
    {
        setText("Edit Event");
    }
}


void UpdateEventCommand::redo()
{
    if (firstRun_)
    {
        firstRun_ = false;

        // On first execution, backup the original event
        const TimelineEvent* event = model_->getEvent(eventId_);
        if (event)
        {
            oldEvent_ = *event;
        }
    }

    // Apply the update
    bool success = model_->updateEvent(eventId_, newEvent_);

    if (!success)
    {
        qWarning() << "UpdateEventCommand::redo() - Failed to update event:" << eventId_;
    }

    qDebug() << "UpdateEventCommand::redo() - Updated event:" << eventId_;
}


void UpdateEventCommand::undo()
{
    // Restore the old event
    bool success = model_->updateEvent(eventId_, oldEvent_);

    if (!success)
    {
        qWarning() << "UpdateEventCommand::undo() - Failed to restore event:" << eventId_;
    }

    qDebug() << "UpdateEventCommand::undo() - Restored event:" << eventId_;
}


// ============================================================================
// BatchDeleteCommand Implementation
// ============================================================================

BatchDeleteCommand::BatchDeleteCommand(TimelineModel* model,
                                       const QStringList& eventIds,
                                       bool softDelete,
                                       QUndoCommand* parent)
    : QUndoCommand(parent)
{
    if (eventIds.size() == 1)
    {
        setText(QString("Delete 1 event"));
    }
    else
    {
        setText(QString("Delete %1 events").arg(eventIds.size()));
    }

    // Create child commands for each event
    for (const QString& eventId : eventIds)
    {
        new DeleteEventCommand(model, eventId, softDelete, this);
    }
}


// ============================================================================
// RestoreEventCommand Implementation
// ============================================================================

RestoreEventCommand::RestoreEventCommand(TimelineModel* model,
                                         const QString& eventId,
                                         QUndoCommand* parent)
    : QUndoCommand(parent)
    , model_(model)
    , eventId_(eventId)
    , firstRun_(true)
{
    const TimelineEvent* event = model_->getArchivedEvent(eventId_);

    if (event)
    {
        setText(QString("Restore Event: %1").arg(event->title));
    }
    else
    {
        setText("Restore Event");
    }
}


void RestoreEventCommand::redo()
{
    firstRun_ = false;

    // Restore the event from archive
    bool success = model_->restoreEvent(eventId_);

    if (!success)
    {
        qWarning() << "RestoreEventCommand::redo() - Failed to restore event:" << eventId_;
    }

    qDebug() << "RestoreEventCommand::redo() - Restored event:" << eventId_;
}


void RestoreEventCommand::undo()
{
    // Archive the event again
    bool success = model_->archiveEvent(eventId_);

    if (!success)
    {
        qWarning() << "RestoreEventCommand::undo() - Failed to re-archive event:" << eventId_;
    }

    qDebug() << "RestoreEventCommand::undo() - Re-archived event:" << eventId_;
}


// ============================================================================
// SetEventLockStateCommand Implementation
// ============================================================================


SetEventLockStateCommand::SetEventLockStateCommand(TimelineModel* model,
                                                   const QString& eventId,
                                                   bool newFixed,
                                                   bool newLocked,
                                                   bool oldFixed,
                                                   bool oldLocked,
                                                   QUndoCommand* parent)
    : QUndoCommand(parent)
    , model_(model)
    , eventId_(eventId)
    , newFixed_(newFixed)
    , newLocked_(newLocked)
    , oldFixed_(oldFixed)
    , oldLocked_(oldLocked)
{
    // Create descriptive command text
    QString lockState;
    if (newLocked)
    {
        lockState = "locked";
    }
    else if (newFixed)
    {
        lockState = "fixed";
    }
    else
    {
        lockState = "unlocked";
    }

    const TimelineEvent* event = model_->getEvent(eventId_);
    if (event)
    {
        setText(QString("Set '%1' to %2").arg(event->title).arg(lockState));
    }
    else
    {
        setText(QString("Set event to %1").arg(lockState));
    }
}


void SetEventLockStateCommand::redo()
{
    bool success = model_->setEventLockState(eventId_, newFixed_, newLocked_);

    if (!success)
    {
        qWarning() << "SetEventLockStateCommand::redo() - Failed to set lock state for event:" << eventId_;
    }

    qDebug() << "SetEventLockStateCommand::redo() - Set event" << eventId_
             << "to fixed=" << newFixed_ << "locked=" << newLocked_;
}


void SetEventLockStateCommand::undo()
{
    bool success = model_->setEventLockState(eventId_, oldFixed_, oldLocked_);

    if (!success)
    {
        qWarning() << "SetEventLockStateCommand::undo() - Failed to restore lock state for event:" << eventId_;
    }

    qDebug() << "SetEventLockStateCommand::undo() - Restored event" << eventId_
             << "to fixed=" << oldFixed_ << "locked=" << oldLocked_;
}
