// ArchivedEventsDialog.cpp


#include "ArchivedEventsDialog.h"
#include "ui_ArchivedEventsDialog.h"
#include "TimelineCommands.h"
#include "TimelineModel.h"
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <algorithm>


ArchivedEventsDialog::ArchivedEventsDialog(TimelineModel* model,
                                           QUndoStack* undoStack,
                                           QWidget* parent)
    : QDialog(parent)
    , ui_(new Ui::ArchivedEventsDialog)
    , model_(model)
    , undoStack_(undoStack)
{
    setupUi();
    loadArchivedEvents();
    updateButtonStates();
}


ArchivedEventsDialog::~ArchivedEventsDialog()
{
    delete ui_;
}


void ArchivedEventsDialog::setupUi()
{
    ui_->setupUi(this);

    // Enable multi-selection
    ui_->archivedEventsList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Connect signals
    connect(ui_->restoreButton, &QPushButton::clicked, this, &ArchivedEventsDialog::onRestoreClicked);
    connect(ui_->permanentDeleteButton, &QPushButton::clicked, this, &ArchivedEventsDialog::onPermanentDeleteClicked);
    connect(ui_->closeButton, &QPushButton::clicked, this, &QDialog::accept);

    connect(ui_->archivedEventsList, &QListWidget::itemSelectionChanged, this, &ArchivedEventsDialog::onSelectionChanged);
    connect(ui_->archivedEventsList, &QListWidget::itemDoubleClicked, this, &ArchivedEventsDialog::onItemDoubleClicked);

    // Set initial splitter sizes (60% list, 40% details)
    ui_->splitter->setStretchFactor(0, 6);
    ui_->splitter->setStretchFactor(1, 4);

    // Initially hide details panel
    ui_->detailsGroupBox->setVisible(false);
}


void ArchivedEventsDialog::loadArchivedEvents()
{
    ui_->archivedEventsList->clear();

    QVector<TimelineEvent> archivedEvents = model_->getAllArchivedEvents();

    if (archivedEvents.isEmpty())
    {
        auto emptyItem = new QListWidgetItem("No archived events");
        emptyItem->setFlags(Qt::NoItemFlags);
        emptyItem->setForeground(Qt::gray);
        ui_->archivedEventsList->addItem(emptyItem);
        return;
    }

    // Sort by date (most recent first)
    std::sort(archivedEvents.begin(), archivedEvents.end(),
              [](const TimelineEvent& a, const TimelineEvent& b) {
                  return a.startDate > b.startDate;
              });

    for (const auto& event : archivedEvents)
    {
        QListWidgetItem* item = createListItem(event);
        ui_->archivedEventsList->addItem(item);
    }

    // Update window title with count
    setWindowTitle(QString("Archived Events (%1)").arg(archivedEvents.size()));
}


QListWidgetItem* ArchivedEventsDialog::createListItem(const TimelineEvent& event)
{
    auto item = new QListWidgetItem();
    item->setText(formatEventText(event));
    item->setData(Qt::UserRole, event.id);

    // Create color indicator icon
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(event.color);
    item->setIcon(QIcon(colorPixmap));

    // Tooltip
    QString tooltip = QString("%1\n%2 to %3\nPriority: %4")
                          .arg(event.title)
                          .arg(event.startDate.toString("MMM dd, yyyy"))
                          .arg(event.endDate.toString("MMM dd, yyyy"))
                          .arg(event.priority);

    if (!event.description.isEmpty())
    {
        tooltip += "\n\n" + event.description;
    }

    item->setToolTip(tooltip);

    return item;
}


QString ArchivedEventsDialog::formatEventText(const TimelineEvent& event) const
{
    QString dateRange;
    if (event.startDate == event.endDate)
    {
        dateRange = event.startDate.toString("MMM dd, yyyy");
    }
    else
    {
        dateRange = QString("%1 - %2")
        .arg(event.startDate.toString("MMM dd"))
            .arg(event.endDate.toString("MMM dd, yyyy"));
    }

    return QString("%1\n%2").arg(event.title, dateRange);
}


void ArchivedEventsDialog::displayEventDetails(const QString& eventId)
{
    const TimelineEvent* event = model_->getArchivedEvent(eventId);

    if (!event)
    {
        clearEventDetails();
        return;
    }

    // Update detail labels
    ui_->titleValueLabel->setText(event->title);
    ui_->typeValueLabel->setText(eventTypeToString(event->type));

    QString dateRange;
    if (event->startDate == event->endDate)
    {
        dateRange = event->startDate.toString("MMM dd, yyyy");
    }
    else
    {
        dateRange = QString("%1 to %2")
        .arg(event->startDate.toString("MMM dd, yyyy"))
            .arg(event->endDate.toString("MMM dd, yyyy"));
    }
    ui_->datesValueLabel->setText(dateRange);

    ui_->priorityValueLabel->setText(QString::number(event->priority));
    ui_->archivedValueLabel->setText("Yes");
    ui_->descriptionTextEdit->setPlainText(event->description);

    // Show details panel
    ui_->detailsGroupBox->setVisible(true);
}


void ArchivedEventsDialog::clearEventDetails()
{
    ui_->titleValueLabel->clear();
    ui_->typeValueLabel->clear();
    ui_->datesValueLabel->clear();
    ui_->priorityValueLabel->clear();
    ui_->archivedValueLabel->clear();
    ui_->descriptionTextEdit->clear();

    ui_->detailsGroupBox->setVisible(false);
}


QStringList ArchivedEventsDialog::getSelectedEventIds() const
{
    QStringList eventIds;
    QList<QListWidgetItem*> selectedItems = ui_->archivedEventsList->selectedItems();

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


void ArchivedEventsDialog::onRestoreClicked()
{
    QStringList selectedIds = getSelectedEventIds();

    if (selectedIds.isEmpty())
    {
        QMessageBox::warning(this, "No Selection",
                             "Please select one or more events to restore.");
        return;
    }

    // Confirm restoration
    QString message;
    if (selectedIds.size() == 1)
    {
        const TimelineEvent* event = model_->getArchivedEvent(selectedIds.first());
        message = QString("Restore event '%1' back to the timeline?")
                      .arg(event ? event->title : "Unknown");
    }
    else
    {
        message = QString("Restore %1 events back to the timeline?")
        .arg(selectedIds.size());
    }

    auto result = QMessageBox::question(this, "Confirm Restore", message,
                                        QMessageBox::Yes | QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    // Restore events using undo stack
    for (const QString& eventId : selectedIds)
    {
        undoStack_->push(new RestoreEventCommand(model_, eventId));
    }

    // Reload list
    loadArchivedEvents();
    clearEventDetails();
    updateButtonStates();

    // Show success message
    QString successMsg = selectedIds.size() == 1
                             ? "Event restored successfully"
                             : QString("%1 events restored successfully").arg(selectedIds.size());

    QMessageBox::information(this, "Success", successMsg);
}


void ArchivedEventsDialog::onPermanentDeleteClicked()
{
    QStringList selectedIds = getSelectedEventIds();

    if (selectedIds.isEmpty())
    {
        QMessageBox::warning(this, "No Selection",
                             "Please select one or more events to delete permanently.");
        return;
    }

    // Build event list for confirmation
    QStringList eventTitles;
    for (const QString& eventId : selectedIds)
    {
        const TimelineEvent* event = model_->getArchivedEvent(eventId);
        if (event)
        {
            eventTitles.append(event->title);
        }
    }

    // Confirm permanent deletion
    QString message;
    if (selectedIds.size() == 1)
    {
        message = QString("⚠️ Permanently delete event '%1'?\n\n"
                          "This action CANNOT be undone!")
                      .arg(eventTitles.first());
    }
    else
    {
        message = QString("⚠️ Permanently delete these %1 events?\n\n"
                          "• %2\n\n"
                          "This action CANNOT be undone!")
                      .arg(selectedIds.size())
                      .arg(eventTitles.join("\n• "));
    }

    auto result = QMessageBox::warning(this, "Confirm Permanent Deletion",
                                       message,
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    // Permanently delete events (no undo support)
    int successCount = 0;
    for (const QString& eventId : selectedIds)
    {
        if (model_->permanentlyDeleteArchivedEvent(eventId))
        {
            successCount++;
        }
    }

    // Reload list
    loadArchivedEvents();
    clearEventDetails();
    updateButtonStates();

    // Show result
    if (successCount == selectedIds.size())
    {
        QString successMsg = selectedIds.size() == 1
                                 ? "Event permanently deleted"
                                 : QString("%1 events permanently deleted").arg(successCount);

        QMessageBox::information(this, "Success", successMsg);
    }
    else
    {
        QMessageBox::warning(this, "Partial Success",
                             QString("Deleted %1 of %2 events")
                                 .arg(successCount)
                                 .arg(selectedIds.size()));
    }
}


void ArchivedEventsDialog::onSelectionChanged()
{
    QStringList selectedIds = getSelectedEventIds();

    if (selectedIds.size() == 1)
    {
        // Single selection - show details
        displayEventDetails(selectedIds.first());
    }
    else if (selectedIds.size() > 1)
    {
        // Multi-selection - show count in details
        clearEventDetails();
        ui_->detailsGroupBox->setVisible(true);
        ui_->titleValueLabel->setText(QString("%1 events selected").arg(selectedIds.size()));
    }
    else
    {
        // No selection
        clearEventDetails();
    }

    updateButtonStates();
}


void ArchivedEventsDialog::onItemDoubleClicked(QListWidgetItem* item)
{
    if (!item || item->flags() == Qt::NoItemFlags)
    {
        return;
    }

    QString eventId = item->data(Qt::UserRole).toString();
    if (eventId.isEmpty())
    {
        return;
    }

    // Double-click restores the event
    undoStack_->push(new RestoreEventCommand(model_, eventId));

    loadArchivedEvents();
    clearEventDetails();
    updateButtonStates();
}


void ArchivedEventsDialog::updateButtonStates()
{
    bool hasSelection = !getSelectedEventIds().isEmpty();

    ui_->restoreButton->setEnabled(hasSelection);
    ui_->permanentDeleteButton->setEnabled(hasSelection);
}


QString ArchivedEventsDialog::eventTypeToString(TimelineEventType type) const
{
    switch (type)
    {
    case TimelineEventType_Meeting: return "Meeting";
    case TimelineEventType_Action: return "Action";
    case TimelineEventType_TestEvent: return "Test Event";
    case TimelineEventType_DueDate: return "Due Date";
    case TimelineEventType_Reminder: return "Reminder";
    default: return "Unknown";
    }
}
