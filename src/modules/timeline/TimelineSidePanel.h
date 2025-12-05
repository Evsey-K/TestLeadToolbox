// TimelineSidePanel.h

#pragma once
#include <QWidget>
#include "TimelineModel.h"

// Forward declarations
class QTabWidget;
class QListWidget;
class QListWidgetItem;
class TimelineModel;
class TimelineView;

namespace Ui {
class TimelineSidePanel;
}

/**
 * @class TimelineSidePanel
 * @brief Side panel displaying all timeline events in a list
 *
 * Enhanced with tab context menus for:
 * - Today Tab: Set to current date, set to specific date, export
 * - Lookahead Tab: Set lookahead range, export
 * - All Events Tab: Export
 *
 * Shows all events sorted by date with:
 * - Event title, date range, color indicator
 * - Multi-selection support (Ctrl+Click, Shift+Click)
 * - Focus management after deletion
 * - Delete key support
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
     * @param view Timeline view (needed for exports)
     * @param parent Parent widget
     */
    explicit TimelineSidePanel(TimelineModel* model, TimelineView* view = nullptr, QWidget* parent = nullptr);

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
     * @brief Get list of selected event IDs from all tabs
     */
    QStringList getSelectedEventIds() const;

    /**
     * @brief Set the timeline view reference (for exports)
     */
    void setTimelineView(TimelineView* view) { view_ = view; }

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
     * @brief Emitted when user requests to delete multiple events
     */
    void batchDeleteRequested(const QStringList& eventIds);

    /**
     * @brief Emitted when selection changes in any list
     */
    void selectionChanged();

public slots:
    /**
     * @brief Display event details in the details group box
     * @param eventId Event ID to display
     */
    void displayEventDetails(const QString& eventId);

protected:
    /**
     * @brief Handle key press events (Delete/Backspace for deletion)
     */
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onEventAdded(const QString& eventId);
    void onEventRemoved(const QString& eventId);
    void onEventUpdated(const QString& eventId);
    void onLanesRecalculated();

    void onAllEventsItemClicked(QListWidgetItem* item);
    void onLookaheadItemClicked(QListWidgetItem* item);
    void onTodayItemClicked(QListWidgetItem* item);

    void onListContextMenuRequested(const QPoint& pos);
    void onListSelectionChanged();

    // Tab context menu handlers
    void onTabBarContextMenuRequested(const QPoint& pos);
    void showTodayTabContextMenu(const QPoint& globalPos);
    void showLookaheadTabContextMenu(const QPoint& globalPos);
    void showAllEventsTabContextMenu(const QPoint& globalPos);

    // Today tab actions
    void onSetToCurrentDate();
    void onSetToSpecificDate();
    void onExportTodayTab(const QString& format);

    // Lookahead tab actions
    void onSetLookaheadRange();
    void onExportLookaheadTab(const QString& format);

    // All events tab actions
    void onExportAllEventsTab(const QString& format);

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
    void setupTabBarContextMenu();

    // Multi-selection support
    void enableMultiSelection(QListWidget* listWidget);
    QStringList getSelectedEventIds(QListWidget* listWidget) const;
    void focusNextItem(QListWidget* listWidget, int deletedRow);

    // Export helper methods
    QVector<TimelineEvent> getEventsFromTab(int tabIndex) const;
    bool exportEvents(const QVector<TimelineEvent>& events, const QString& format, const QString& tabName);

    // Tab label update methods
    void updateTodayTabLabel();
    void updateLookaheadTabLabel();
    void updateAllEventsTabLabel();

    Ui::TimelineSidePanel* ui;      ///< UI pointer
    TimelineModel* model_;          ///< Timeline model (not owned)
    TimelineView* view_;            ///< Timeline view for exports (not owned)
};
