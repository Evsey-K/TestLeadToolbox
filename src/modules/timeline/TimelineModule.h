// TimelineModule.h


#pragma once
#include <QWidget>


class TimelineView;
class TimelineModel;
class TimelineCoordinateMapper;
class TimelineSidePanel;
class QPushButton;


/**
 * @class TimelineModule
 * @brief Main container for the timeline feature
 *
 * Integrates:
 * - TimelineModel (data)
 * - TimelineView with Scene (visualization)
 * - TimelineSidePanel (event list)
 * - Add Event button and dialog
 */
class TimelineModule : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a TimelineModule instance
     * @param parent Optional parent widget
     */
    explicit TimelineModule(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~TimelineModule();

    /**
     * @brief Get access to the model (useful for testing or external integration)
     */
    TimelineModel* model() const { return model_; }

private slots:
    /**
     * @brief Handle Add Event button click
     */
    void onAddEventClicked();

    /**
     * @brief Handle event selection from side panel
     */
    void onEventSelectedInPanel(const QString& eventId);

private:
    void setupUi();
    void setupConnections();
    void createSampleData(); // Temporary: add some sample events

    TimelineModel* model_;
    TimelineCoordinateMapper* mapper_;
    TimelineView* view_;
    TimelineSidePanel* sidePanel_;
    QPushButton* addEventButton_;
};
