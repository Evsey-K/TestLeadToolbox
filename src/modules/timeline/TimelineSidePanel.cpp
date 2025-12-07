// TimelineSidePanel.cpp


#include "TimelineSidePanel.h"
#include "ui_TimelineSidePanel.h"
#include "TimelineModel.h"
#include "TimelineView.h"
#include "TimelineSettings.h"
#include "TimelineExporter.h"
#include "SetTodayDateDialog.h"
#include "SetLookaheadRangeDialog.h"
#include <QListWidget>
#include <QPixmap>
#include <QPainter>
#include <QDate>
#include <QMenu>
#include <QKeyEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabBar>
#include <algorithm>
#include <QClipboard>
#include <QGuiApplication>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialog>


TimelineSidePanel::TimelineSidePanel(TimelineModel* model, TimelineView* view, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TimelineSidePanel)
    , model_(model)
    , view_(view)
{
    ui->setupUi(this);

    // Load saved sort/filter preferences
    currentSortMode_ = TimelineSettings::instance().sidePanelSortMode();
    activeFilterTypes_ = TimelineSettings::instance().sidePanelFilterTypes();

    // Set initial splitter sizes (70% for tabs, 30% for details)
    ui->splitter->setStretchFactor(0, 7);
    ui->splitter->setStretchFactor(1, 3);

    // Hide details panel initially
    ui->eventDetailsGroupBox->setVisible(false);

    // Enable multi-selection on all list widgets
    enableMultiSelection(ui->allEventsList);
    enableMultiSelection(ui->lookaheadList);
    enableMultiSelection(ui->todayList);

    connectSignals();
    refreshAllTabs();
    adjustWidthToFitTabs();

    setupListWidgetContextMenu(ui->allEventsList);
    setupListWidgetContextMenu(ui->lookaheadList);
    setupListWidgetContextMenu(ui->todayList);

    // Setup tab bar context menu
    setupTabBarContextMenu();

    // Set focus policy to receive key events
    setFocusPolicy(Qt::StrongFocus);
    ui->allEventsList->setFocusPolicy(Qt::StrongFocus);
    ui->lookaheadList->setFocusPolicy(Qt::StrongFocus);
    ui->todayList->setFocusPolicy(Qt::StrongFocus);
}


TimelineSidePanel::~TimelineSidePanel()
{
    delete ui;
}


void TimelineSidePanel::keyPressEvent(QKeyEvent* event)
{
    // Handle Delete key with batch deletion support
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        QStringList selectedIds = getSelectedEventIds();

        if (!selectedIds.isEmpty())
        {
            if (selectedIds.size() == 1)
            {
                emit deleteEventRequested(selectedIds.first());
            }
            else
            {
                emit batchDeleteRequested(selectedIds);
            }

            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}


void TimelineSidePanel::connectSignals()
{
    // Connect to model signals
    connect(model_, &TimelineModel::eventAdded, this, &TimelineSidePanel::onEventAdded);
    connect(model_, &TimelineModel::eventRemoved, this, &TimelineSidePanel::onEventRemoved);
    connect(model_, &TimelineModel::eventUpdated, this, &TimelineSidePanel::onEventUpdated);
    connect(model_, &TimelineModel::lanesRecalculated, this, &TimelineSidePanel::onLanesRecalculated);
    connect(model_, &TimelineModel::eventArchived, this, &TimelineSidePanel::onEventRemoved);
    connect(model_, &TimelineModel::eventRestored, this, &TimelineSidePanel::onEventAdded);

    // Connect to list widget click signals
    connect(ui->allEventsList, &QListWidget::itemClicked, this, &TimelineSidePanel::onAllEventsItemClicked);
    connect(ui->lookaheadList, &QListWidget::itemClicked, this, &TimelineSidePanel::onLookaheadItemClicked);
    connect(ui->todayList, &QListWidget::itemClicked, this, &TimelineSidePanel::onTodayItemClicked);

    // Connect selection change signals
    connect(ui->allEventsList, &QListWidget::itemSelectionChanged, this, &TimelineSidePanel::onListSelectionChanged);
    connect(ui->lookaheadList, &QListWidget::itemSelectionChanged, this, &TimelineSidePanel::onListSelectionChanged);
    connect(ui->todayList, &QListWidget::itemSelectionChanged, this, &TimelineSidePanel::onListSelectionChanged);
}


void TimelineSidePanel::setupTabBarContextMenu()
{
    // Enable context menu on tab bar
    QTabBar* tabBar = ui->tabWidget->tabBar();
    tabBar->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tabBar, &QTabBar::customContextMenuRequested,
            this, &TimelineSidePanel::onTabBarContextMenuRequested);
}


void TimelineSidePanel::onTabBarContextMenuRequested(const QPoint& pos)
{
    QTabBar* tabBar = ui->tabWidget->tabBar();

    // Find which tab was clicked
    int clickedTabIndex = -1;
    for (int i = 0; i < tabBar->count(); ++i)
    {
        if (tabBar->tabRect(i).contains(pos))
        {
            clickedTabIndex = i;
            break;
        }
    }

    if (clickedTabIndex == -1)
    {
        return; // Click was not on a tab
    }

    // Map to global position for menu display
    QPoint globalPos = tabBar->mapToGlobal(pos);

    // Show appropriate context menu based on which tab was clicked
    // Tab order: Today (0), Lookahead (1), All Events (2)
    switch (clickedTabIndex)
    {
    case 0: // Today tab
        showTodayTabContextMenu(globalPos);
        break;
    case 1: // Lookahead tab
        showLookaheadTabContextMenu(globalPos);
        break;
    case 2: // All Events tab
        showAllEventsTabContextMenu(globalPos);
        break;
    default:
        break;
    }
}


void TimelineSidePanel::showTodayTabContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);

    // Common actions (at top)
    QAction* refreshAction = menu.addAction("🔄 Refresh Tab");
    refreshAction->setShortcut(QKeySequence::Refresh);

    // Sort submenu
    QMenu* sortMenu = menu.addMenu("📊 Sort By");
    QAction* sortDateAction = sortMenu->addAction("📅 Date");
    QAction* sortPriorityAction = sortMenu->addAction("⭐ Priority");
    QAction* sortTypeAction = sortMenu->addAction("🏷️ Type");

    // Mark current sort mode
    sortDateAction->setCheckable(true);
    sortPriorityAction->setCheckable(true);
    sortTypeAction->setCheckable(true);

    switch (currentSortMode_)
    {
    case TimelineSettings::SortMode::ByDate:
        sortDateAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByPriority:
        sortPriorityAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByType:
        sortTypeAction->setChecked(true);
        break;
    }

    // Filter submenu
    QMenu* filterMenu = menu.addMenu("🔍 Filter By Type");
    QAction* filterMeetingAction = filterMenu->addAction("Meeting");
    QAction* filterActionAction = filterMenu->addAction("Action");
    QAction* filterTestAction = filterMenu->addAction("Test Event");
    QAction* filterReminderAction = filterMenu->addAction("Reminder");
    QAction* filterJiraAction = filterMenu->addAction("Jira Ticket");
    filterMenu->addSeparator();
    QAction* filterAllAction = filterMenu->addAction("All Types...");

    // Mark active filters
    filterMeetingAction->setCheckable(true);
    filterActionAction->setCheckable(true);
    filterTestAction->setCheckable(true);
    filterReminderAction->setCheckable(true);
    filterJiraAction->setCheckable(true);

    filterMeetingAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Meeting));
    filterActionAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Action));
    filterTestAction->setChecked(activeFilterTypes_.contains(TimelineEventType_TestEvent));
    filterReminderAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Reminder));
    filterJiraAction->setChecked(activeFilterTypes_.contains(TimelineEventType_JiraTicket));

    menu.addSeparator();

    // Selection and clipboard actions
    QAction* selectAllAction = menu.addAction("✅ Select All Events");
    selectAllAction->setShortcut(QKeySequence::SelectAll);

    QAction* copyAction = menu.addAction("📋 Copy Event Details");
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(!getSelectedEventIds(ui->todayList).isEmpty());

    menu.addSeparator();

    // Date selection actions (existing)
    QAction* setCurrentAction = menu.addAction("📅 Set to Current Date");
    setCurrentAction->setToolTip("Reset to today's actual date");

    QAction* setSpecificAction = menu.addAction("📆 Set to Specific Date...");
    setSpecificAction->setToolTip("Choose a custom date to display");

    menu.addSeparator();

    // Export submenu (existing)
    QMenu* exportMenu = menu.addMenu("📤 Export");
    QAction* exportPdfAction = exportMenu->addAction("Export to PDF");
    QAction* exportCsvAction = exportMenu->addAction("Export to CSV");
    QAction* exportPngAction = exportMenu->addAction("Export Screenshot (PNG)");

    // Execute menu and handle selection
    QAction* selected = menu.exec(globalPos);

    if (selected == refreshAction)
    {
        onRefreshTab();
    }
    else if (selected == sortDateAction)
    {
        onSortByDate();
    }
    else if (selected == sortPriorityAction)
    {
        onSortByPriority();
    }
    else if (selected == sortTypeAction)
    {
        onSortByType();
    }
    else if (selected == filterMeetingAction)
    {
        toggleFilter(TimelineEventType_Meeting);
    }
    else if (selected == filterActionAction)
    {
        toggleFilter(TimelineEventType_Action);
    }
    else if (selected == filterTestAction)
    {
        toggleFilter(TimelineEventType_TestEvent);
    }
    else if (selected == filterReminderAction)
    {
        toggleFilter(TimelineEventType_Reminder);
    }
    else if (selected == filterJiraAction)
    {
        toggleFilter(TimelineEventType_JiraTicket);
    }
    else if (selected == filterAllAction)
    {
        onFilterByType(); // Shows dialog for batch selection
    }
    else if (selected == selectAllAction)
    {
        onSelectAllEvents();
    }
    else if (selected == copyAction)
    {
        onCopyEventDetails();
    }
    else if (selected == setCurrentAction)
    {
        onSetToCurrentDate();
    }
    else if (selected == setSpecificAction)
    {
        onSetToSpecificDate();
    }
    else if (selected == exportPdfAction)
    {
        onExportTodayTab("PDF");
    }
    else if (selected == exportCsvAction)
    {
        onExportTodayTab("CSV");
    }
    else if (selected == exportPngAction)
    {
        onExportTodayTab("PNG");
    }
}


void TimelineSidePanel::showLookaheadTabContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);

    // Common actions (at top)
    QAction* refreshAction = menu.addAction("🔄 Refresh Tab");
    refreshAction->setShortcut(QKeySequence::Refresh);

    // Sort submenu
    QMenu* sortMenu = menu.addMenu("📊 Sort By");
    QAction* sortDateAction = sortMenu->addAction("📅 Date");
    QAction* sortPriorityAction = sortMenu->addAction("⭐ Priority");
    QAction* sortTypeAction = sortMenu->addAction("🏷️ Type");

    sortDateAction->setCheckable(true);
    sortPriorityAction->setCheckable(true);
    sortTypeAction->setCheckable(true);

    switch (currentSortMode_)
    {
    case TimelineSettings::SortMode::ByDate:
        sortDateAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByPriority:
        sortPriorityAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByType:
        sortTypeAction->setChecked(true);
        break;
    }

    // Filter submenu
    QMenu* filterMenu = menu.addMenu("🔍 Filter By Type");
    QAction* filterMeetingAction = filterMenu->addAction("Meeting");
    QAction* filterActionAction = filterMenu->addAction("Action");
    QAction* filterTestAction = filterMenu->addAction("Test Event");
    QAction* filterReminderAction = filterMenu->addAction("Reminder");
    QAction* filterJiraAction = filterMenu->addAction("Jira Ticket");
    QAction* filterAllAction = filterMenu->addAction("All Types");

    filterMeetingAction->setCheckable(true);
    filterActionAction->setCheckable(true);
    filterTestAction->setCheckable(true);
    filterReminderAction->setCheckable(true);
    filterJiraAction->setCheckable(true);

    filterMeetingAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Meeting));
    filterActionAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Action));
    filterTestAction->setChecked(activeFilterTypes_.contains(TimelineEventType_TestEvent));
    filterReminderAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Reminder));
    filterJiraAction->setChecked(activeFilterTypes_.contains(TimelineEventType_JiraTicket));

    menu.addSeparator();

    // Selection and clipboard actions
    QAction* selectAllAction = menu.addAction("✅ Select All Events");
    selectAllAction->setShortcut(QKeySequence::SelectAll);

    QAction* copyAction = menu.addAction("📋 Copy Event Details");
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(!getSelectedEventIds(ui->lookaheadList).isEmpty());

    menu.addSeparator();

    // Range selection action (existing)
    int currentDays = TimelineSettings::instance().lookaheadTabDays();
    QAction* setRangeAction = menu.addAction(QString("📆 Set Lookahead Range (currently %1 days)").arg(currentDays));
    setRangeAction->setToolTip("Configure how many days ahead to display");

    menu.addSeparator();

    // Export submenu (existing)
    QMenu* exportMenu = menu.addMenu("📤 Export");
    QAction* exportPdfAction = exportMenu->addAction("Export to PDF");
    QAction* exportCsvAction = exportMenu->addAction("Export to CSV");
    QAction* exportPngAction = exportMenu->addAction("Export Screenshot (PNG)");

    // Execute menu and handle selection
    QAction* selected = menu.exec(globalPos);

    if (selected == refreshAction)
    {
        onRefreshTab();
    }
    else if (selected == sortDateAction)
    {
        onSortByDate();
    }
    else if (selected == sortPriorityAction)
    {
        onSortByPriority();
    }
    else if (selected == sortTypeAction)
    {
        onSortByType();
    }
    else if (selected == filterMeetingAction)
    {
        toggleFilter(TimelineEventType_Meeting);
    }
    else if (selected == filterActionAction)
    {
        toggleFilter(TimelineEventType_Action);
    }
    else if (selected == filterTestAction)
    {
        toggleFilter(TimelineEventType_TestEvent);
    }
    else if (selected == filterReminderAction)
    {
        toggleFilter(TimelineEventType_Reminder);
    }
    else if (selected == filterJiraAction)
    {
        toggleFilter(TimelineEventType_JiraTicket);
    }
    else if (selected == filterAllAction)
    {
        onFilterByType();
    }
    else if (selected == selectAllAction)
    {
        onSelectAllEvents();
    }
    else if (selected == copyAction)
    {
        onCopyEventDetails();
    }
    else if (selected == setRangeAction)
    {
        onSetLookaheadRange();
    }
    else if (selected == exportPdfAction)
    {
        onExportLookaheadTab("PDF");
    }
    else if (selected == exportCsvAction)
    {
        onExportLookaheadTab("CSV");
    }
    else if (selected == exportPngAction)
    {
        onExportLookaheadTab("PNG");
    }
}


void TimelineSidePanel::showAllEventsTabContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);

    // Common actions (at top)
    QAction* refreshAction = menu.addAction("🔄 Refresh Tab");
    refreshAction->setShortcut(QKeySequence::Refresh);

    // Sort submenu
    QMenu* sortMenu = menu.addMenu("📊 Sort By");
    QAction* sortDateAction = sortMenu->addAction("📅 Date");
    QAction* sortPriorityAction = sortMenu->addAction("⭐ Priority");
    QAction* sortTypeAction = sortMenu->addAction("🏷️ Type");

    sortDateAction->setCheckable(true);
    sortPriorityAction->setCheckable(true);
    sortTypeAction->setCheckable(true);

    switch (currentSortMode_)
    {
    case TimelineSettings::SortMode::ByDate:
        sortDateAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByPriority:
        sortPriorityAction->setChecked(true);
        break;
    case TimelineSettings::SortMode::ByType:
        sortTypeAction->setChecked(true);
        break;
    }

    // Filter submenu
    QMenu* filterMenu = menu.addMenu("🔍 Filter By Type");
    QAction* filterMeetingAction = filterMenu->addAction("Meeting");
    QAction* filterActionAction = filterMenu->addAction("Action");
    QAction* filterTestAction = filterMenu->addAction("Test Event");
    QAction* filterReminderAction = filterMenu->addAction("Reminder");
    QAction* filterJiraAction = filterMenu->addAction("Jira Ticket");
    QAction* filterAllAction = filterMenu->addAction("All Types");

    filterMeetingAction->setCheckable(true);
    filterActionAction->setCheckable(true);
    filterTestAction->setCheckable(true);
    filterReminderAction->setCheckable(true);
    filterJiraAction->setCheckable(true);

    filterMeetingAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Meeting));
    filterActionAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Action));
    filterTestAction->setChecked(activeFilterTypes_.contains(TimelineEventType_TestEvent));
    filterReminderAction->setChecked(activeFilterTypes_.contains(TimelineEventType_Reminder));
    filterJiraAction->setChecked(activeFilterTypes_.contains(TimelineEventType_JiraTicket));

    menu.addSeparator();

    // Selection and clipboard actions
    QAction* selectAllAction = menu.addAction("✅ Select All Events");
    selectAllAction->setShortcut(QKeySequence::SelectAll);

    QAction* copyAction = menu.addAction("📋 Copy Event Details");
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(!getSelectedEventIds(ui->allEventsList).isEmpty());

    menu.addSeparator();

    // Export submenu (existing)
    QMenu* exportMenu = menu.addMenu("📤 Export");
    QAction* exportPdfAction = exportMenu->addAction("Export to PDF");
    QAction* exportCsvAction = exportMenu->addAction("Export to CSV");
    QAction* exportPngAction = exportMenu->addAction("Export Screenshot (PNG)");

    // Execute menu and handle selection
    QAction* selected = menu.exec(globalPos);

    if (selected == refreshAction)
    {
        onRefreshTab();
    }
    else if (selected == sortDateAction)
    {
        onSortByDate();
    }
    else if (selected == sortPriorityAction)
    {
        onSortByPriority();
    }
    else if (selected == sortTypeAction)
    {
        onSortByType();
    }
    else if (selected == filterMeetingAction)
    {
        toggleFilter(TimelineEventType_Meeting);
    }
    else if (selected == filterActionAction)
    {
        toggleFilter(TimelineEventType_Action);
    }
    else if (selected == filterTestAction)
    {
        toggleFilter(TimelineEventType_TestEvent);
    }
    else if (selected == filterJiraAction)
    {
        toggleFilter(TimelineEventType_JiraTicket);
    }
    else if (selected == filterReminderAction)
    {
        toggleFilter(TimelineEventType_Reminder);
    }
    else if (selected == filterAllAction)
    {
        onFilterByType();
    }
    else if (selected == selectAllAction)
    {
        onSelectAllEvents();
    }
    else if (selected == copyAction)
    {
        onCopyEventDetails();
    }
    else if (selected == exportPdfAction)
    {
        onExportAllEventsTab("PDF");
    }
    else if (selected == exportCsvAction)
    {
        onExportAllEventsTab("CSV");
    }
    else if (selected == exportPngAction)
    {
        onExportAllEventsTab("PNG");
    }
}


void TimelineSidePanel::onSetToCurrentDate()
{
    // Reset to using current date
    TimelineSettings::instance().setTodayTabUseCustomDate(false);

    // Refresh the Today tab to reflect current date
    refreshTodayTab();
    updateTodayTabLabel();

    QMessageBox::information(this, "Today Tab Reset",
                             QString("Today tab reset to current date: %1")
                                 .arg(QDate::currentDate().toString("yyyy-MM-dd")));
}


void TimelineSidePanel::onSetToSpecificDate()
{
    // Get current custom date if set, otherwise use today
    QDate currentDate = TimelineSettings::instance().todayTabUseCustomDate()
                            ? TimelineSettings::instance().todayTabCustomDate()
                            : QDate::currentDate();

    // Show date selection dialog
    SetTodayDateDialog dialog(
        currentDate,
        model_->versionStartDate(),
        model_->versionEndDate(),
        this
        );

    if (dialog.exec() == QDialog::Accepted)
    {
        QDate selectedDate = dialog.selectedDate();

        // Save custom date settings
        TimelineSettings::instance().setTodayTabUseCustomDate(true);
        TimelineSettings::instance().setTodayTabCustomDate(selectedDate);

        // Refresh the Today tab to show events for selected date
        refreshTodayTab();
        updateTodayTabLabel();

        QMessageBox::information(this, "Today Tab Updated",
                                 QString("Today tab now showing events for: %1")
                                     .arg(selectedDate.toString("yyyy-MM-dd")));
    }
}


void TimelineSidePanel::onExportTodayTab(const QString& format)
{
    QVector<TimelineEvent> events = getEventsFromTab(0); // Today tab is index 0

    if (events.isEmpty())
    {
        QMessageBox::information(this, "No Events",
                                 "There are no events in the Today tab to export.");
        return;
    }

    QString tabName = TimelineSettings::instance().todayTabUseCustomDate()
                          ? QString("UserSetDate_%1").arg(TimelineSettings::instance().todayTabCustomDate().toString("yyyy-MM-dd"))
                          : QString("Today_%1").arg(QDate::currentDate().toString("yyyy-MM-dd"));

    exportEvents(events, format, tabName);
}


void TimelineSidePanel::onSetLookaheadRange()
{
    int currentDays = TimelineSettings::instance().lookaheadTabDays();

    // Show range selection dialog
    SetLookaheadRangeDialog dialog(currentDays, this);

    if (dialog.exec() == QDialog::Accepted)
    {
        int newDays = dialog.selectedDays();

        // Save new range
        TimelineSettings::instance().setLookaheadTabDays(newDays);

        // Refresh the Lookahead tab
        refreshLookaheadTab();
        updateLookaheadTabLabel();

        QDate today = QDate::currentDate();
        QDate endDate = today.addDays(newDays);

        QMessageBox::information(this, "Lookahead Range Updated",
                                 QString("Lookahead tab now showing %1 days (%2 to %3)")
                                     .arg(newDays)
                                     .arg(today.toString("MMM dd"))
                                     .arg(endDate.toString("MMM dd, yyyy")));
    }
}


void TimelineSidePanel::onExportLookaheadTab(const QString& format)
{
    QVector<TimelineEvent> events = getEventsFromTab(1); // Lookahead tab is index 1

    if (events.isEmpty())
    {
        QMessageBox::information(this, "No Events",
                                 "There are no events in the Lookahead tab to export.");
        return;
    }

    int days = TimelineSettings::instance().lookaheadTabDays();
    QString tabName = QString("Lookahead_%1days").arg(days);

    exportEvents(events, format, tabName);
}


void TimelineSidePanel::onExportAllEventsTab(const QString& format)
{
    QVector<TimelineEvent> events = getEventsFromTab(2); // All Events tab is index 2

    if (events.isEmpty())
    {
        QMessageBox::information(this, "No Events",
                                 "There are no events to export.");
        return;
    }

    QString tabName = "AllEvents";

    exportEvents(events, format, tabName);
}


QVector<TimelineEvent> TimelineSidePanel::getEventsFromTab(int tabIndex) const
{
    switch (tabIndex)
    {
    case 0: // Today tab
    {
        QDate targetDate = TimelineSettings::instance().todayTabUseCustomDate()
        ? TimelineSettings::instance().todayTabCustomDate()
        : QDate::currentDate();

        QVector<TimelineEvent> allEvents = model_->getAllEvents();
        QVector<TimelineEvent> filteredEvents;

        for (const auto& event : allEvents)
        {
            if (event.occursOnDate(targetDate))
            {
                filteredEvents.append(event);
            }
        }

        return filteredEvents;
    }

    case 1: // Lookahead tab
    {
        int days = TimelineSettings::instance().lookaheadTabDays();
        return model_->getEventsLookahead(days);
    }

    case 2: // All Events tab
    {
        return model_->getAllEvents();
    }

    default:
        return QVector<TimelineEvent>();
    }
}


bool TimelineSidePanel::exportEvents(const QVector<TimelineEvent>& events,
                                     const QString& format,
                                     const QString& tabName)
{
    QString filter;
    QString defaultName;

    if (format == "PDF")
    {
        filter = "PDF Files (*.pdf)";
        defaultName = QString("timeline_%1.pdf").arg(tabName);
    }
    else if (format == "CSV")
    {
        filter = "CSV Files (*.csv)";
        defaultName = QString("timeline_%1.csv").arg(tabName);
    }
    else if (format == "PNG")
    {
        filter = "PNG Images (*.png)";
        defaultName = QString("timeline_%1.png").arg(tabName);
    }
    else
    {
        qWarning() << "Unknown export format:" << format;
        return false;
    }

    // Show file save dialog
    QString filePath = QFileDialog::getSaveFileName(
        this,
        QString("Export %1 to %2").arg(tabName, format),
        defaultName,
        filter
        );

    if (filePath.isEmpty())
    {
        return false; // User cancelled
    }

    bool success = false;

    if (format == "PDF")
    {
        QString reportTitle = QString("Timeline Export: %1 (%2 events)")
        .arg(tabName)
            .arg(events.size());

        success = TimelineExporter::exportEventsToPDF(events, view_, filePath, true, reportTitle);
    }
    else if (format == "CSV")
    {
        success = TimelineExporter::exportEventsToCSV(events, filePath);
    }
    else if (format == "PNG")
    {
        if (!view_)
        {
            QMessageBox::warning(this, "Export Failed",
                                 "Screenshot export requires a timeline view reference.");
            return false;
        }

        success = TimelineExporter::exportScreenshot(view_, filePath, true);
    }

    if (success)
    {
        QMessageBox::information(this, "Export Successful",
                                 QString("Exported %1 events to:\n%2")
                                     .arg(events.size())
                                     .arg(filePath));
    }
    else
    {
        QMessageBox::critical(this, "Export Failed",
                              QString("Failed to export to %1").arg(format));
    }

    return success;
}


void TimelineSidePanel::updateTodayTabLabel()
{
    QDate targetDate;
    QString labelPrefix;

    if (TimelineSettings::instance().todayTabUseCustomDate())
    {
        targetDate = TimelineSettings::instance().todayTabCustomDate();
        labelPrefix = "User Set";
    }
    else
    {
        targetDate = QDate::currentDate();
        labelPrefix = "Today";
    }

    // Count events on target date
    QVector<TimelineEvent> allEvents = model_->getAllEvents();
    int eventCount = 0;

    for (const auto& event : allEvents)
    {
        if (event.occursOnDate(targetDate))
        {
            eventCount++;
        }
    }

    QString label = QString("%1 (%2) - %3")
                        .arg(labelPrefix)
                        .arg(eventCount)
                        .arg(targetDate.toString("MMM dd"));

    ui->tabWidget->setTabText(0, label);
}


void TimelineSidePanel::updateLookaheadTabLabel()
{
    int days = TimelineSettings::instance().lookaheadTabDays();
    QVector<TimelineEvent> events = model_->getEventsLookahead(days);

    QString label = QString("Lookahead (%1) - %2 days")
                        .arg(events.size())
                        .arg(days);

    ui->tabWidget->setTabText(1, label);
}


void TimelineSidePanel::updateAllEventsTabLabel()
{
    QVector<TimelineEvent> events = model_->getAllEvents();

    QString label = QString("All Events (%1)").arg(events.size());

    ui->tabWidget->setTabText(2, label);
}


void TimelineSidePanel::refreshAllTabs()
{
    refreshAllEventsTab();
    refreshLookaheadTab();
    refreshTodayTab();
}


void TimelineSidePanel::refreshTodayTab()
{
    QDate targetDate = TimelineSettings::instance().todayTabUseCustomDate()
    ? TimelineSettings::instance().todayTabCustomDate()
    : QDate::currentDate();

    QVector<TimelineEvent> allEvents = model_->getAllEvents();
    QVector<TimelineEvent> events;

    for (const auto& event : allEvents)
    {
        if (event.occursOnDate(targetDate))
        {
            events.append(event);
        }
    }

    // Apply sort and filter
    events = applySortAndFilter(events);

    populateListWidget(ui->todayList, events);
    updateTodayTabLabel();
    adjustWidthToFitTabs();
}


void TimelineSidePanel::refreshLookaheadTab()
{
    int days = TimelineSettings::instance().lookaheadTabDays();
    QVector<TimelineEvent> events = model_->getEventsLookahead(days);

    // Apply sort and filter
    events = applySortAndFilter(events);

    populateListWidget(ui->lookaheadList, events);
    updateLookaheadTabLabel();
    adjustWidthToFitTabs();
}


void TimelineSidePanel::refreshAllEventsTab()
{
    QVector<TimelineEvent> events = model_->getAllEvents();

    // Apply sort and filter
    events = applySortAndFilter(events);

    populateListWidget(ui->allEventsList, events);
    updateAllEventsTabLabel();
    adjustWidthToFitTabs();
}


void TimelineSidePanel::populateListWidget(QListWidget* listWidget, const QVector<TimelineEvent>& events)
{
    listWidget->clear();

    if (events.isEmpty())
    {
        auto emptyItem = new QListWidgetItem("No events");
        emptyItem->setFlags(Qt::NoItemFlags);
        emptyItem->setForeground(Qt::gray);
        listWidget->addItem(emptyItem);
        return;
    }

    for (const auto& event : events)
    {
        QListWidgetItem* item = createListItem(event);
        listWidget->addItem(item);
    }
}


QListWidgetItem* TimelineSidePanel::createListItem(const TimelineEvent& event)
{
    auto item = new QListWidgetItem();
    item->setText(formatEventText(event));
    item->setData(Qt::UserRole, event.id);

    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(event.color);
    item->setIcon(QIcon(colorPixmap));

    QString tooltip = QString("%1\n%2\nPriority: %3\nLane: %4")
                          .arg(event.title)
                          .arg(formatEventDateRange(event))
                          .arg(event.priority)
                          .arg(event.lane);

    if (!event.description.isEmpty())
    {
        tooltip += "\n\n" + event.description;
    }

    item->setToolTip(tooltip);
    return item;
}


QString TimelineSidePanel::formatEventText(const TimelineEvent& event) const
{
    QString dateRange = formatEventDateRange(event);
    QString priorityIndicator = (event.priority <= 2) ? "🔴 " : "";

    return QString("%1%2\n%3")
        .arg(priorityIndicator)
        .arg(event.title)
        .arg(dateRange);
}


QString TimelineSidePanel::formatEventDateRange(const TimelineEvent& event) const
{
    if (event.startDate == event.endDate)
    {
        return event.startDate.toString("MMM dd, yyyy");
    }
    else
    {
        return QString("%1 - %2")
        .arg(event.startDate.toString("MMM dd"))
            .arg(event.endDate.toString("MMM dd, yyyy"));
    }
}


void TimelineSidePanel::adjustWidthToFitTabs()
{
    QTabBar* tabBar = ui->tabWidget->tabBar();
    QFontMetrics fm(tabBar->font());

    int totalWidth = 0;

    for (int i = 0; i < ui->tabWidget->count(); ++i)
    {
        QString tabText = ui->tabWidget->tabText(i);
        int textWidth = fm.horizontalAdvance(tabText);
        totalWidth += textWidth + 30;
    }

    totalWidth += 20;
    setMinimumWidth(totalWidth);
}


void TimelineSidePanel::displayEventDetails(const QString& eventId)
{
    const TimelineEvent* event = model_->getEvent(eventId);
    if (!event)
    {
        clearEventDetails();
        return;
    }

    // === COMMON FIELDS (All Event Types) ===
    ui->titleValueLabel->setText(event->title);
    ui->typeValueLabel->setText(eventTypeToString(event->type));
    ui->priorityValueLabel->setText(QString::number(event->priority));
    ui->laneValueLabel->setText(QString::number(event->lane));

    // Clear previous type-specific content
    clearTypeSpecificFields();

    // Build details text combining type-specific info and description
    QString detailsText;

    // === TYPE-SPECIFIC FIELDS ===
    switch (event->type)
    {
    case TimelineEventType_Meeting:
        detailsText = buildMeetingDetails(*event);
        break;

    case TimelineEventType_Action:
        detailsText = buildActionDetails(*event);
        break;

    case TimelineEventType_TestEvent:
        detailsText = buildTestEventDetails(*event);
        break;

    case TimelineEventType_Reminder:
        detailsText = buildReminderDetails(*event);
        break;

    case TimelineEventType_JiraTicket:
        detailsText = buildJiraTicketDetails(*event);
        break;

    default:
        detailsText = buildGenericDetails(*event);
        break;
    }

    // === ADD DESCRIPTION (Common to all types) ===
    if (!event->description.isEmpty())
    {
        if (!detailsText.isEmpty())
        {
            detailsText += "\n\n";
        }
        detailsText += QString("<b>Description:</b><br>%1").arg(event->description);
    }

    // Set the combined details text
    ui->detailsTextEdit->setHtml(detailsText);

    // Show the details group box
    ui->eventDetailsGroupBox->setVisible(true);
}


QString TimelineSidePanel::buildMeetingDetails(const TimelineEvent& event)
{
    QString details;

    // Date/Time Information
    details += QString("<b>When:</b><br>");
    details += QString("Start: %1 at %2<br>")
                   .arg(event.startDate.toString("yyyy-MM-dd"))
                   .arg(event.startTime.toString("HH:mm"));
    details += QString("End: %1 at %2<br>")
                   .arg(event.endDate.toString("yyyy-MM-dd"))
                   .arg(event.endTime.toString("HH:mm"));

    // Duration calculation
    int durationDays = event.startDate.daysTo(event.endDate);
    if (durationDays == 0)
    {
        QTime startTime = event.startTime;
        QTime endTime = event.endTime;
        int minutes = startTime.secsTo(endTime) / 60;
        int hours = minutes / 60;
        int mins = minutes % 60;
        details += QString("Duration: %1h %2m<br>").arg(hours).arg(mins);
    }
    else
    {
        details += QString("Duration: %1 days<br>").arg(durationDays + 1);
    }

    details += "<br>";

    // Location
    if (!event.location.isEmpty())
    {
        details += QString("<b>Location:</b><br>%1<br><br>").arg(event.location);
    }

    // Participants
    if (!event.participants.isEmpty())
    {
        details += QString("<b>Participants:</b><br>%1<br>").arg(event.participants);
    }

    return details;
}


QString TimelineSidePanel::buildActionDetails(const TimelineEvent& event)
{
    QString details;

    // Start Date/Time
    details += QString("<b>Started:</b><br>");
    details += QString("%1 at %2<br><br>")
                   .arg(event.startDate.toString("yyyy-MM-dd"))
                   .arg(event.startTime.toString("HH:mm"));

    // Due Date/Time
    if (event.dueDateTime.isValid())
    {
        details += QString("<b>Due:</b><br>");
        details += QString("%1<br><br>")
                       .arg(event.dueDateTime.toString("yyyy-MM-dd HH:mm"));

        // Calculate time remaining
        QDateTime now = QDateTime::currentDateTime();
        qint64 secsRemaining = now.secsTo(event.dueDateTime);

        if (secsRemaining > 0)
        {
            int days = secsRemaining / 86400;
            int hours = (secsRemaining % 86400) / 3600;
            details += QString("<b>Time Remaining:</b><br>%1 days, %2 hours<br><br>")
                           .arg(days)
                           .arg(hours);
        }
        else if (secsRemaining < 0)
        {
            int days = (-secsRemaining) / 86400;
            details += QString("<b style='color:red;'>Overdue by:</b> %1 days<br><br>").arg(days);
        }
        else
        {
            details += QString("<b style='color:orange;'>Due now!</b><br><br>");
        }
    }

    // Assignee
    if (!event.assignee.isEmpty())
    {
        details += QString("<b>Assignee:</b><br>%1<br>").arg(event.assignee);
    }

    return details;
}


QString TimelineSidePanel::buildTestEventDetails(const TimelineEvent& event)
{
    QString details;

    // Date Information
    details += QString("<b>Test Window:</b><br>");
    details += QString("Start: %1<br>").arg(event.startDate.toString("yyyy-MM-dd"));
    details += QString("End: %1<br><br>").arg(event.endDate.toString("yyyy-MM-dd"));

    // Test Type
    if (!event.testType.isEmpty())
    {
        details += QString("<b>Test Type:</b><br>%1<br><br>").arg(event.testType);
    }

    // Environment
    if (!event.environment.isEmpty())
    {
        details += QString("<b>Environment:</b><br>%1<br><br>").arg(event.environment);
    }

    // Test Lead
    if (!event.testLead.isEmpty())
    {
        details += QString("<b>Test Lead:</b><br>%1<br>").arg(event.testLead);
    }

    return details;
}


QString TimelineSidePanel::buildReminderDetails(const TimelineEvent& event)
{
    QString details;

    // Reminder Time
    details += QString("<b>Reminder Date/Time:</b><br>");
    details += QString("%1 at %2<br><br>")
                   .arg(event.startDate.toString("yyyy-MM-dd"))
                   .arg(event.startTime.toString("HH:mm"));

    // Calculate time until reminder
    QDateTime reminderTime(event.startDate, event.startTime);
    QDateTime now = QDateTime::currentDateTime();
    qint64 secsUntilReminder = now.secsTo(reminderTime);

    if (secsUntilReminder > 0)
    {
        int days = secsUntilReminder / 86400;
        int hours = (secsUntilReminder % 86400) / 3600;
        int mins = ((secsUntilReminder % 86400) % 3600) / 60;

        if (days > 0)
        {
            details += QString("<b>Time Until:</b><br>%1 days, %2 hours, %3 minutes<br>")
            .arg(days)
                .arg(hours)
                .arg(mins);
        }
        else if (hours > 0)
        {
            details += QString("<b>Time Until:</b><br>%1 hours, %2 minutes<br>")
            .arg(hours)
                .arg(mins);
        }
        else
        {
            details += QString("<b>Time Until:</b><br>%1 minutes<br>").arg(mins);
        }
    }
    else if (secsUntilReminder < 0)
    {
        details += QString("<b style='color:red;'>Reminder Past Due</b><br>");
    }
    else
    {
        details += QString("<b style='color:orange;'>Reminder is NOW!</b><br>");
    }

    return details;
}


QString TimelineSidePanel::buildJiraTicketDetails(const TimelineEvent& event)
{
    QString details;

    // Jira Key
    if (!event.jiraKey.isEmpty())
    {
        details += QString("<b>Jira Key:</b> %1<br><br>").arg(event.jiraKey);
    }

    // Jira Type with Icon
    if (!event.jiraType.isEmpty())
    {
        QString typeIcon = "📝";
        if (event.jiraType == "Bug")
            typeIcon = "🐛";
        else if (event.jiraType == "Story")
            typeIcon = "📖";
        else if (event.jiraType == "Epic")
            typeIcon = "🎯";
        else if (event.jiraType == "Task")
            typeIcon = "✅";

        details += QString("<b>Type:</b> %1 %2<br>").arg(typeIcon).arg(event.jiraType);
    }

    // Status
    if (!event.jiraStatus.isEmpty())
    {
        QString statusColor = "black";
        if (event.jiraStatus == "Done")
            statusColor = "green";
        else if (event.jiraStatus == "In Progress")
            statusColor = "blue";

        details += QString("<b>Status:</b> <span style='color:%1;'>%2</span><br><br>")
                       .arg(statusColor)
                       .arg(event.jiraStatus);
    }

    // Date Range
    details += QString("<b>Timeline:</b><br>");
    details += QString("Start: %1<br>").arg(event.startDate.toString("yyyy-MM-dd"));
    details += QString("Due: %1<br>").arg(event.endDate.toString("yyyy-MM-dd"));

    // Calculate time remaining
    QDate today = QDate::currentDate();
    int daysRemaining = today.daysTo(event.endDate);

    if (daysRemaining > 0)
    {
        details += QString("<b>Days Remaining:</b> %1<br>").arg(daysRemaining);
    }
    else if (daysRemaining < 0)
    {
        details += QString("<b style='color:red;'>Overdue by %1 days</b><br>").arg(-daysRemaining);
    }
    else
    {
        details += QString("<b style='color:orange;'>Due today!</b><br>");
    }

    return details;
}


QString TimelineSidePanel::buildGenericDetails(const TimelineEvent& event)
{
    // Fallback for any event type without specific handler
    QString details;

    details += QString("<b>Dates:</b><br>");
    details += QString("%1 to %2<br>")
                   .arg(event.startDate.toString("yyyy-MM-dd"))
                   .arg(event.endDate.toString("yyyy-MM-dd"));

    if (event.startDate.daysTo(event.endDate) > 0)
    {
        details += QString("Duration: %1 days<br>")
        .arg(event.startDate.daysTo(event.endDate) + 1);
    }

    return details;
}






void TimelineSidePanel::clearTypeSpecificFields()
{
    // Clear any dynamically created labels/widgets for type-specific fields
    // This ensures clean slate before displaying new event details

    // If using dynamic labels, remove them here
    // For simplicity, we'll use a rich text display approach
}


void TimelineSidePanel::displayMeetingDetails(const TimelineEvent& event)
{
    QString details;

    // Date/Time Information
    details += QString("<b>When:</b><br>");
    details += QString("Start: %1 at %2<br>")
                   .arg(event.startDate.toString("yyyy-MM-dd"))
                   .arg(event.startTime.toString("HH:mm"));
    details += QString("End: %1 at %2<br>")
                   .arg(event.endDate.toString("yyyy-MM-dd"))
                   .arg(event.endTime.toString("HH:mm"));

    // Duration calculation
    int durationDays = event.startDate.daysTo(event.endDate);
    if (durationDays == 0)
    {
        QTime startTime = event.startTime;
        QTime endTime = event.endTime;
        int minutes = startTime.secsTo(endTime) / 60;
        int hours = minutes / 60;
        int mins = minutes % 60;
        details += QString("Duration: %1h %2m<br>").arg(hours).arg(mins);
    }
    else
    {
        details += QString("Duration: %1 days<br>").arg(durationDays + 1);
    }

    details += "<br>";

    // Location
    if (!event.location.isEmpty())
    {
        details += QString("<b>Location:</b><br>%1<br><br>").arg(event.location);
    }

    // Participants
    if (!event.participants.isEmpty())
    {
        details += QString("<b>Participants:</b><br>%1<br>").arg(event.participants);
    }

    ui->datesValueLabel->setText(details);
}


void TimelineSidePanel::displayActionDetails(const TimelineEvent& event)
{
    QString details;

    // Start Date/Time
    details += QString("<b>Started:</b><br>");
    details += QString("%1 at %2<br><br>")
                   .arg(event.startDate.toString("yyyy-MM-dd"))
                   .arg(event.startTime.toString("HH:mm"));

    // Due Date/Time
    if (event.dueDateTime.isValid())
    {
        details += QString("<b>Due:</b><br>");
        details += QString("%1<br><br>")
                       .arg(event.dueDateTime.toString("yyyy-MM-dd HH:mm"));

        // Calculate time remaining
        QDateTime now = QDateTime::currentDateTime();
        qint64 secsRemaining = now.secsTo(event.dueDateTime);

        if (secsRemaining > 0)
        {
            int days = secsRemaining / 86400;
            int hours = (secsRemaining % 86400) / 3600;
            details += QString("<b>Time Remaining:</b><br>%1 days, %2 hours<br><br>")
                           .arg(days).arg(hours);
        }
        else
        {
            details += QString("<b style='color:red;'>OVERDUE</b><br><br>");
        }
    }

    // Status
    if (!event.status.isEmpty())
    {
        QString statusColor = "black";
        if (event.status == "Completed")
            statusColor = "green";
        else if (event.status == "In Progress")
            statusColor = "blue";
        else if (event.status == "Blocked")
            statusColor = "red";

        details += QString("<b>Status:</b> <span style='color:%1;'>%2</span><br>")
                       .arg(statusColor)
                       .arg(event.status);
    }

    ui->datesValueLabel->setText(details);
}


void TimelineSidePanel::displayTestEventDetails(const TimelineEvent& event)
{
    QString details;

    // Date Range
    details += QString("<b>Test Period:</b><br>");
    details += QString("%1 to %2<br>")
                   .arg(event.startDate.toString("yyyy-MM-dd"))
                   .arg(event.endDate.toString("yyyy-MM-dd"));
    details += QString("Duration: %1 days<br><br>")
                   .arg(event.startDate.daysTo(event.endDate) + 1);

    // Test Category
    if (!event.testCategory.isEmpty())
    {
        details += QString("<b>Category:</b> %1<br><br>").arg(event.testCategory);
    }

    // Preparation Checklist
    if (!event.preparationChecklist.isEmpty())
    {
        details += QString("<b>Preparation Checklist:</b><br>");

        int completed = 0;
        int total = event.preparationChecklist.size();

        for (auto it = event.preparationChecklist.begin();
             it != event.preparationChecklist.end(); ++it)
        {
            QString checkMark = it.value() ? "✓" : "☐";
            QString style = it.value() ? "color:green;" : "color:gray;";
            details += QString("<span style='%1'>%2 %3</span><br>")
                           .arg(style)
                           .arg(checkMark)
                           .arg(it.key());

            if (it.value())
                completed++;
        }

        details += QString("<br><b>Progress:</b> %1/%2 completed (%3%)<br>")
                       .arg(completed)
                       .arg(total)
                       .arg((completed * 100) / total);
    }

    ui->datesValueLabel->setText(details);
}


void TimelineSidePanel::displayReminderDetails(const TimelineEvent& event)
{
    QString details;

    // Reminder Date/Time
    if (event.reminderDateTime.isValid())
    {
        details += QString("<b>Reminder Set For:</b><br>");
        details += QString("%1<br><br>")
                       .arg(event.reminderDateTime.toString("yyyy-MM-dd HH:mm"));

        // Calculate time until reminder
        QDateTime now = QDateTime::currentDateTime();
        qint64 secsUntil = now.secsTo(event.reminderDateTime);

        if (secsUntil > 0)
        {
            int days = secsUntil / 86400;
            int hours = (secsUntil % 86400) / 3600;
            int mins = ((secsUntil % 86400) % 3600) / 60;

            details += QString("<b>Time Until:</b><br>");
            if (days > 0)
                details += QString("%1 days, ").arg(days);
            details += QString("%1 hours, %2 minutes<br><br>").arg(hours).arg(mins);
        }
        else
        {
            details += QString("<b style='color:red;'>Past due</b><br><br>");
        }
    }

    // Recurring Rule
    if (!event.recurringRule.isEmpty() && event.recurringRule != "None")
    {
        details += QString("<b>Recurrence:</b> %1<br>").arg(event.recurringRule);
    }

    ui->datesValueLabel->setText(details);
}


void TimelineSidePanel::displayJiraTicketDetails(const TimelineEvent& event)
{
    QString details;

    // Jira Key (clickable if it's a URL pattern)
    if (!event.jiraKey.isEmpty())
    {
        details += QString("<b>Jira Ticket:</b> %1<br><br>").arg(event.jiraKey);
    }

    // Summary
    if (!event.jiraSummary.isEmpty())
    {
        details += QString("<b>Summary:</b><br>%1<br><br>").arg(event.jiraSummary);
    }

    // Type
    if (!event.jiraType.isEmpty())
    {
        QString typeIcon = "📋";
        if (event.jiraType == "Bug")
            typeIcon = "🐛";
        else if (event.jiraType == "Story")
            typeIcon = "📖";
        else if (event.jiraType == "Epic")
            typeIcon = "🎯";
        else if (event.jiraType == "Task")
            typeIcon = "✅";

        details += QString("<b>Type:</b> %1 %2<br>").arg(typeIcon).arg(event.jiraType);
    }

    // Status
    if (!event.jiraStatus.isEmpty())
    {
        QString statusColor = "black";
        if (event.jiraStatus == "Done")
            statusColor = "green";
        else if (event.jiraStatus == "In Progress")
            statusColor = "blue";

        details += QString("<b>Status:</b> <span style='color:%1;'>%2</span><br><br>")
                       .arg(statusColor)
                       .arg(event.jiraStatus);
    }

    // Date Range
    details += QString("<b>Timeline:</b><br>");
    details += QString("Start: %1<br>").arg(event.startDate.toString("yyyy-MM-dd"));
    details += QString("Due: %1<br>").arg(event.endDate.toString("yyyy-MM-dd"));

    // Calculate time remaining
    QDate today = QDate::currentDate();
    int daysRemaining = today.daysTo(event.endDate);

    if (daysRemaining > 0)
    {
        details += QString("<b>Days Remaining:</b> %1<br>").arg(daysRemaining);
    }
    else if (daysRemaining < 0)
    {
        details += QString("<b style='color:red;'>Overdue by %1 days</b><br>").arg(-daysRemaining);
    }
    else
    {
        details += QString("<b style='color:orange;'>Due today!</b><br>");
    }

    ui->datesValueLabel->setText(details);
}


void TimelineSidePanel::displayGenericDetails(const TimelineEvent& event)
{
    // Fallback for any event type without specific handler
    QString details;

    details += QString("<b>Dates:</b><br>");
    details += QString("%1 to %2<br>")
                   .arg(event.startDate.toString("yyyy-MM-dd"))
                   .arg(event.endDate.toString("yyyy-MM-dd"));

    if (event.startDate.daysTo(event.endDate) > 0)
    {
        details += QString("Duration: %1 days<br>")
        .arg(event.startDate.daysTo(event.endDate) + 1);
    }

    ui->datesValueLabel->setText(details);
}





void TimelineSidePanel::updateEventDetails(const TimelineEvent& event)
{
    ui->titleValueLabel->setText(event.title);

    QString typeText;

    switch (event.type)
    {
    case TimelineEventType_Meeting: typeText = "Meeting"; break;
    case TimelineEventType_Action: typeText = "Action"; break;
    case TimelineEventType_TestEvent: typeText = "Test Event"; break;
    case TimelineEventType_Reminder: typeText = "Reminder"; break;
    case TimelineEventType_JiraTicket: typeText = "Jira Ticket"; break;
    default: typeText = "Unknown"; break;
    }
    ui->typeValueLabel->setText(typeText);

    ui->priorityValueLabel->setText(QString::number(event.priority));
    ui->laneValueLabel->setText(QString::number(event.lane));

    // Build combined details text
    QString detailsText;

    // Add date range
    QString dateRange;
    if (event.startDate == event.endDate)
    {
        dateRange = event.startDate.toString("MMM dd, yyyy");
    }
    else
    {
        dateRange = QString("%1 to %2")
        .arg(event.startDate.toString("MMM dd, yyyy"))
            .arg(event.endDate.toString("MMM dd, yyyy"));
    }
    detailsText = QString("<b>Date Range:</b><br>%1").arg(dateRange);

    // Add description if present
    if (!event.description.isEmpty())
    {
        detailsText += QString("\n\n<b>Description:</b><br>%1").arg(event.description);
    }

    ui->detailsTextEdit->setHtml(detailsText);

    ui->eventDetailsGroupBox->setVisible(true);
}


void TimelineSidePanel::clearEventDetails()
{
    ui->titleValueLabel->clear();
    ui->typeValueLabel->clear();
    ui->priorityValueLabel->clear();
    ui->laneValueLabel->clear();
    ui->detailsTextEdit->clear();

    ui->eventDetailsGroupBox->setVisible(false);
}



void TimelineSidePanel::onEventAdded(const QString& /*eventId*/)
{
    refreshAllTabs();
}


void TimelineSidePanel::onEventRemoved(const QString& /*eventId*/)
{
    refreshAllTabs();
}


void TimelineSidePanel::onEventUpdated(const QString& /*eventId*/)
{
    refreshAllTabs();
}


void TimelineSidePanel::onLanesRecalculated()
{
    refreshAllTabs();
}


void TimelineSidePanel::onAllEventsItemClicked(QListWidgetItem* item)
{
    if (item && item->flags() != Qt::NoItemFlags)
    {
        QString eventId = item->data(Qt::UserRole).toString();

        if (!eventId.isEmpty())
        {
            displayEventDetails(eventId);
            emit eventSelected(eventId);
        }
    }
}


void TimelineSidePanel::onLookaheadItemClicked(QListWidgetItem* item)
{
    if (item && item->flags() != Qt::NoItemFlags)
    {
        QString eventId = item->data(Qt::UserRole).toString();
        if (!eventId.isEmpty())
        {
            displayEventDetails(eventId);
            emit eventSelected(eventId);
        }
    }
}


void TimelineSidePanel::onTodayItemClicked(QListWidgetItem* item)
{
    if (item && item->flags() != Qt::NoItemFlags)
    {
        QString eventId = item->data(Qt::UserRole).toString();

        if (!eventId.isEmpty())
        {
            displayEventDetails(eventId);
            emit eventSelected(eventId);
        }
    }
}


void TimelineSidePanel::setupListWidgetContextMenu(QListWidget* listWidget)
{
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(listWidget, &QListWidget::customContextMenuRequested,
            this, &TimelineSidePanel::onListContextMenuRequested);
}


void TimelineSidePanel::onListContextMenuRequested(const QPoint& pos)
{
    QListWidget* listWidget = qobject_cast<QListWidget*>(sender());

    if (!listWidget)
    {
        return;
    }

    showListItemContextMenu(listWidget, pos);
}


void TimelineSidePanel::showListItemContextMenu(QListWidget* listWidget, const QPoint& pos)
{
    QListWidgetItem* item = listWidget->itemAt(pos);

    if (!item || item->flags() == Qt::NoItemFlags)
    {
        return;
    }

    QString eventId = item->data(Qt::UserRole).toString();

    if (eventId.isEmpty())
    {
        return;
    }

    QMenu menu(this);
    QAction* editAction = menu.addAction("✏️ Edit Event");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("🗑️ Delete Event");

    QFont deleteFont = deleteAction->font();
    deleteFont.setBold(true);
    deleteAction->setFont(deleteFont);

    QAction* selected = menu.exec(listWidget->mapToGlobal(pos));

    if (selected == editAction)
    {
        emit editEventRequested(eventId);
    }
    else if (selected == deleteAction)
    {
        emit deleteEventRequested(eventId);
    }
}


void TimelineSidePanel::enableMultiSelection(QListWidget* listWidget)
{
    if (!listWidget)
    {
        return;
    }

    listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
}


QStringList TimelineSidePanel::getSelectedEventIds(QListWidget* listWidget) const
{
    QStringList eventIds;

    if (!listWidget)
    {
        return eventIds;
    }

    QList<QListWidgetItem*> selectedItems = listWidget->selectedItems();

    for (QListWidgetItem* item : selectedItems)
    {
        if (item && item->flags() != Qt::NoItemFlags)
        {
            QString eventId = item->data(Qt::UserRole).toString();
            if (!eventId.isEmpty())
            {
                eventIds.append(eventId);
            }
        }
    }

    return eventIds;
}


QStringList TimelineSidePanel::getSelectedEventIds() const
{
    QStringList eventIds;

    QWidget* currentWidget = ui->tabWidget->currentWidget();

    QListWidget* activeList = nullptr;

    if (currentWidget == ui->todayTab)
    {
        activeList = ui->todayList;
    }
    else if (currentWidget == ui->lookaheadTab)
    {
        activeList = ui->lookaheadList;
    }
    else if (currentWidget == ui->allEventsTab)
    {
        activeList = ui->allEventsList;
    }

    if (activeList)
    {
        eventIds = getSelectedEventIds(activeList);
    }

    return eventIds;
}


void TimelineSidePanel::onListSelectionChanged()
{
    emit selectionChanged();
}


void TimelineSidePanel::focusNextItem(QListWidget* listWidget, int deletedRow)
{
    if (!listWidget || listWidget->count() == 0)
    {
        return;
    }

    int nextRow = deletedRow;

    if (nextRow >= listWidget->count())
    {
        nextRow = listWidget->count() - 1;
    }

    if (nextRow >= 0 && nextRow < listWidget->count())
    {
        QListWidgetItem* nextItem = listWidget->item(nextRow);
        if (nextItem)
        {
            listWidget->setCurrentItem(nextItem);
            nextItem->setSelected(true);

            QString eventId = nextItem->data(Qt::UserRole).toString();
            if (!eventId.isEmpty())
            {
                displayEventDetails(eventId);
            }
        }
    }
}


void TimelineSidePanel::toggleFilter(TimelineEventType type)
{
    if (activeFilterTypes_.contains(type))
    {
        activeFilterTypes_.remove(type);
    }
    else
    {
        activeFilterTypes_.insert(type);
    }

    // Save to settings
    TimelineSettings::instance().setSidePanelFilterTypes(activeFilterTypes_);

    // Refresh current tab
    onRefreshTab();
}


void TimelineSidePanel::onRefreshTab()
{
    // Refresh currently active tab
    QWidget* currentWidget = ui->tabWidget->currentWidget();

    if (currentWidget == ui->todayTab)
    {
        refreshTodayTab();
    }
    else if (currentWidget == ui->lookaheadTab)
    {
        refreshLookaheadTab();
    }
    else if (currentWidget == ui->allEventsTab)
    {
        refreshAllEventsTab();
    }
}


void TimelineSidePanel::onSortByDate()
{
    currentSortMode_ = TimelineSettings::SortMode::ByDate;
    TimelineSettings::instance().setSidePanelSortMode(currentSortMode_);
    refreshAllTabs();
}


void TimelineSidePanel::onSortByPriority()
{
    currentSortMode_ = TimelineSettings::SortMode::ByPriority;
    TimelineSettings::instance().setSidePanelSortMode(currentSortMode_);
    refreshAllTabs();
}


void TimelineSidePanel::onSortByType()
{
    currentSortMode_ = TimelineSettings::SortMode::ByType;
    TimelineSettings::instance().setSidePanelSortMode(currentSortMode_);
    refreshAllTabs();
}


void TimelineSidePanel::onFilterByType()
{
    // Show dialog to select multiple types at once
    QDialog dialog(this);
    dialog.setWindowTitle("Filter Events by Type");
    dialog.setMinimumWidth(300);

    auto layout = new QVBoxLayout(&dialog);

    layout->addWidget(new QLabel("Select event types to display:"));

    QCheckBox* meetingCheck = new QCheckBox("Meeting");
    QCheckBox* actionCheck = new QCheckBox("Action");
    QCheckBox* testCheck = new QCheckBox("Test Event");
    QCheckBox* reminderCheck = new QCheckBox("Reminder");
    QCheckBox* jiraCheck = new QCheckBox("Jira Ticket");

    meetingCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_Meeting));
    actionCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_Action));
    testCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_TestEvent));
    reminderCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_Reminder));
    jiraCheck->setChecked(activeFilterTypes_.contains(TimelineEventType_JiraTicket));

    layout->addWidget(meetingCheck);
    layout->addWidget(actionCheck);
    layout->addWidget(testCheck);
    layout->addWidget(reminderCheck);
    layout->addWidget(jiraCheck);

    layout->addStretch();

    auto buttonLayout = new QHBoxLayout();
    auto selectAllButton = new QPushButton("Select All");
    auto clearAllButton = new QPushButton("Clear All");
    auto okButton = new QPushButton("OK");
    auto cancelButton = new QPushButton("Cancel");

    buttonLayout->addWidget(selectAllButton);
    buttonLayout->addWidget(clearAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    connect(selectAllButton, &QPushButton::clicked, [&]() {
        meetingCheck->setChecked(true);
        actionCheck->setChecked(true);
        testCheck->setChecked(true);
        reminderCheck->setChecked(true);
        jiraCheck->setChecked(true);
    });

    connect(clearAllButton, &QPushButton::clicked, [&]() {
        meetingCheck->setChecked(false);
        actionCheck->setChecked(false);
        testCheck->setChecked(false);
        reminderCheck->setChecked(false);
        jiraCheck->setChecked(false);
    });

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        activeFilterTypes_.clear();

        if (meetingCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_Meeting);
        if (actionCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_Action);
        if (testCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_TestEvent);
        if (reminderCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_Reminder);
        if (jiraCheck->isChecked()) activeFilterTypes_.insert(TimelineEventType_JiraTicket);

        // Save to settings
        TimelineSettings::instance().setSidePanelFilterTypes(activeFilterTypes_);

        // Refresh all tabs
        refreshAllTabs();
    }
}

void TimelineSidePanel::onSelectAllEvents()
{
    QWidget* currentWidget = ui->tabWidget->currentWidget();

    QListWidget* activeList = nullptr;

    if (currentWidget == ui->todayTab)
    {
        activeList = ui->todayList;
    }
    else if (currentWidget == ui->lookaheadTab)
    {
        activeList = ui->lookaheadList;
    }
    else if (currentWidget == ui->allEventsTab)
    {
        activeList = ui->allEventsList;
    }

    if (activeList)
    {
        activeList->selectAll();
    }
}


void TimelineSidePanel::onCopyEventDetails()
{
    QStringList selectedIds = getSelectedEventIds();

    if (selectedIds.isEmpty())
    {
        return;
    }

    QStringList eventDetails;

    for (const QString& eventId : selectedIds)
    {
        const TimelineEvent* event = model_->getEvent(eventId);
        if (event)
        {
            QString details = QString(
                                  "Title: %1\n"
                                  "Type: %2\n"
                                  "Dates: %3 to %4\n"
                                  "Priority: %5\n"
                                  "Lane: %6\n"
                                  "Description: %7\n"
                                  ).arg(event->title)
                                  .arg(eventTypeToString(event->type))
                                  .arg(event->startDate.toString("yyyy-MM-dd"))
                                  .arg(event->endDate.toString("yyyy-MM-dd"))
                                  .arg(event->priority)
                                  .arg(event->lane)
                                  .arg(event->description.isEmpty() ? "(No description)" : event->description);

            eventDetails.append(details);
        }
    }

    QString fullDetails = eventDetails.join("\n---\n");
    QGuiApplication::clipboard()->setText(fullDetails);
}


void TimelineSidePanel::sortEvents(QVector<TimelineEvent>& events) const
{
    switch (currentSortMode_)
    {
    case TimelineSettings::SortMode::ByDate:
        std::sort(events.begin(), events.end(),
                  [](const TimelineEvent& a, const TimelineEvent& b) {
                      return a.startDate < b.startDate;
                  });
        break;

    case TimelineSettings::SortMode::ByPriority:
        std::sort(events.begin(), events.end(),
                  [](const TimelineEvent& a, const TimelineEvent& b) {
                      if (a.priority != b.priority)
                      {
                          return a.priority < b.priority; // Lower number = higher priority
                      }
                      return a.startDate < b.startDate; // Tie-breaker
                  });
        break;

    case TimelineSettings::SortMode::ByType:
        std::sort(events.begin(), events.end(),
                  [](const TimelineEvent& a, const TimelineEvent& b) {
                      if (a.type != b.type)
                      {
                          return a.type < b.type;
                      }
                      return a.startDate < b.startDate; // Tie-breaker
                  });
        break;
    }
}

QVector<TimelineEvent> TimelineSidePanel::filterEvents(const QVector<TimelineEvent>& events) const
{
    QVector<TimelineEvent> filtered;

    for (const auto& event : events)
    {
        if (activeFilterTypes_.contains(event.type))
        {
            filtered.append(event);
        }
    }

    return filtered;
}


QVector<TimelineEvent> TimelineSidePanel::applySortAndFilter(const QVector<TimelineEvent>& events) const
{
    // First filter
    QVector<TimelineEvent> result = filterEvents(events);

    // Then sort
    sortEvents(result);

    return result;
}


QString TimelineSidePanel::sortModeToString(TimelineSettings::SortMode mode) const
{
    switch (mode)
    {
    case TimelineSettings::SortMode::ByDate: return "Date";
    case TimelineSettings::SortMode::ByPriority: return "Priority";
    case TimelineSettings::SortMode::ByType: return "Type";
    default: return "Unknown";
    }
}


QString TimelineSidePanel::eventTypeToString(TimelineEventType type) const
{
    switch (type)
    {
    case TimelineEventType_Meeting:
        return "Meeting";
    case TimelineEventType_Action:
        return "Action";
    case TimelineEventType_TestEvent:
        return "Test Event";
    case TimelineEventType_Reminder:
        return "Reminder";
    case TimelineEventType_JiraTicket:
        return "Jira Ticket";
    default:
        return "Unknown";
    }
}
