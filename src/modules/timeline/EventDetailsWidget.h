// EventDetailsWidget.h

#pragma once
#include "TimelineModel.h"
#include "AttachmentListWidget.h"
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QScrollArea>

/**
 * @class EventDetailsWidget
 * @brief Professional event details pane with inline editing capabilities
 *
 * Features:
 * - Inline editing for all event fields with visual feedback
 * - Auto-save on focus loss or explicit save/cancel
 * - Type-specific field visibility
 * - Integrated attachment viewer/manager
 * - Lock/Fixed state enforcement
 * - Collapsible sections for better UX
 * - Professional styling consistent with modern dev tools
 *
 * Architecture:
 * - Header: Event type badge + quick action buttons
 * - Common Fields: Inline editable (title, dates, priority, status, lane)
 * - Type-Specific Fields: Dynamically shown based on event type
 * - Description: Expandable text editor
 * - Attachments: Collapsible section with AttachmentListWidget
 */
class EventDetailsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EventDetailsWidget(TimelineModel* model, QWidget* parent = nullptr);
    ~EventDetailsWidget() override;

    void displayEvent(const QString& eventId);                  ///< @brief Display event details (view mode)
    void clear();                                               ///< @brief Clear all fields and reset to empty state
    QString currentEventId() const { return currentEventId_; }  ///< @brief Get currently displayed event ID

signals:
    void eventModified(const QString& eventId);                 ///< @brief Emitted when event is modified
    void editFullDialogRequested(const QString& eventId);       ///< @brief Emitted when user requests full edit dialog

private slots:
    // Field editing handlers
    void onFieldEditingStarted();
    void onFieldEditingFinished();
    void onSaveChanges();
    void onCancelChanges();
    void onOpenFullEditor();

    // Attachment handlers
    void onAttachmentsChanged();

    // Collapsible section handlers
    void onToggleDescription();
    void onToggleAttachments();
    void onToggleTypeSpecific();

    // Model change handlers
    void onEventUpdatedInModel(const QString& eventId);
    void onEventRemovedFromModel(const QString& eventId);

private:
    void setupUi();
    void setupConnections();
    void createHeaderSection();
    void createCommonFieldsSection();
    void createTypeSpecificSection();
    void createDescriptionSection();
    void createAttachmentsSection();

    void applyProfessionalStyling();
    void setEditMode(bool editing);
    void updateFieldsFromEvent(const TimelineEvent& event);
    void updateEventFromFields(TimelineEvent& event);
    void showTypeSpecificFields(TimelineEventType type);
    void clearTypeSpecificFields();

    bool validateFields();
    void highlightInvalidField(QWidget* field);
    void clearFieldHighlights();

    void updateLockStateUI();
    void updateHeaderBadge(TimelineEventType type);

    // Helper methods for type-specific fields
    QWidget* createMeetingFieldsWidget();
    QWidget* createActionFieldsWidget();
    QWidget* createTestEventFieldsWidget();
    QWidget* createReminderFieldsWidget();
    QWidget* createJiraTicketFieldsWidget();

    void populateMeetingFields(const TimelineEvent& event);
    void populateActionFields(const TimelineEvent& event);
    void populateTestEventFields(const TimelineEvent& event);
    void populateReminderFields(const TimelineEvent& event);
    void populateJiraTicketFields(const TimelineEvent& event);

    void extractMeetingFields(TimelineEvent& event);
    void extractActionFields(TimelineEvent& event);
    void extractTestEventFields(TimelineEvent& event);
    void extractReminderFields(TimelineEvent& event);
    void extractJiraTicketFields(TimelineEvent& event);

    // UI Components - Header
    QLabel* typeBadgeLabel_;
    QPushButton* fullEditButton_;
    QPushButton* saveButton_;
    QPushButton* cancelButton_;
    QFrame* headerFrame_;

    // UI Components - Common Fields
    QLineEdit* titleEdit_;
    QDateTimeEdit* startDateTimeEdit_;
    QDateTimeEdit* endDateTimeEdit_;
    QSpinBox* prioritySpinner_;
    QComboBox* statusCombo_;
    QSpinBox* laneSpinner_;
    QCheckBox* laneControlCheckbox_;
    QCheckBox* fixedCheckbox_;
    QCheckBox* lockedCheckbox_;
    QLabel* lockWarningLabel_;

    // UI Components - Description Section
    QGroupBox* descriptionGroup_;
    QTextEdit* descriptionEdit_;
    QPushButton* toggleDescriptionButton_;
    bool descriptionExpanded_;

    // UI Components - Type-Specific Section
    QGroupBox* typeSpecificGroup_;
    QWidget* typeSpecificContainer_;
    QVBoxLayout* typeSpecificLayout_;
    QPushButton* toggleTypeSpecificButton_;
    bool typeSpecificExpanded_;

    // Type-specific field widgets (created dynamically)
    // Meeting
    QLineEdit* locationEdit_;
    QTextEdit* participantsEdit_;

    // Action
    QDateTimeEdit* dueDateTimeEdit_;
    QComboBox* actionStatusCombo_;

    // Test Event
    QComboBox* testCategoryCombo_;
    QVBoxLayout* checklistLayout_;
    QMap<QString, QCheckBox*> checklistItems_;

    // Reminder
    QDateTimeEdit* reminderDateTimeEdit_;
    QComboBox* recurringRuleCombo_;

    // Jira Ticket
    QLineEdit* jiraKeyEdit_;
    QLineEdit* jiraSummaryEdit_;
    QComboBox* jiraTypeCombo_;
    QComboBox* jiraStatusCombo_;
    QDateTimeEdit* jiraDueDateTimeEdit_;

    // UI Components - Attachments Section
    QGroupBox* attachmentsGroup_;
    AttachmentListWidget* attachmentWidget_;
    QPushButton* toggleAttachmentsButton_;
    bool attachmentsExpanded_;

    // Layout components
    QScrollArea* scrollArea_;
    QWidget* contentWidget_;
    QVBoxLayout* mainLayout_;
    QFormLayout* commonFieldsLayout_;

    // State
    TimelineModel* model_;
    QString currentEventId_;
    bool isEditMode_;
    bool hasUnsavedChanges_;
    TimelineEvent originalEvent_;  // Backup for cancel functionality
};
