// ConfirmationDialog.h


#pragma once
#include <QDate>
#include <QString>
#include <QWidget>
#include <QMessageBox>
#include <QCheckBox>
#include "TimelineSettings.h"


/**
 * @class ConfirmationDialog
 * @brief Utility for showing deletion confirmation dialogs with "don't ask again" option
 */
class ConfirmationDialog
{
public:
    static bool confirmDeletion(QWidget* parent,
                                const QString& eventTitle,
                                const QDate& eventStartDate,
                                const QDate& eventEndDate,
                                bool softDelete = true);

    static bool confirmBatchDeletion(QWidget* parent,
                                     const QStringList& eventTitles,
                                     bool softDelete = true);

    static bool shouldShowConfirmation();

private:
    static QMessageBox::StandardButton showConfirmationWithCheckbox(
        QWidget* parent,
        const QString& title,
        const QString& message,
        bool* dontAskAgain);
};


inline bool ConfirmationDialog::shouldShowConfirmation()
{
    return TimelineSettings::instance().showDeleteConfirmation();
}


inline bool ConfirmationDialog::confirmDeletion(QWidget* parent,
                                                const QString& eventTitle,
                                                const QDate& eventStartDate,
                                                const QDate& eventEndDate,
                                                bool softDelete)
{
    if (!shouldShowConfirmation())
    {
        return true;
    }

    QString action = softDelete ? "archive" : "delete";
    QString message = QString(
                          "Are you sure you want to %1 this event?\n\n"
                          "Title: %2\n"
                          "Dates: %3 to %4\n\n"
                          "%5"
                          ).arg(action)
                          .arg(eventTitle)
                          .arg(eventStartDate.toString("yyyy-MM-dd"))
                          .arg(eventEndDate.toString("yyyy-MM-dd"))
                          .arg(softDelete ? "You can restore it later from the Archive."
                                          : "This action cannot be undone!");

    bool dontAskAgain = false;

    auto result = showConfirmationWithCheckbox(
        parent,
        "Confirm Deletion",
        message,
        &dontAskAgain
        );

    if (dontAskAgain)
    {
        TimelineSettings::instance().setShowDeleteConfirmation(false);
    }

    return (result == QMessageBox::Yes);
}

inline bool ConfirmationDialog::confirmBatchDeletion(QWidget* parent,
                                                     const QStringList& eventTitles,
                                                     bool softDelete)
{
    if (!shouldShowConfirmation())
    {
        return true;
    }

    QString action = softDelete ? "archive" : "delete";
    QString message;

    if (eventTitles.size() == 1)
    {
        message = QString(
                      "Are you sure you want to %1 this event?\n\n"
                      "• %2\n\n"
                      "%3"
                      ).arg(action)
                      .arg(eventTitles.first())
                      .arg(softDelete ? "You can restore it later from the Archive."
                                      : "This action cannot be undone!");
    }
    else
    {
        QStringList displayTitles = eventTitles.mid(0, 10);
        QString eventList = displayTitles.join("\n• ");

        if (eventTitles.size() > 10)
        {
            eventList += QString("\n... and %1 more").arg(eventTitles.size() - 10);
        }

        message = QString(
                      "Are you sure you want to %1 these %2 events?\n\n"
                      "• %3\n\n"
                      "%4"
                      ).arg(action)
                      .arg(eventTitles.size())
                      .arg(eventList)
                      .arg(softDelete ? "You can restore them later from the Archive."
                                      : "This action cannot be undone!");
    }

    bool dontAskAgain = false;

    auto result = showConfirmationWithCheckbox(
        parent,
        "Confirm Deletion",
        message,
        &dontAskAgain
        );

    if (dontAskAgain)
    {
        TimelineSettings::instance().setShowDeleteConfirmation(false);
    }

    return (result == QMessageBox::Yes);
}

inline QMessageBox::StandardButton ConfirmationDialog::showConfirmationWithCheckbox(
    QWidget* parent,
    const QString& title,
    const QString& message,
    bool* dontAskAgain)
{
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    QCheckBox* checkBox = new QCheckBox("Don't ask me again");
    msgBox.setCheckBox(checkBox);

    auto result = static_cast<QMessageBox::StandardButton>(msgBox.exec());

    if (dontAskAgain)
    {
        *dontAskAgain = checkBox->isChecked();
    }

    return result;
}
