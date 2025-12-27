// TimelineCommands.h


#pragma once
#include <QUndoCommand>
#include "TimelineModel.h"


/**
 * @class AddEventCommand
 * @brief Undoable command for adding a timeline event
 */
class AddEventCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct an add event command
     */
    AddEventCommand(TimelineModel* model, const TimelineEvent& event, QUndoCommand* parent = nullptr);


    void redo() override;       ///< Execute the add operation (redo)
    void undo() override;       ///< Reverse the add operation (undo)

private:
    TimelineModel* model_;      ///< Model to modify (not owned)
    TimelineEvent event_;       ///< Event to add/remove
    QString eventId_;           ///< ID assigned to the event
};


/**
 * @class DeleteEventCommand
 * @brief Undoable command for deleting/archiving a timeline event
 */
class DeleteEventCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct a delete event command
     */
    DeleteEventCommand(TimelineModel* model,
                       const QString& eventId,
                       bool softDelete = true,
                       QUndoCommand* parent = nullptr);

    void redo() override;           ///< Execute the delete operation (redo)
    void undo() override;           ///< Reverse the delete operation (undo)

private:
    TimelineModel* model_;          ///< Model to modify (not owned)
    QString eventId_;               ///< Event ID to delete
    TimelineEvent eventBackup_;     ///< Backup for undo
    bool softDelete_;               ///< Archive vs hard delete
    bool firstRun_;                 ///< Track if this is the first execution
};


/**
 * @class UpdateEventCommand
 * @brief Undoable command for modifying a timeline event
 */
class UpdateEventCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct an update event command
     */
    UpdateEventCommand(TimelineModel* model,
                       const QString& eventId,
                       const TimelineEvent& newEvent,
                       QUndoCommand* parent = nullptr);


    void redo() override;           ///< Execute the update operation (redo)
    void undo() override;           ///< Reverse the update operation (undo)

private:
    TimelineModel* model_;          ///< Model to modify (not owned)
    QString eventId_;               ///< Event ID to update
    TimelineEvent oldEvent_;        ///< Original event state
    TimelineEvent newEvent_;        ///< Updated event state
    bool firstRun_;                 ///< Track if this is the first execution
};


/**
 * @class BatchDeleteCommand
 * @brief Undoable command for deleting multiple events at once
 */
class BatchDeleteCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct a batch delete command
     */
    BatchDeleteCommand(TimelineModel* model,
                       const QStringList& eventIds,
                       bool softDelete = true,
                       QUndoCommand* parent = nullptr);

    // No need to override redo/undo - child commands handle it
};


/**
 * @class RestoreEventCommand
 * @brief Undoable command for restoring an archived event
 */
class RestoreEventCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct a restore event command
     */
    RestoreEventCommand(TimelineModel* model,
                        const QString& eventId,
                        QUndoCommand* parent = nullptr);

    void redo() override;       ///< Execute the restore operation (redo)
    void undo() override;       ///< Reverse the restore operation (undo)

private:
    TimelineModel* model_;      ///< Model to modify (not owned)
    QString eventId_;           ///< Event ID to restore
    bool firstRun_;             ///< Track if this is the first execution
};


/**
 * @class SetEventLockStateCommand
 * @brief Undoable command for changing event lock/fixed state
 */
class SetEventLockStateCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct a lock state change command
     */
    SetEventLockStateCommand(TimelineModel* model,
                             const QString& eventId,
                             bool newFixed,
                             bool newLocked,
                             bool oldFixed,
                             bool oldLocked,
                             QUndoCommand* parent = nullptr);

    void redo() override;       ///< Execute the lock state change (redo)
    void undo() override;       ///< Reverse the lock state change (undo)

private:
    TimelineModel* model_;      ///< Model to modify (not owned)
    QString eventId_;           ///< Event ID to modify
    bool newFixed_;             ///< New fixed state
    bool newLocked_;            ///< New locked state
    bool oldFixed_;             ///< Original fixed state
    bool oldLocked_;            ///< Original locked state
};
