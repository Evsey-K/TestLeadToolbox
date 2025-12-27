// TimelineModule.h


#pragma once
#include "shared/interfaces/IModule.h"
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
 */
class TimelineModule : public QWidget, public IModule {
    Q_OBJECT

public:
    explicit TimelineModule(QWidget* parent = nullptr);     ///< @brief Constructs a TimelineModule instance
    ~TimelineModule() override;                             ///< @brief Destructor

    // IModule interface implementation
    QWidget* createWidget(QWidget* parent = nullptr) override;
    QString name() const override;
    QString description() const override;
    QIcon icon() const override;
    QString moduleId() const override;
    void onActivate() override;
    void onDeactivate() override;
    bool hasUnsavedChanges() const override;
    bool save() override;

    TimelineModel* model() const { return model_; }
    QUndoStack* undoStack() const { return undoStack_; }

public slots:
    // File menu
    void saveAs();                                              ///< @brief Public slot to trigger save-as operation (exposed for MainWindow)
    void load();                                                ///< @brief Public slot to trigger load operation (exposed for MainWindow)

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    // Toolbar
    void onSaveClicked();                                       ///< @brief Handle Save button click
    void onSaveAsClicked();                                     ///< @brief Handle Save As button click - always prompts for new location
    void onLoadClicked();                                       ///< @brief Handle Load button click
    void onVersionSettingsClicked();                            ///< @brief Handle Version Settings button click
    void onAddEventClicked();                                   ///< @brief Handle Add Event button click
    void onEditActionTriggered();                               ///< @brief Handle Edit button click from toolbar

    void onEventSelectedInPanel(const QString& eventId);        ///< @brief Handle event selection from side panel

    void onExportScreenshot();                                  ///< @brief Handle Export Screenshot action
    void onExportCSV();                                         ///< @brief Handle Export CSV action
    void onExportPDF();                                         ///< @brief Handle Export PDF action

    void onScrollToDate();                                      ///< @brief Handle Scroll to Date action
    void onGoToCurrentDay();                                    ///< @brief Handle Go to Current Day action
    void onGoToCurrentWeek();                                   ///< @brief Handle Go to Current Week action

    void onZoomIn();                                            ///< @brief Handle zoom in button click
    void onZoomOut();                                           ///< @brief Handle zoom out button click

    void onEditEventRequested(const QString& eventId);          ///< @brief Handle edit event request from scene or side panel
    void onDeleteEventRequested(const QString& eventId);        ///< @brief Handle delete event request from scene or side panel
    void onBatchDeleteRequested(const QStringList& eventIds);   ///< @brief Handle batch delete request from scene
    void onDeleteActionTriggered();                             ///< @brief Handle toolbar delete button click

    void onShowArchivedEvents();
    void updateDeleteActionState();                             ///< @brief Update delete button enabled state based on selection
    void updateEditActionState();                               ///< @brief Update edit button enabled state based on selection
    void onToggleSidePanelClicked();                            ///< @brief Handle side panel toggle button click
    void onLegendToggled(bool checked);
    void onSplitterMoved(int pos, int index);

private:
    void setupUi();
    void setupConnections();
    void setupAutoSave();
    void loadTimelineData();
    void setupUndoStack();
    bool saveToFile(const QString& filePath);                           ///< @brief Save timeline to specified file path
    void setCurrentFilePath(const QString& filePath);                   ///< @brief Set current file path and update auto-save state

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
    QAction* editAction_;
    QAction* deleteAction_;
    QPushButton* zoomInButton_;
    QPushButton* zoomOutButton_;
    QUndoStack* undoStack_;
    QAction* toggleSidePanelAction_;
    TimelineLegend* legend_;                    ///< Color legend (nullable)
    QCheckBox* legendCheckbox_;                 ///< Legend visibility checkbox
    QSplitter* splitter_;                       ///< Splitter between view and panel
    QString currentFilePath_;                   ///< Path to currently opened/saved file (empty if not saved yet)
    bool hasUnsavedChanges_;
};
