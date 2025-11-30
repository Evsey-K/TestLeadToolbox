// TimelineSidePanel.cpp


#include "TimelineSidePanel.h"
#include "ui_TimelineSidePanel.h"
#include "TimelineModel.h"
#include <QListWidget>
#include <QPixmap>
#include <QPainter>
#include <QDate>
#include <algorithm>


TimelineSidePanel::TimelineSidePanel(TimelineModel* model, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TimelineSidePanel)
    , model_(model)
{
    ui->setupUi(this);

    connectSignals();
    refreshAllTabs();
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

    ui->tabWidget->setTabText(2, todayLabel);
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
    QString priorityIndicator = (event.priority >= 4) ? "🔴 " : "";

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
            emit eventSelected(eventId);
        }
    }
}




