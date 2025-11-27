// TimelineSidePanel.h


#pragma once
#include <QWidget>


// Forward declaration
class QListWidget;
class QListWidgetItem;
class TimelineModel;


//
namespace Ui {
class TimelineSidePanel;
}


/**
 * @class TimelineSidePanel
 * @brief Side panel displaying all timeline events in a list
 *
 * Shows all events sorted by date with:
 * - Event title
 * - Date range
 * - Color indicator matching timeline
 *
 * Responds to model changes to keep list synchronized.
 */
class TimelineSidePanel : public QWidget
{
    Q_OBJECT


public:
    /**
     * @brief Construct the side panel
     * @param model Timeline model to display
     * @param parent Parent widget
     */
    explicit TimelineSidePanel(TimelineModel* model, QWidget* parent = nullptr);


    /**
     * @brief Destructor - cleans up UI
     */
    ~TimelineSidePanel();


    /**
     * @brief Refresh the entire list from the model
     */
    void refreshList();


signals:
    /**
     * @brief Emitted when user clicks on an event in the list
     */
    void eventSelected(const QString& eventId);


private slots:
    void onEventAdded(const QString& eventiD);
    void onEventRemoved(const QString& eventiD);
    void onEventUpdated(const QString& eventiD);
    void onItemClicked(QListWidgetItem* item);


private:
    void connectSignals();
    void addEventToList(const QString& eventId);
    void removeEventFromList(const QString& eventId);
    void updateEventInList(const QString& eventId);
    QListWidgetItem* findListItem(const QString& eventId) const;
    QString formatEventText(const QString& eventId) const;


    Ui::TimelineSidePanel *ui;  // Ui Form Pointer
    TimelineModel* model_;
};


