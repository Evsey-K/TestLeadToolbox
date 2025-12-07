// EditEventDialog.h


#pragma once
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
#include <QPushButton>
#include <QLabel>


/**
 * @class EditEventDialog
 * @brief Dynamic dialog for editing existing timeline events with type-specific fields
 *
 * Features:
 * - Pre-populates all fields from existing event
 * - Allows changing event type (with warning if type-specific data will be lost)
 * - Delete button for removing the event
 * - Same validation as AddEventDialog
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

    /**
     * @brief Get the updated event from user inputs
     * @return TimelineEvent with all fields populated based on type
     */
    TimelineEvent getEvent() const;

signals:
    /**
     * @brief Emitted when user requests to delete the event
     */
    void deleteRequested();

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
     * @brief Handle delete button click
     */
    void onDeleteClicked();

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
     * @brief Populate dialog fields from existing event
     */
    void populateFromEvent(const TimelineEvent& event);

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
    QPushButton* deleteButton_;

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
    QString eventId_;
    TimelineModel* model_;
    QDate versionStart_;
    QDate versionEnd_;
    TimelineEventType originalType_;

    // =Lane Control Fields
    QCheckBox* laneControlCheckbox_;
    QSpinBox* manualLaneSpinner_;
    QLabel* laneControlWarningLabel_;
};
