// TimelineModule.h


#pragma once
#include <QWidget>
#include <qundostack.h>
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
class QAction;
class TimelineLegend;
class QCheckBox;
class QSplitter;


/**
 * @class TimelineModule
 * @brief Main container for the timeline feature with Phase 5 Tier 2 enhancements
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
 * - Toolbar delete button (context-sensitive)
 * - Multi-selection support
 * - Focus management after deletion
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

    TimelineModel* model() const { return model_; }         ///< @brief Get access to the model (useful for testing or external integration)

    QUndoStack* undoStack() const { return undoStack_; }    ///<

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onAddEventClicked();                                   ///< @brief Handle Add Event button click
    void onVersionSettingsClicked();                            ///< @brief Handle Version Settings button click
    void onSaveClicked();                                       ///< @brief Handle Save button click
    void onSaveAsClicked();                                     ///< @brief Handle Save As button click - always prompts for new location
    void onLoadClicked();                                       ///< @brief Handle Load button click

    void onEventSelectedInPanel(const QString& eventId);        ///< @brief Handle event selection from side panel

    void onExportScreenshot();                                  ///< @brief Handle Export Screenshot action
    void onExportCSV();                                         ///< @brief Handle Export CSV action
    void onExportPDF();                                         ///< @brief Handle Export PDF action

    void onScrollToDate();                                      ///< @brief Handle Scroll to Date action

    void onEditEventRequested(const QString& eventId);          ///< @brief Handle edit event request from scene or side panel
    void onDeleteEventRequested(const QString& eventId);        ///< @brief Handle delete event request from scene or side panel
    void onBatchDeleteRequested(const QStringList& eventIds);   ///< @brief Handle batch delete request from scene
    void onDeleteActionTriggered();                             ///< @brief Handle toolbar delete button click

    void onShowArchivedEvents();

    void updateDeleteActionState();                             ///< @brief Update delete button enabled state based on selection

    void onToggleSidePanelClicked();                            ///< @brief Handle side panel toggle button click

    void onLegendToggled(bool checked);

    void onSplitterMoved(int pos, int index);

private:
    void setupUi();
    void setupConnections();
    void setupAutoSave();
    void loadTimelineData();
    void setupUndoStack();

    QToolBar* createToolbar();                                          ///< @brief Create toolbar with all actions

    void updateToggleButtonState();                                     ///< @brief Update toggle button state based on panel visibility
    void restoreSidePanelState();                                       ///< @brief Load and apply side panel visibility from settings

    void createLegend();
    void updateLegendPosition();
    void setLegendVisible(bool visible);

    bool confirmDeletion(const QString& eventId);                       ///< @brief Show confirmation dialog for single event deletion
    bool deleteEvent(const QString& eventId);                           ///< @brief Delete a single event with confirmation

    bool confirmBatchDeletion(const QStringList& eventIds);             ///< @brief Show confirmation dialog for batch event deletion
    bool deleteEventWithoutConfirmation(const QString& eventId);        ///< @brief Delete a single event without confirmation (for use after EditDialog)
    bool deleteBatchEvents(const QStringList& eventIds);                ///< @brief Delete multiple events with single confirmation dialog

    QStringList getAllSelectedEventIds() const;                         ///< @brief Get all currently selected event IDs from both scene and side panel

    bool saveToFile(const QString& filePath);                           ///< @brief Save timeline to specified file path
    void setCurrentFilePath(const QString& filePath);                   ///< @brief Set current file path and update auto-save state

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
    QAction* deleteAction_;
    QUndoStack* undoStack_;
    QAction* toggleSidePanelAction_;
    TimelineLegend* legend_;                    ///< Color legend (nullable)
    QCheckBox* legendCheckbox_;                 ///< Legend visibility checkbox
    QSplitter* splitter_;                       ///< Splitter between view and panel
    QString currentFilePath_;                   ///< Path to currently opened/saved file (empty if not saved yet)
};
