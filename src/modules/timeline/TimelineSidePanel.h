// TimelineSidePanel.h


#pragma once
#include "TimelineModel.h"
#include "TimelineSettings.h"
#include <QWidget>
#include <QSet>
#include <QMap>
#include <QHash>


// Forward declarations
class AttachmentListWidget;
class TimelineModel;
class TimelineView;
class QTabWidget;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QGroupBox;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QFormLayout;
class QStackedWidget;
class QComboBox;
class QSpinBox;
class QTextEdit;
class QLineEdit;
class QDateTimeEdit;
class QCheckBox;
class QWidget;
class QShortcut;


namespace Ui {
class TimelineSidePanel;
}


/**
 * @class TimelineSidePanel
 * @brief Side panel displaying all timeline events in a list + event inspector/details pane
 */
class TimelineSidePanel : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineSidePanel(TimelineModel* model, TimelineView* view = nullptr, QWidget* parent = nullptr);
    ~TimelineSidePanel();

    void refreshAllTabs();                                              ///< Refresh all tabs from the model
    void adjustWidthToFitTabs();                                        ///< Adjust side panel width to get all tabs in view
    QStringList getSelectedEventIds() const;                            ///< Selected event IDs from active tab
    void setTimelineView(TimelineView* view) { view_ = view; }          ///< Set timeline view reference (exports)

signals:
    void eventSelected(const QString& eventId);
    void editEventRequested(const QString& eventId);
    void deleteEventRequested(const QString& eventId);
    void batchDeleteRequested(const QStringList& eventIds);
    void selectionChanged();

public slots:
    void displayEventDetails(const QString& eventId);

protected:
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

    // Common tab context menu actions
    void onRefreshTab();
    void onSortByDate();
    void onSortByPriority();
    void onSortByType();
    void onFilterByType();
    void onSelectAllEvents();
    void onCopyEventDetails();

    // Inline details editor actions
    void onDetailsEditClicked();
    void onDetailsSaveClicked();
    void onDetailsCancelClicked();
    void onDetailsTypeChanged(int index);

    void onEventAttachmentsChanged(const QString& eventId);

    void onToggleDetailsSection();
    void onToggleTypeSpecificSection();
    void onToggleAttachmentsSection();

private:
    void connectSignals();

    void refreshAllEventsTab();
    void refreshLookaheadTab();
    void refreshTodayTab();

    void clearEventDetails();
    void clearTypeSpecificFields();

    bool validateEditFields();
    void highlightInvalidField(QWidget* field, const QString& message);
    void clearFieldHighlights();

    bool hasUnsavedChanges_ = false;

    QString buildMeetingDetails(const TimelineEvent& event);
    QString buildActionDetails(const TimelineEvent& event);
    QString buildTestEventDetails(const TimelineEvent& event);
    QString buildReminderDetails(const TimelineEvent& event);
    QString buildJiraTicketDetails(const TimelineEvent& event);
    QString buildGenericDetails(const TimelineEvent& event);

    void populateListWidget(QListWidget* listWidget, const QVector<TimelineEvent>& events);
    QListWidgetItem* createListItem(const TimelineEvent& event);
    QString formatEventText(const TimelineEvent& event) const;
    QString formatEventDateRange(const TimelineEvent& event) const;

    void setupListWidgetContextMenu(QListWidget* listWidget);
    void showListItemContextMenu(QListWidget* listWidget, const QPoint& pos);
    void setupTabBarContextMenu();

    void toggleFilter(TimelineEventType type);
    void sortEvents(QVector<TimelineEvent>& events) const;
    QVector<TimelineEvent> filterEvents(const QVector<TimelineEvent>& events) const;
    QVector<TimelineEvent> applySortAndFilter(const QVector<TimelineEvent>& events) const;
    QString sortModeToString(TimelineSettings::SortMode mode) const;
    QString eventTypeToString(TimelineEventType type) const;

    void enableMultiSelection(QListWidget* listWidget);
    QStringList getSelectedEventIds(QListWidget* listWidget) const;
    void focusNextItem(QListWidget* listWidget, int deletedRow);

    QVector<TimelineEvent> getEventsFromTab(int tabIndex) const;
    bool exportEvents(const QVector<TimelineEvent>& events, const QString& format, const QString& tabName);

    void updateTodayTabLabel();
    void updateLookaheadTabLabel();
    void updateAllEventsTabLabel();

    // Attachments
    void setupAttachmentsSection();
    void refreshAttachmentsForCurrentEvent();

    // Inline Editor
    void setupInlineEditor();
    void enterEditMode(const TimelineEvent& event);
    void exitEditMode(bool keepSelection);
    TimelineEvent buildEditedEvent(const TimelineEvent& base) const;
    void populateTypeSpecificEditors(TimelineEventType type, const TimelineEvent& event);
    void collectTypeSpecificEdits(TimelineEvent& event) const;
    void rebuildPreparationChecklistUI(const QMap<QString, bool>& checklist);
    void updateEventDetails(const TimelineEvent& event);

    // Collapsible section states
    bool detailsExpanded_ = true;
    bool typeSpecificExpanded_ = true;
    bool attachmentsExpanded_ = true;

    void updateDetailsPaneGeometry();

    QList<QMetaObject::Connection> changeTrackingConnections_;

private:
    Ui::TimelineSidePanel* ui;
    TimelineModel* model_;
    TimelineView* view_;

    TimelineSettings::SortMode currentSortMode_;
    QSet<TimelineEventType> activeFilterTypes_;

    QString currentEventId_;

    // Attachments (created programmatically)
    QWidget* attachmentsSection_ = nullptr;

    // Attachments header UI
    QWidget* attachmentsHeader_ = nullptr;
    QLabel* attachmentsCountBadge_ = nullptr;

    QLabel* attachmentsEmptyStateLabel_ = nullptr;

    AttachmentListWidget* attachmentsWidget_ = nullptr;

    // Inline Details Editor State
    bool isEditingDetails_ = false;
    TimelineEvent detailsOriginalEvent_;
    QWidget* editPanel_ = nullptr;

    // Common fields
    QComboBox* typeCombo_ = nullptr;
    QLineEdit* titleEdit_ = nullptr;
    QSpinBox* prioritySpin_ = nullptr;
    QDateTimeEdit* startDateTimeEdit_ = nullptr;
    QDateTimeEdit* endDateTimeEdit_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;

    // Type-specific stack
    QStackedWidget* typeSpecificStack_ = nullptr;

    // Meeting
    QLineEdit* meetingLocationEdit_ = nullptr;
    QLineEdit* meetingParticipantsEdit_ = nullptr;

    // Action
    QComboBox* actionStatusCombo_ = nullptr;
    QDateTimeEdit* actionDueDateTimeEdit_ = nullptr;

    // Test Event
    QComboBox* testCategoryCombo_ = nullptr;
    QWidget* prepChecklistWidget_ = nullptr;
    QVBoxLayout* prepChecklistLayout_ = nullptr;
    QMap<QString, QCheckBox*> prepChecklistChecks_;

    // Reminder
    QDateTimeEdit* reminderDateTimeEdit_ = nullptr;
    QComboBox* reminderRecurringCombo_ = nullptr;

    // Jira
    QLineEdit* jiraKeyEdit_ = nullptr;
    QLineEdit* jiraSummaryEdit_ = nullptr;
    QComboBox* jiraTypeCombo_ = nullptr;
    QComboBox* jiraStatusCombo_ = nullptr;

    // Advanced
    QCheckBox* laneControlEnabledCheck_ = nullptr;
    QSpinBox* manualLaneSpin_ = nullptr;
    QCheckBox* fixedCheck_ = nullptr;
    QCheckBox* lockedCheck_ = nullptr;

    // Collapsible section toggle buttons
    QPushButton* toggleDetailsButton_ = nullptr;
    QPushButton* toggleTypeSpecificButton_ = nullptr;
    QPushButton* toggleAttachmentsButton_ = nullptr;

    // Shortcuts (work even when child widgets have focus)
    QShortcut* saveShortcut_ = nullptr;
    QShortcut* cancelShortcut_ = nullptr;

    // Style preservation (avoid nuking .ui / theme styles)
    QString originalEditPanelStyle_;
    QString originalDetailsGroupStyle_;
    QHash<QWidget*, QString> originalFieldStyles_;
};
