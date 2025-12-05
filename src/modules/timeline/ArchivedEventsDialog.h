// ArchivedEventsDialog.h
// Phase 5 Tier 3: Archived Events Management (MANUAL UI VERSION)


#pragma once
#include <QDialog>
#include <QStringList>


class TimelineModel;
class QUndoStack;
class QTableWidget;
class QPushButton;
class QLabel;


/**
 * @class ArchivedEventsDialog
 * @brief Dialog for viewing and managing archived (soft-deleted) events
 *
 * This is a placeholder implementation until TimelineModel supports soft-delete.
 * For now, it informs users to use Ctrl+Z for undo functionality.
 */
class ArchivedEventsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Construct archived events dialog
     * @param model Timeline model
     * @param undoStack Undo stack for restoration commands
     * @param parent Parent widget
     */
    explicit ArchivedEventsDialog(TimelineModel* model,
                                  QUndoStack* undoStack,
                                  QWidget* parent = nullptr);

    ~ArchivedEventsDialog() = default;

private:
    /**
     * @brief Setup UI components manually (no Qt Designer)
     */
    void setupUi();

    TimelineModel* model_;
    QUndoStack* undoStack_;

    // UI components
    QLabel* infoLabel_;
    QPushButton* closeButton_;
};
