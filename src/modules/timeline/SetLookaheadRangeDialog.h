// SetLookaheadRangeDialog.h


#pragma once
#include <QDialog>


namespace Ui {
class SetLookaheadRangeDialog;
}


/**
 * @class SetLookaheadRangeDialog
 * @brief Dialog for setting the lookahead range (number of days from current date)
 */
class SetLookaheadRangeDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Construct dialog
     * @param currentDays Current lookahead days setting
     * @param parent Parent widget
     */
    explicit SetLookaheadRangeDialog(int currentDays, QWidget* parent = nullptr);
    ~SetLookaheadRangeDialog();

    /**
     * @brief Get the selected number of days
     */
    int selectedDays() const;

private slots:
    /**
     * @brief Update example label when spinner value changes
     */
    void onDaysValueChanged(int days);

private:
    void setupConnections();

    Ui::SetLookaheadRangeDialog* ui;
};
