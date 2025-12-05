// ArchivedEventsDialog.h

#pragma once
#include <QDialog>
#include <QString>
#include <QDate>

// Forward declarations
class TimelineModel;
class QUndoStack;
class QListWidgetItem;
struct TimelineEvent;

namespace Ui {
class ArchivedEventsDialog;
}

/**
 * @class ArchivedEventsDialog
 * @brief Dialog for viewing and managing archived events (Qt Designer version)
 *
 * Features:
 * - View all archived events in a list
 * - Display detailed information for selected event
 * - Restore events back to timeline (with undo support)
 * - Permanently delete archived events (no undo)
 * - Multi-selection support (Ctrl+Click, Shift+Click)
 * - Automatic refresh when model changes
 *
 * UI created in Qt Designer (ArchivedEventsDialog.ui)
 */
class ArchivedEventsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Construct archived events dialog
     * @param model Timeline model containing archived events
     * @param undoStack Undo stack for restore operations
     * @param parent Parent widget
     */
    explicit ArchivedEventsDialog(TimelineModel* model,
                                  QUndoStack* undoStack,
                                  QWidget* parent = nullptr);

    /**
     * @brief Destructor - cleans up UI
     */
    ~ArchivedEventsDialog();

private slots:
    /**
     * @brief Handle model changes that affect archived events
     */
    void onModelChanged();

    /**
     * @brief Handle event selection in list
     */
    void onEventSelected(QListWidgetItem* item);

    /**
     * @brief Handle restore button click
     */
    void onRestoreClicked();

    /**
     * @brief Handle permanent delete button click
     */
    void onPermanentDeleteClicked();

    /**
     * @brief Handle close button click
     */
    void onCloseClicked();

    /**
     * @brief Update button states based on selection
     */
    void updateButtonStates();

private:
    /**
     * @brief Setup signal/slot connections
     */
    void setupConnections();

    /**
     * @brief Refresh the list of archived events
     */
    void refreshList();

    /**
     * @brief Create a list item for an archived event
     * @param eventId ID of the event to create item for
     * @return Created list widget item (or nullptr on failure)
     */
    QListWidgetItem* createListItem(const QString& eventId);

    /**
     * @brief Display event details in the details panel
     * @param eventId ID of event to display
     */
    void displayEventDetails(const QString& eventId);

    /**
     * @brief Clear event details panel
     */
    void clearEventDetails();

    /**
     * @brief Get currently selected event IDs
     * @return List of selected event IDs
     */
    QStringList getSelectedEventIds() const;

    /**
     * @brief Format event type enum as display string
     * @param eventType Event type enum value
     * @return Human-readable type string
     */
    QString formatEventType(int eventType) const;

    /**
     * @brief Format date range as display string
     * @param startDate Event start date
     * @param endDate Event end date
     * @return Formatted date range string
     */
    QString formatDateRange(const QDate& startDate, const QDate& endDate) const;

    Ui::ArchivedEventsDialog* ui;   ///< UI class generated from .ui file
    TimelineModel* model_;          ///< Timeline model (not owned)
    QUndoStack* undoStack_;         ///< Undo stack (not owned)
};
