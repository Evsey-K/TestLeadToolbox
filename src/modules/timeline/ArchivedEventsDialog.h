// ArchivedEventsDialog.h


#pragma once
#include <QDialog>
#include <qlistwidget.h>
#include "TimelineModel.h"


class QUndoStack;


namespace Ui {
class ArchivedEventsDialog;
}


/**
 * @class ArchivedEventsDialog
 * @brief Dialog for viewing and managing archived (soft-deleted) events
 *
 * Features:
 * - List all archived events with color indicators
 * - Display detailed event information
 * - Restore events back to timeline (with undo support)
 * - Permanently delete archived events
 * - Multi-selection support for batch operations
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

    ~ArchivedEventsDialog();

private slots:
    /**
     * @brief Handle restore button click
     */
    void onRestoreClicked();

    /**
     * @brief Handle permanent delete button click
     */
    void onPermanentDeleteClicked();

    /**
     * @brief Handle selection change in list
     */
    void onSelectionChanged();

    /**
     * @brief Handle item double-click (restore event)
     */
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    /**
     * @brief Setup UI components and connections
     */
    void setupUi();

    /**
     * @brief Load archived events from model into list
     */
    void loadArchivedEvents();

    /**
     * @brief Display event details in detail panel
     */
    void displayEventDetails(const QString& eventId);

    /**
     * @brief Clear event details panel
     */
    void clearEventDetails();

    /**
     * @brief Create list item for archived event
     */
    QListWidgetItem* createListItem(const TimelineEvent& event);

    /**
     * @brief Format event text for list display
     */
    QString formatEventText(const TimelineEvent& event) const;

    /**
     * @brief Get selected event IDs from list
     */
    QStringList getSelectedEventIds() const;

    /**
     * @brief Update button states based on selection
     */
    void updateButtonStates();

    /**
     * @brief Convert event type to display string
     */
    QString eventTypeToString(TimelineEventType type) const;

    Ui::ArchivedEventsDialog* ui_;
    TimelineModel* model_;
    QUndoStack* undoStack_;
};
