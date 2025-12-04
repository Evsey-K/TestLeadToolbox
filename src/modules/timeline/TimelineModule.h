// TimelineModule.h


#pragma once
#include <QWidget>
#include "DateRangeHighlight.h"


class TimelineView;
class TimelineModel;
class TimelineCoordinateMapper;
class TimelineSidePanel;
class AutoSaveManager;
class TimelineScrollAnimator;
class QPushButton;
class QToolBar;
class QLabel;


/**
 * @class TimelineModule
 * @brief Main container for the timeline feature with Phase 4 enhancements
 *
 * Integrates:
 * - TimelineModel (data)
 * - TimelineView with Scene (visualization)
 * - TimelineSidePanel (event list)
 * - Add Event button and dialog
 * - Version Settings button and dialog
 * - Auto-save/load timeline data (JSON)
 * - Export screenshot
 * - Export list view to CSV/PDF
 * - Drag resizing for multi-day events
 * - Smart scroll-to-date functionality
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
    TimelineModel* model() const
    {
        return model_;
    }

private slots:
    /**
     * @brief Handle Add Event button click
     */
    void onAddEventClicked();

    /**
     * @brief Handle Version Settings button click (NEW)
     */
    void onVersionSettingsClicked();

    /**
     * @brief Handle event selection from side panel
     */
    void onEventSelectedInPanel(const QString& eventId);

    /**
     * @brief Handle Save button click
     */
    void onSaveClicked();

    /**
     * @brief Handle Load button click
     */
    void onLoadClicked();

    /**
     * @brief Handle Export Screenshot action
     */
    void onExportScreenshot();

    /**
     * @brief Handle Export CSV action
     */
    void onExportCSV();

    /**
     * @brief Handle Export PDF action
     */
    void onExportPDF();

    /**
     * @brief Handle Scroll to Date action
     */
    void onScrollToDate();

    void onEditEventRequested(const QString& eventId);
    void onDeleteEventRequested(const QString& eventId);
    void onBatchDeleteRequested(const QStringList& eventIds);

private:
    void setupUi();
    void setupConnections();
    void setupAutoSave();
    void loadTimelineData();
    void createSampleData(); // Temporary: add some sample events

    QToolBar* createToolbar();                          ///< @brief Create toolbar with all actions

    bool confirmDeletion(const QString& eventId);       ///<
    bool deleteEvent(const QString& eventId);           ///<

    bool confirmBatchDeletion(const QStringList& eventIds);             ///< @brief Show confirmation dialog for batch event deletion
    bool deleteEventWithoutConfirmation(const QString& eventId);        ///< @brief Delete a single event without confirmation (for use after EditDialog)
    bool deleteBatchEvents(const QStringList& eventIds);                ///< @brief Delete multiple events with single confirmation dialog

    TimelineModel* model_;
    TimelineCoordinateMapper* mapper_;
    TimelineView* view_;
    TimelineSidePanel* sidePanel_;
    QPushButton* addEventButton_;
    QPushButton* versionSettingsButton_;    
    AutoSaveManager* autoSaveManager_;
    TimelineScrollAnimator* scrollAnimator_;
    QLabel* statusLabel_;
    QLabel* unsavedIndicator_;
};
