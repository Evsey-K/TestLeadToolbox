// EditEventDialog.h


#pragma once
#include "AttachmentListWidget.h"
#include "TimelineModel.h"
#include <QDialog>
#include <QDate>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QStackedWidget>
#include <QDateEdit>
#include <QTimeEdit>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>


/**
 * @class EditEventDialog
 * @brief Dynamic dialog for editing timeline events with type-specific fields
 *
 * Features:
 * - Pre-populated with existing event data
 * - Type-specific fields shown dynamically based on event type
 * - Validation rules enforced per event type
 * - Delete button for removing events
 * - Clean MVC-style architecture
 */
class EditEventDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditEventDialog(const QString& eventId,
                             TimelineModel* model,
                             const QDate& versionStart,
                             const QDate& versionEnd,
                             QWidget* parent = nullptr);
    ~EditEventDialog();

    TimelineEvent getEvent() const;                                 ///< @brief Get the updated event from user inputs
    bool isDeleted() const { return deleted_; }                     ///< @brief Check if event was deleted

signals:
    void deleteRequested();                                         ///< @brief Emitted when user requests to delete the event

private slots:
    void onTypeChanged(int index);                                  ///< @brief Handle event type selection change
    void validateAndAccept();                                       ///< @brief Validate and accept the dialog
    void onDeleteClicked();                                         ///< @brief Handle delete button click

    void onMeetingAllDayChanged(bool checked);
    void onActionAllDayChanged(bool checked);
    void onTestAllDayChanged(bool checked);
    void onReminderAllDayChanged(bool checked);
    void onJiraAllDayChanged(bool checked);

    void onAttachmentsChanged();

private:
    void setupUi();                                                 ///< @brief Setup UI components and connections
    void createFieldGroups();                                       ///< @brief Create field group widgets
    void showFieldsForType(TimelineEventType type);                 ///< @brief Show appropriate field group for event type
    void populateFromEvent(const TimelineEvent& event);             ///< @brief Populate dialog fields from existing event
    bool validateCommonFields();                                    ///< @brief Validate common fields
    bool validateTypeSpecificFields();                              ///< @brief Validate type-specific fields
    void populateCommonFields(TimelineEvent& event) const;          /// <@brief Populate event from common fields
    void populateTypeSpecificFields(TimelineEvent& event) const;    ///< @brief Populate event from type-specific fields

    // Helper methods for creating field groups
    QWidget* createMeetingFields();
    QWidget* createActionFields();
    QWidget* createTestEventFields();
    QWidget* createReminderFields();
    QWidget* createJiraTicketFields();

    // UI Components - Common
    QComboBox* typeCombo_;
    QLineEdit* titleEdit_;
    QSpinBox* prioritySpinner_;
    QTextEdit* descriptionEdit_;
    QStackedWidget* fieldStack_;
    QPushButton* deleteButton_;

    // UI Components - Meeting
    QDateEdit* meetingStartDate_;
    QTimeEdit* meetingStartTime_;
    QDateEdit* meetingEndDate_;
    QTimeEdit* meetingEndTime_;
    QCheckBox* meetingAllDayCheckbox_;
    QLineEdit* locationEdit_;
    QTextEdit* participantsEdit_;

    // UI Components - Action
    QDateEdit* actionStartDate_;
    QTimeEdit* actionStartTime_;
    QDateEdit* actionDueDate_;
    QTimeEdit* actionDueTime_;
    QCheckBox* actionAllDayCheckbox_;
    QComboBox* statusCombo_;

    // UI Components - Test Event
    QDateEdit* testStartDate_;
    QTimeEdit* testStartTime_;
    QDateEdit* testEndDate_;
    QTimeEdit* testEndTime_;
    QCheckBox* testAllDayCheckbox_;
    QComboBox* testCategoryCombo_;
    QMap<QString, QCheckBox*> checklistItems_;

    // UI Components - Reminder
    QDateEdit* reminderDate_;
    QTimeEdit* reminderTime_;
    QDateEdit* reminderEndDate_;
    QTimeEdit* reminderEndTime_;
    QCheckBox* reminderAllDayCheckbox_;
    QComboBox* recurringRuleCombo_;

    // UI Components - Jira Ticket
    QLineEdit* jiraKeyEdit_;
    QLineEdit* jiraSummaryEdit_;
    QComboBox* jiraTypeCombo_;
    QComboBox* jiraStatusCombo_;
    QDateEdit* jiraStartDate_;
    QTimeEdit* jiraStartTime_;
    QDateEdit* jiraDueDate_;
    QTimeEdit* jiraDueTime_;
    QCheckBox* jiraAllDayCheckbox_;

    // Configuration
    QString eventId_;
    TimelineModel* model_;
    QDate versionStart_;
    QDate versionEnd_;
    TimelineEventType originalType_;

    // Lane Control Fields
    QCheckBox* laneControlCheckbox_;
    QSpinBox* manualLaneSpinner_;
    QLabel* laneControlWarningLabel_;

    // Attachment widget
    AttachmentListWidget* attachmentWidget_ = nullptr;

    // State
    bool deleted_ = false;
};
