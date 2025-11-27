// TimelineSidePanel.cpp


#include "TimelineSidePanel.h"
#include "ui_TimelineSidePanel.h"
#include "TimelineModel.h"
#include <QListWidget>
#include <QPixmap>
#include <QPainter>
#include <algorithm>


TimelineSidePanel::TimelineSidePanel(TimelineModel* model, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TimelineSidePanel)
    , model_(model)
{
    ui->setupUi(this);
    connectSignals();
    refreshList();
}


TimelineSidePanel::~TimelineSidePanel()
{
    delete ui;
}


void TimelineSidePanel::connectSignals()
{
    // Connect to Model signals
    connect(model_, &TimelineModel::eventAdded, this, &TimelineSidePanel::onEventAdded);
    connect(model_, &TimelineModel::eventRemoved, this, &TimelineSidePanel::onEventRemoved);
    connect(model_, &TimelineModel::eventUpdated, this, &TimelineSidePanel::onEventUpdated);

    // Connect to List Widget signals
    connect(ui->listWidget, &QListWidget::itemClicked, this, &TimelineSidePanel::onItemClicked);
}


void TimelineSidePanel::refreshList()
{
    ui->listWidget->clear();

    // Get all events sorted by start date
    QVector<TimelineObject> events = model_->allEvents();
    std::sort(events.begin(), events.end(),
              [](const TimelineObject& a, const TimelineObject& b)
              {
                return a.startDate < b.startDate;
              });

    // Add each event to the list
    for (const auto& event : events)
    {
        addEventToList(event.id);
    }
}


void TimelineSidePanel::onEventAdded(const QString& eventId)
{
    addEventToList(eventId);
}


void TimelineSidePanel::onEventRemoved(const QString& eventId)
{
    removeEventFromList(eventId);
}


void TimelineSidePanel::onEventUpdated(const QString& eventId)
{
    updateEventInList(eventId);
}


void TimelineSidePanel::onItemClicked(QListWidgetItem* item)
{
    if (item)
    {
        QString eventId = item->data(Qt::UserRole).toString();
        emit eventSelected(eventId);
    }
}


void TimelineSidePanel::addEventToList(const QString& eventId)
{
    const TimelineObject* event = model_->findEvent(eventId);
    if(!event)
    {
        return;
    }

    // Create list item
    auto item = new QListWidgetItem();
    item->setText(formatEventText(eventId));
    item->setData(Qt::UserRole, eventId);

    // Create color indicator icon
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(event->color);
    item->setIcon(QIcon(colorPixmap));

    // Insert in sorted position by date
    int insertPos = 0;
    for (int i = 0; i < ui->listWidget->count(); ++i)
    {
        QString existingId = ui->listWidget->item(i)->data(Qt::UserRole).toString();
        const TimelineObject* existing = model_->findEvent(existingId);

        if (existing && existing->startDate > event->startDate)
        {
            break;
        }
        insertPos = i + 1;
    }

    ui->listWidget->insertItem(insertPos, item);
}


void TimelineSidePanel::removeEventFromList(const QString& eventId)
{
    QListWidgetItem* item = findListItem(eventId);

    if (item)
    {
        ui->listWidget->takeItem(ui->listWidget->row(item));
    }
}


void TimelineSidePanel::updateEventInList(const QString& eventId)
{
    QListWidgetItem* item = findListItem(eventId);
    if (item)
    {
        // Update text
        item->setText(formatEventText(eventId));

        // Update color
        const TimelineObject* event = model_->findEvent(eventId);

        if (event)
        {
            QPixmap colorPixmap(16, 16);
            colorPixmap.fill(event->color);
            item->setIcon(QIcon(colorPixmap));
        }

        // Restore list (remove and re-add)
        removeEventFromList(eventId);
        addEventToList(eventId);
    }
}


QListWidgetItem* TimelineSidePanel::findListItem(const QString& eventId) const
{
    for (int i = 0; i < ui->listWidget->count(); ++i)
    {
        QListWidgetItem* item = ui->listWidget->item(i);

        if (item->data(Qt::UserRole).toString() == eventId)
        {
            return item;
        }
    }

    return nullptr;
}


QString TimelineSidePanel::formatEventText(const QString& eventId) const
{
    const TimelineObject* event = model_->findEvent(eventId);
    if (!event)
    {
        return QString();
    }

    QString dateRange;
    if (event->startDate == event->endDate)
    {
        dateRange = event->startDate.toString("MMM dd");
    }
    else
    {
        dateRange = QString("%1 - %2")
        .arg(event->startDate.toString("MMM dd"))
            .arg(event->endDate.toString("MMM dd"));
    }

    return QString("%1\n%2").arg(event->title, dateRange);
}
