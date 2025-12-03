// EditEventDialog.h


#pragma once
#include <QDialog>
#include "TimelineModel.h"


// Forward declaration of UI class
namespace Ui {
class EditEventDialog;
}


/**
 * @class EditEventDialog
 * @brief Dialog for editing existing timeline events with delete capability
 */
class EditEventDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Custom dialog result code for delete requests
     */
    static constexpr int DeleteRequested = QDialog::Accepted + 1;

    /**
     * @brief Construct edit dialog for existing event
     */
    explicit EditEventDialog(const QString& eventId,
                             TimelineModel* model,
                             const QDate& versionStart,
                             const QDate& versionEnd,
                             QWidget* parent = nullptr);

    ~EditEventDialog();

    /**
     * @brief Get updated event data from dialog
     */
    TimelineEvent getEvent() const;

    /**
     * @brief Get the event ID being edited
     */
    QString eventId() const { return eventId_; }

private slots:
    void validateAndAccept();
    void onDeleteClicked();
    void onStartDateChanged(const QDate& date);

private:
    void populateFromEvent(const TimelineEvent& event);

    Ui::EditEventDialog* ui;
    QString eventId_;
    TimelineModel* model_;
    QDate versionStart_;
    QDate versionEnd_;
};
