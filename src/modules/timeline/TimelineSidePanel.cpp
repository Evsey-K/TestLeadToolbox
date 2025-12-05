// TimelineSidePanel.cpp


#include "TimelineSidePanel.h"
#include "ui_TimelineSidePanel.h"
#include "TimelineModel.h"
#include <QListWidget>
#include <QPixmap>
#include <QPainter>
#include <QDate>
#include <QMenu>
#include <algorithm>


TimelineSidePanel::TimelineSidePanel(TimelineModel* model, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TimelineSidePanel)
    , model_(model)
{
    ui->setupUi(this);

    // Set initial splitter sizes (70% for tabs, 30% for details)
    ui->splitter->setStretchFactor(0, 7);
    ui->splitter->setStretchFactor(1, 3);

    // Hide details panel initially
    ui->eventDetailsGroupBox->setVisible(false);

    // TIER 2: Enable multi-selection on all list widgets
    enableMultiSelection(ui->allEventsList);
    enableMultiSelection(ui->lookaheadList);
    enableMultiSelection(ui->todayList);

    connectSignals();
    refreshAllTabs();
    adjustWidthToFitTabs();

    setupListWidgetContextMenu(ui->allEventsList);
    setupListWidgetContextMenu(ui->lookaheadList);
    setupListWidgetContextMenu(ui->todayList);
}


TimelineSidePanel::~TimelineSidePanel()
{
    // Clean up the UI pointer allocated in the constructor
    delete ui;
}


void TimelineSidePanel::connectSignals()
{
    // Connect to model signals
    connect(model_, &TimelineModel::eventAdded, this, &TimelineSidePanel::onEventAdded);
    connect(model_, &TimelineModel::eventRemoved, this, &TimelineSidePanel::onEventRemoved);
    connect(model_, &TimelineModel::eventUpdated, this, &TimelineSidePanel::onEventUpdated);
    connect(model_, &TimelineModel::lanesRecalculated, this, &TimelineSidePanel::onLanesRecalculated);

    // Connect to list widget click signals
    connect(ui->allEventsList, &QListWidget::itemClicked, this, &TimelineSidePanel::onAllEventsItemClicked);
    connect(ui->lookaheadList, &QListWidget::itemClicked, this, &TimelineSidePanel::onLookaheadItemClicked);
    connect(ui->todayList, &QListWidget::itemClicked, this, &TimelineSidePanel::onTodayItemClicked);

    // TIER 2: Connect selection change signals
    connect(ui->allEventsList, &QListWidget::itemSelectionChanged, this, &TimelineSidePanel::onListSelectionChanged);
    connect(ui->lookaheadList, &QListWidget::itemSelectionChanged, this, &TimelineSidePanel::onListSelectionChanged);
    connect(ui->todayList, &QListWidget::itemSelectionChanged, this, &TimelineSidePanel::onListSelectionChanged);
}


void TimelineSidePanel::refreshAllTabs()
{
    refreshAllEventsTab();
    refreshLookaheadTab();
    refreshTodayTab();
}

void TimelineSidePanel::refreshAllEventsTab()
{
    QVector<TimelineEvent> events = model_->getAllEvents();

    // Sort by start date
    std::sort(events.begin(), events.end(),
              [](const TimelineEvent& a, const TimelineEvent& b)
              {
                  return a.startDate < b.startDate;
              });

    populateListWidget(ui->allEventsList, events);

    // Update tab label with count
    ui->tabWidget->setTabText(2, QString("All Events (%1)").arg(events.size()));

    adjustWidthToFitTabs();
}

void TimelineSidePanel::refreshLookaheadTab()
{
    // Show events in next 2 weeks
    QVector<TimelineEvent> events = model_->getEventsLookahead(14);

    // Sort by start date
    std::sort(events.begin(), events.end(),
              [](const TimelineEvent& a, const TimelineEvent& b)
              {
                  return a.startDate < b.startDate;
              });

    populateListWidget(ui->lookaheadList, events);

    // Update tab label with count
    ui->tabWidget->setTabText(1, QString("Lookahead (%1)").arg(events.size()));

    adjustWidthToFitTabs();
}


void TimelineSidePanel::refreshTodayTab()
{
    // Show today's events
    QVector<TimelineEvent> events = model_->getEventsForToday();

    // Sort by priority (higher priority first), then by start time
    std::sort(events.begin(), events.end(),
              [](const TimelineEvent& a, const TimelineEvent& b)
              {
                  if (a.priority != b.priority)
                  {
                      return a.priority > b.priority; // Higher priority first
                  }
                  return a.startDate < b.startDate;
              });

    populateListWidget(ui->todayList, events);

    // Update tab label with count and today's date
    QString todayLabel = QString("Today (%1) - %2")
                             .arg(events.size())
                             .arg(QDate::currentDate().toString("MMM dd"));

    ui->tabWidget->setTabText(0, todayLabel);

    adjustWidthToFitTabs();
}


void TimelineSidePanel::populateListWidget(QListWidget* listWidget, const QVector<TimelineEvent>& events)
{
    listWidget->clear();

    if (events.isEmpty())
    {
        auto emptyItem = new QListWidgetItem("No events");
        emptyItem->setFlags(Qt::NoItemFlags); // Make it non-selectable
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

    // Create color indicator icon
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(event.color);
    item->setIcon(QIcon(colorPixmap));

    // Add lane information to tooltip
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

    // Include priority indicator for high-priority events
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
    // Get the tab bar to measure actual tab widths
    QTabBar* tabBar = ui->tabWidget->tabBar();
    QFontMetrics fm(tabBar->font());

    int totalWidth = 0;

    // Measure each tab's actual width
    for (int i = 0; i < ui->tabWidget->count(); ++i)
    {
        QString tabText = ui->tabWidget->tabText(i);
        int textWidth = fm.horizontalAdvance(tabText);

        // Add padding per tab (Qt adds ~20-30px per tab for margins/spacing)
        totalWidth += textWidth + 30;
    }

    // Add small margin for tab bar frame and borders (~20px)
    totalWidth += 20;

    // Set minimum width to fit all tabs snugly
    setMinimumWidth(totalWidth);
}


void TimelineSidePanel::displayEventDetails(const QString& eventId)
{
    const TimelineEvent* event = model_->getEvent(eventId);

    if (event)
    {
        updateEventDetails(*event);
    }
    else
    {
        clearEventDetails();
    }
}


void TimelineSidePanel::updateEventDetails(const TimelineEvent& event)
{
    // Update all detail labels
    ui->titleValueLabel->setText(event.title);

    // Convert type enum to string
    QString typeText;
    switch (event.type)
    {
    case TimelineEventType_Meeting: typeText = "Meeting"; break;
    case TimelineEventType_Action: typeText = "Action"; break;
    case TimelineEventType_TestEvent: typeText = "Test Event"; break;
    case TimelineEventType_DueDate: typeText = "Deadline"; break;
    case TimelineEventType_Reminder: typeText = "Reminder"; break;
    default: typeText = "Unknown"; break;
    }
    ui->typeValueLabel->setText(typeText);

    // Format dates
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
    ui->datesValueLabel->setText(dateRange);

    ui->priorityValueLabel->setText(QString::number(event.priority));
    ui->laneValueLabel->setText(QString::number(event.lane));
    ui->descriptionTextEdit->setPlainText(event.description);

    // Make the group box visible
    ui->eventDetailsGroupBox->setVisible(true);
}


void TimelineSidePanel::clearEventDetails()
{
    ui->titleValueLabel->clear();
    ui->typeValueLabel->clear();
    ui->datesValueLabel->clear();
    ui->priorityValueLabel->clear();
    ui->laneValueLabel->clear();
    ui->descriptionTextEdit->clear();

    // Optionally hide the group box when no selection
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
    // SPRINT 2: Refresh all tabs when lanes change (updates lane info in tooltips)
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


// TIER 2 IMPLEMENTATION: Enable multi-selection on list widget
void TimelineSidePanel::enableMultiSelection(QListWidget* listWidget)
{
    if (!listWidget)
    {
        return;
    }

    // Set selection mode to allow Ctrl+Click and Shift+Click
    listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
}


// TIER 2 IMPLEMENTATION: Get selected event IDs from specific list widget
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


// TIER 2 IMPLEMENTATION: Get all selected event IDs from all tabs
QStringList TimelineSidePanel::getSelectedEventIds() const
{
    QStringList eventIds;

    // Get currently active tab
    QWidget* currentWidget = ui->tabWidget->currentWidget();

    // Get the list widget for the active tab
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

    // Only get selection from active tab
    if (activeList)
    {
        eventIds = getSelectedEventIds(activeList);
    }

    return eventIds;
}


// TIER 2 IMPLEMENTATION: Handle selection changes in list widgets
void TimelineSidePanel::onListSelectionChanged()
{
    // Emit signal to notify that selection has changed
    emit selectionChanged();
}


// TIER 2 IMPLEMENTATION: Focus management after deletion
void TimelineSidePanel::focusNextItem(QListWidget* listWidget, int deletedRow)
{
    if (!listWidget || listWidget->count() == 0)
    {
        return;
    }

    // Calculate next item to select
    int nextRow = deletedRow;

    // If deleted the last item, select previous
    if (nextRow >= listWidget->count())
    {
        nextRow = listWidget->count() - 1;
    }

    // Select and focus the next item
    if (nextRow >= 0 && nextRow < listWidget->count())
    {
        QListWidgetItem* nextItem = listWidget->item(nextRow);
        if (nextItem)
        {
            listWidget->setCurrentItem(nextItem);
            nextItem->setSelected(true);

            // Display details for newly focused item
            QString eventId = nextItem->data(Qt::UserRole).toString();
            if (!eventId.isEmpty())
            {
                displayEventDetails(eventId);
            }
        }
    }
}
