// TimelineCommands.cpp


#include "TimelineCommands.h"
#include <QDebug>


// ============================================================================
// AddEventCommand Implementation
// ============================================================================

AddEventCommand::AddEventCommand(TimelineModel* model, const TimelineEvent& event, QUndoCommand* parent)
    : QUndoCommand(parent)
    , model_(model)
    , event_(event)
    , firstRedo_(true)
{
    setText(QString("Add Event: %1").arg(event.title));
}


void AddEventCommand::undo()
{
    if (!eventId_.isEmpty())
    {
        model_->removeEvent(eventId_);
    }
}


void AddEventCommand::redo()
{
    if (firstRedo_)
    {
        // First time - actually add the event and capture the ID
        eventId_ = model_->addEvent(event_);
        firstRedo_ = false;
    }
    else
    {
        // Subsequent redo - re-add the same event with preserved ID
        TimelineEvent eventWithId = event_;
        eventWithId.id = eventId_;
        model_->addEvent(eventWithId);
    }
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
    , firstRedo_(true)
{
    // Capture event before deletion
    const TimelineEvent* event = model_->getEvent(eventId);
    if (event)
    {
        deletedEvent_ = *event;
        setText(QString("Delete Event: %1").arg(deletedEvent_.title));
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
        setText(QString("Edit Event: %1").arg(oldEvent->title));
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


bool UpdateEventCommand::mergeWith(const QUndoCommand* other)
{
    // Can merge if the same event is being updated multiple times
    const UpdateEventCommand* otherUpdate = dynamic_cast<const UpdateEventCommand*>(other);

    if (!otherUpdate || otherUpdate->eventId_ != eventId_)
    {
        return false;
    }

    // Merge by updating newEvent_ with the latest changes
    // This allows multiple rapid edits to collapse into a single undo step
    newEvent_ = otherUpdate->newEvent_;

    qDebug() << "UpdateEventCommand::mergeWith() - Merged updates for event:" << eventId_;

    return true;
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
