// TimelineSidePanel.h


#pragma once
#include "TimelineModel.h"
#include "TimelineSettings.h"
#include <QWidget>


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

    void refreshAllTabs();                                              ///< @brief Refresh all tabs from the model
    void adjustWidthToFitTabs();                                        ///< @brief Adjust side panel width to get all tabs in view
    QStringList getSelectedEventIds() const;                            ///< @brief Get list of selected event IDs from all tabs
    void setTimelineView(TimelineView* view) { view_ = view; }          ///< @brief Set the timeline view reference (for exports)

signals:
    void eventSelected(const QString& eventId);                         ///< @brief Emitted when user clicks on an event in the list
    void editEventRequested(const QString& eventId);                    ///< @brief Emitted when user requests to edit an event
    void deleteEventRequested(const QString& eventId);                  ///< @brief Emitted when user requests to delete an event
    void batchDeleteRequested(const QStringList& eventIds);             ///< @brief Emitted when user requests to delete multiple events
    void selectionChanged();                                            ///< @brief Emitted when selection changes in any list

public slots:
    void displayEventDetails(const QString& eventId);                   ///< @brief Display event details in the details group box

protected:
    void keyPressEvent(QKeyEvent* event) override;                      ///< @brief Handle key press events (Delete/Backspace for deletion)

private slots:
    //
    void onEventAdded(const QString& eventId);
    void onEventRemoved(const QString& eventId);
    void onEventUpdated(const QString& eventId);
    void onLanesRecalculated();

    //
    void onAllEventsItemClicked(QListWidgetItem* item);
    void onLookaheadItemClicked(QListWidgetItem* item);
    void onTodayItemClicked(QListWidgetItem* item);

    //
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

    // Common tab context menu actions
    void onRefreshTab();
    void onSortByDate();
    void onSortByPriority();
    void onSortByType();
    void onFilterByType();
    void onSelectAllEvents();
    void onCopyEventDetails();

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
    void toggleFilter(TimelineEventType type);                                                  ///< @brief Toggle a specific event type in the filter
    void sortEvents(QVector<TimelineEvent>& events) const;                                      ///< @brief Sort events according to current sort mode
    QVector<TimelineEvent> filterEvents(const QVector<TimelineEvent>& events) const;            ///< @brief Filter events according to active filter types
    QVector<TimelineEvent> applySortAndFilter(const QVector<TimelineEvent>& events) const;      ///< @brief Apply both sorting and filtering to events
    QString sortModeToString(TimelineSettings::SortMode mode) const;                            ///< @brief Get string representation of sort mode
    QString eventTypeToString(TimelineEventType type) const;                                    ///< @brief Get string representation of event type

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

    Ui::TimelineSidePanel* ui;                      ///< UI pointer
    TimelineModel* model_;                          ///< Timeline model (not owned)
    TimelineView* view_;                            ///< Timeline view for exports (not owned)
    TimelineSettings::SortMode currentSortMode_;
    QSet<TimelineEventType> activeFilterTypes_;

};
