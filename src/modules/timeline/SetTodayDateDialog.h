// SetTodayDateDialog.h


#pragma once
#include <QDialog>
#include <QDate>


namespace Ui {
class SetTodayDateDialog;
}


/**
 * @class SetTodayDateDialog
 * @brief Dialog for selecting a custom date for the Today tab
 */
class SetTodayDateDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Construct dialog
     * @param currentDate Current date to show initially
     * @param minDate Minimum selectable date (version start)
     * @param maxDate Maximum selectable date (version end)
     * @param parent Parent widget
     */
    explicit SetTodayDateDialog(const QDate& currentDate,
                                const QDate& minDate,
                                const QDate& maxDate,
                                QWidget* parent = nullptr);
    ~SetTodayDateDialog();

    /**
     * @brief Get the selected date
     */
    QDate selectedDate() const;

private slots:
    /**
     * @brief Quick-set to current date
     */
    void onSetToCurrentClicked();

private:
    void setupConnections();
    void configureDateEdit();

    Ui::SetTodayDateDialog* ui;
    QDate minDate_;
    QDate maxDate_;
};
