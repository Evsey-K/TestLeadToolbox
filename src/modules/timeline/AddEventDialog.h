// AddEventDialog.h


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


/**
 * @class AddEventDialog
 * @brief Dynamic dialog for creating timeline events with type-specific fields
 *
 * Features:
 * - Common fields always visible (Type, Title, Priority, Description)
 * - Type-specific fields shown dynamically based on selected type
 * - Validation rules enforced per event type
 * - Sensible default values pre-populated
 * - Clean MVC-style architecture
 */
class AddEventDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEventDialog(const QDate& versionStart,
                            const QDate& versionEnd,
                            QWidget* parent = nullptr);
    ~AddEventDialog();

    TimelineEvent getEvent() const;                                 ///< @brief Get the constructed event from user inputs

private slots:
    void onTypeChanged(int index);                                  ///< @brief Handle event type selection change
    void validateAndAccept();                                       ///< @brief Validate and accept the dialog
    void onMeetingAllDayChanged(bool checked);                      ///< @brief Handle meeting all-day checkbox
    void onActionAllDayChanged(bool checked);                       ///< @brief Handle action all-day checkbox
    void onTestAllDayChanged(bool checked);                         ///< @brief Handle test event all-day checkbox
    void onReminderAllDayChanged(bool checked);                     ///< @brief Handle reminder all-day checkbox
    void onJiraAllDayChanged(bool checked);                         ///< @brief Handle Jira all-day checkbox

private:
    void setupUi();                                                 ///< @brief Setup UI components and connections
    void createFieldGroups();                                       ///< @brief Create field group widgets
    void showFieldsForType(TimelineEventType type);                 ///< @brief Show appropriate field group for event type
    bool validateCommonFields();                                    ///< @brief Validate common fields
    bool validateTypeSpecificFields();                              ///< @brief Validate type-specific fields
    void populateCommonFields(TimelineEvent& event) const;          ///< @brief Populate event from common fields
    void populateTypeSpecificFields(TimelineEvent& event) const;    ///< @brief Populate event from type-specific fields

    // Attachment widget (temp ID for pre-adding attachments)
    AttachmentListWidget* attachmentWidget_ = nullptr;
    QString tempEventId_;

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
    QDate versionStart_;
    QDate versionEnd_;

    // Lane Control Fields
    QCheckBox* laneControlCheckbox_;
    QSpinBox* manualLaneSpinner_;
    QLabel* laneControlWarningLabel_;
};
