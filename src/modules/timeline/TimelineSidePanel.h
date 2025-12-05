// TimelineSidePanel.h


#pragma once
#include <QWidget>
#include "TimelineModel.h"


// Forward declaration
class QTabWidget;
class QListWidget;
class QListWidgetItem;
class TimelineModel;


//
namespace Ui {
class TimelineSidePanel;
}


/**
 * @class TimelineSidePanel
 * @brief Side panel displaying all timeline events in a list
 *
 * - Tab 1: All Events
 * - Tab 2: Lookahead (next 2 weeks)
 * - Tab 3: Today's Events
 *
 * Shows all events sorted by date with:
 * - Event title
 * - Date range
 * - Color indicator matching timeline
 * - Multi-selection support (Ctrl+Click, Shift+Click) - TIER 2
 * - Focus management after deletion - TIER 2
 *
 * All tabs automatically update when model changes.
 */
class TimelineSidePanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Construct the side panel
     * @param model Timeline model to display
     * @param parent Parent widget
     */
    explicit TimelineSidePanel(TimelineModel* model, QWidget* parent = nullptr);

    /**
     * @brief Destructor - cleans up UI
     */
    ~TimelineSidePanel();

    /**
     * @brief Refresh all tabs from the model
     */
    void refreshAllTabs();

    /**
     * @brief Adjust side panel width to get all tabs in view
     */
    void adjustWidthToFitTabs();

    /**
     * @brief Get list of selected event IDs from all tabs (TIER 2)
     */
    QStringList getSelectedEventIds() const;

signals:
    /**
     * @brief Emitted when user clicks on an event in the list
     */
    void eventSelected(const QString& eventId);

    /**
     * @brief Emitted when user requests to edit an event
     */
    void editEventRequested(const QString& eventId);

    /**
     * @brief Emitted when user requests to delete an event
     */
    void deleteEventRequested(const QString& eventId);

    /**
     * @brief Emitted when selection changes in any list (TIER 2)
     */
    void selectionChanged();

public slots:
    /**
     * @brief Display event details in the details group box
     * @param eventId Event ID to display
     */
    void displayEventDetails(const QString& eventId);

private slots:
    void onEventAdded(const QString& eventiD);
    void onEventRemoved(const QString& eventiD);
    void onEventUpdated(const QString& eventiD);
    void onLanesRecalculated();

    void onAllEventsItemClicked(QListWidgetItem* item);
    void onLookaheadItemClicked(QListWidgetItem* item);
    void onTodayItemClicked(QListWidgetItem* item);

    void onListContextMenuRequested(const QPoint& pos);

    /**
     * @brief Handle selection change in any list widget (TIER 2)
     */
    void onListSelectionChanged();

private:
    void connectSignals();

    // Tab refresh methods
    void refreshAllEventsTab();
    void refreshLookaheadTab();
    void refreshTodayTab();

    // Event Details methods
    void clearEventDetails();
    void updateEventDetails(const TimelineEvent& event);

    // Helper methods
    void populateListWidget(QListWidget* listWidget, const QVector<TimelineEvent>& events);
    QListWidgetItem* createListItem(const TimelineEvent& event);
    QString formatEventText(const TimelineEvent& event) const;
    QString formatEventDateRange(const TimelineEvent& event) const;

    // Context Menu methods
    void setupListWidgetContextMenu(QListWidget* listWidget);
    void showListItemContextMenu(QListWidget* listWidget, const QPoint& pos);

    /**
     * @brief Enable multi-selection on a list widget (TIER 2)
     */
    void enableMultiSelection(QListWidget* listWidget);

    /**
     * @brief Get selected event IDs from a specific list widget (TIER 2)
     */
    QStringList getSelectedEventIds(QListWidget* listWidget) const;

    /**
     * @brief Focus management: select next/previous item after deletion (TIER 2)
     */
    void focusNextItem(QListWidget* listWidget, int deletedRow);

    Ui::TimelineSidePanel* ui;      ///< UI pointer

    TimelineModel* model_;
};
