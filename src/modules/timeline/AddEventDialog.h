// AddEventDialog.h - Updated with Dynamic Field Support

#pragma once
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
#include "TimelineModel.h"

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

    /**
     * @brief Get the constructed event from user inputs
     * @return TimelineEvent with all fields populated based on type
     */
    TimelineEvent getEvent() const;

private slots:
    /**
     * @brief Handle event type selection change
     */
    void onTypeChanged(int index);

    /**
     * @brief Validate and accept the dialog
     */
    void validateAndAccept();

    /**
     * @brief Update end date minimum when start date changes
     */
    void onStartDateChanged(const QDate& date);

private:
    /**
     * @brief Setup UI components and connections
     */
    void setupUi();

    /**
     * @brief Create field group widgets
     */
    void createFieldGroups();

    /**
     * @brief Show appropriate field group for event type
     */
    void showFieldsForType(TimelineEventType type);

    /**
     * @brief Validate common fields
     */
    bool validateCommonFields();

    /**
     * @brief Validate type-specific fields
     */
    bool validateTypeSpecificFields();

    /**
     * @brief Populate event from common fields
     */
    void populateCommonFields(TimelineEvent& event) const;

    /**
     * @brief Populate event from type-specific fields
     */
    void populateTypeSpecificFields(TimelineEvent& event) const;

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
    QLineEdit* locationEdit_;
    QTextEdit* participantsEdit_;

    // UI Components - Action
    QDateEdit* actionStartDate_;
    QTimeEdit* actionStartTime_;
    QDateTimeEdit* actionDueDateTime_;
    QComboBox* statusCombo_;

    // UI Components - Test Event
    QDateEdit* testStartDate_;
    QDateEdit* testEndDate_;
    QComboBox* testCategoryCombo_;
    QMap<QString, QCheckBox*> checklistItems_;

    // UI Components - Reminder
    QDateTimeEdit* reminderDateTime_;
    QComboBox* recurringRuleCombo_;

    // UI Components - Jira Ticket
    QLineEdit* jiraKeyEdit_;
    QLineEdit* jiraSummaryEdit_;
    QComboBox* jiraTypeCombo_;
    QComboBox* jiraStatusCombo_;
    QDateEdit* jiraStartDate_;
    QDateEdit* jiraDueDate_;

    // Configuration
    QDate versionStart_;
    QDate versionEnd_;
};
