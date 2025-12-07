// TimelineCommands.h


#pragma once
#include <QUndoCommand>
#include "TimelineModel.h"


/**
 * @class AddEventCommand
 * @brief Undoable command for adding a timeline event
 *
 * Supports undo (removes event) and redo (re-adds event).
 * Preserves the event ID across undo/redo cycles.
 */
class AddEventCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct an add event command
     * @param model Timeline model to modify
     * @param event Event to add
     * @param parent Parent command for command grouping
     */
    AddEventCommand(TimelineModel* model, const TimelineEvent& event, QUndoCommand* parent = nullptr);

    /**
     * @brief Execute the add operation (redo)
     */
    void redo() override;

    /**
     * @brief Reverse the add operation (undo)
     */
    void undo() override;

private:
    TimelineModel* model_;      ///< Model to modify (not owned)
    TimelineEvent event_;       ///< Event to add/remove
    QString eventId_;           ///< ID assigned to the event
};


/**
 * @class DeleteEventCommand
 * @brief Undoable command for deleting/archiving a timeline event
 *
 * Supports soft delete (archive) or hard delete based on settings.
 * Can restore archived events on undo.
 */
class DeleteEventCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct a delete event command
     * @param model Timeline model to modify
     * @param eventId Event ID to delete
     * @param softDelete If true, archives instead of deleting
     * @param parent Parent command for command grouping
     */
    DeleteEventCommand(TimelineModel* model,
                       const QString& eventId,
                       bool softDelete = true,
                       QUndoCommand* parent = nullptr);

    /**
     * @brief Execute the delete operation (redo)
     */
    void redo() override;

    /**
     * @brief Reverse the delete operation (undo)
     */
    void undo() override;

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
 *
 * Supports undo/redo for event modifications including:
 * - Manual edits via Edit dialog
 * - Drag-and-drop moves
 * - Edge resize operations
 *
 * Can merge consecutive updates to the same event.
 */
class UpdateEventCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct an update event command
     * @param model Timeline model to modify
     * @param eventId Event ID to update
     * @param newEvent New event data
     * @param commandText Custom command text (optional, defaults to "Edit Event: {title}")
     * @param parent Parent command for command grouping
     */
    UpdateEventCommand(TimelineModel* model,
                       const QString& eventId,
                       const TimelineEvent& newEvent,
                       QUndoCommand* parent = nullptr);


    void redo() override;                                   ///< @brief Execute the update operation (redo)
    void undo() override;                                   ///< @brief Reverse the update operation (undo)

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
 *
 * Wraps multiple DeleteEventCommand instances into a single undo/redo operation.
 */
class BatchDeleteCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct a batch delete command
     * @param model Timeline model to modify
     * @param eventIds List of event IDs to delete
     * @param softDelete If true, archives instead of deleting
     * @param parent Parent command for command grouping
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
 *
 * Moves an event from archived state back to active state.
 */
class RestoreEventCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct a restore event command
     * @param model Timeline model to modify
     * @param eventId Event ID to restore
     * @param parent Parent command for command grouping
     */
    RestoreEventCommand(TimelineModel* model,
                        const QString& eventId,
                        QUndoCommand* parent = nullptr);

    /**
     * @brief Execute the restore operation (redo)
     */
    void redo() override;

    /**
     * @brief Reverse the restore operation (undo)
     */
    void undo() override;

private:
    TimelineModel* model_;      ///< Model to modify (not owned)
    QString eventId_;           ///< Event ID to restore
    bool firstRun_;             ///< Track if this is the first execution
};
