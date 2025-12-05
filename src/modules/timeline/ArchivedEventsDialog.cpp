// ArchivedEventsDialog.cpp

#include "ArchivedEventsDialog.h"
#include "ui_ArchivedEventsDialog.h"
#include "TimelineModel.h"
#include "TimelineCommands.h"
#include <QUndoStack>
#include <QListWidget>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <algorithm>

ArchivedEventsDialog::ArchivedEventsDialog(TimelineModel* model,
                                           QUndoStack* undoStack,
                                           QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ArchivedEventsDialog)
    , model_(model)
    , undoStack_(undoStack)
{
    // Initialize UI from Qt Designer file
    ui->setupUi(this);

    // Set splitter sizes (70% list, 30% details)
    // Note: Splitter sizes are set as a list of integers representing pixel widths
    QList<int> sizes;
    sizes << 560 << 240;  // 70% of 800px = 560, 30% = 240
    ui->splitter->setSizes(sizes);

    // Initially hide details panel until an event is selected
    ui->detailsGroupBox->setVisible(false);

    // Setup all signal/slot connections
    setupConnections();

    // Load initial data
    refreshList();

    // Update button states based on initial selection (none)
    updateButtonStates();
}

ArchivedEventsDialog::~ArchivedEventsDialog()
{
    delete ui;
}

void ArchivedEventsDialog::setupConnections()
{
    // Model signals - auto-refresh when archive changes
    connect(model_, &TimelineModel::eventArchived,
            this, &ArchivedEventsDialog::onModelChanged);
    connect(model_, &TimelineModel::eventRestored,
            this, &ArchivedEventsDialog::onModelChanged);

    // List widget signals
    connect(ui->archivedEventsList, &QListWidget::itemClicked,
            this, &ArchivedEventsDialog::onEventSelected);
    connect(ui->archivedEventsList, &QListWidget::itemSelectionChanged,
            this, &ArchivedEventsDialog::updateButtonStates);

    // Button signals
    connect(ui->restoreButton, &QPushButton::clicked,
            this, &ArchivedEventsDialog::onRestoreClicked);
    connect(ui->permanentDeleteButton, &QPushButton::clicked,
            this, &ArchivedEventsDialog::onPermanentDeleteClicked);
    connect(ui->closeButton, &QPushButton::clicked,
            this, &ArchivedEventsDialog::onCloseClicked);
}

void ArchivedEventsDialog::refreshList()
{
    // Clear existing items
    ui->archivedEventsList->clear();
    clearEventDetails();

    // Get all archived events from model
    QVector<TimelineEvent> archivedEvents = model_->getAllArchivedEvents();

    if (archivedEvents.isEmpty())
    {
        // Show "no archived events" message
        auto* emptyItem = new QListWidgetItem("No archived events");
        emptyItem->setFlags(Qt::NoItemFlags);  // Non-selectable
        emptyItem->setForeground(Qt::gray);
        ui->archivedEventsList->addItem(emptyItem);
        return;
    }

    // Sort by start date (most recent first)
    std::sort(archivedEvents.begin(), archivedEvents.end(),
              [](const TimelineEvent& a, const TimelineEvent& b) {
                  return a.startDate > b.startDate;
              });

    // Populate list with all archived events
    for (const auto& event : archivedEvents)
    {
        QListWidgetItem* item = createListItem(event.id);
        if (item)
        {
            ui->archivedEventsList->addItem(item);
        }
    }
}

QListWidgetItem* ArchivedEventsDialog::createListItem(const QString& eventId)
{
    const TimelineEvent* event = model_->getArchivedEvent(eventId);
    if (!event)
    {
        return nullptr;
    }

    auto* item = new QListWidgetItem();

    // Set display text (title + date range)
    QString displayText = QString("%1\n%2")
                              .arg(event->title)
                              .arg(formatDateRange(event->startDate, event->endDate));
    item->setText(displayText);

    // Store event ID in UserRole for later retrieval
    item->setData(Qt::UserRole, event->id);

    // Create color indicator icon (16x16 square filled with event color)
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(event->color);
    item->setIcon(QIcon(colorPixmap));

    // Tooltip with full details
    QString tooltip = QString("Title: %1\nType: %2\nDates: %3\nPriority: %4")
                          .arg(event->title)
                          .arg(formatEventType(event->type))
                          .arg(formatDateRange(event->startDate, event->endDate))
                          .arg(event->priority);

    if (!event->description.isEmpty())
    {
        tooltip += "\n\n" + event->description;
    }

    item->setToolTip(tooltip);

    return item;
}

void ArchivedEventsDialog::displayEventDetails(const QString& eventId)
{
    const TimelineEvent* event = model_->getArchivedEvent(eventId);
    if (!event)
    {
        clearEventDetails();
        return;
    }

    // Show details panel
    ui->detailsGroupBox->setVisible(true);

    // Update all detail labels
    ui->titleValueLabel->setText(event->title);
    ui->typeValueLabel->setText(formatEventType(event->type));
    ui->datesValueLabel->setText(formatDateRange(event->startDate, event->endDate));
    ui->priorityValueLabel->setText(QString::number(event->priority));

    // Show "Yes" for archived status
    ui->archivedValueLabel->setText("Yes");

    // Description (may be empty)
    ui->descriptionTextEdit->setPlainText(event->description);
}

void ArchivedEventsDialog::clearEventDetails()
{
    // Clear all detail fields
    ui->titleValueLabel->clear();
    ui->typeValueLabel->clear();
    ui->datesValueLabel->clear();
    ui->priorityValueLabel->clear();
    ui->archivedValueLabel->clear();
    ui->descriptionTextEdit->clear();

    // Hide details panel when no selection
    ui->detailsGroupBox->setVisible(false);
}

void ArchivedEventsDialog::onEventSelected(QListWidgetItem* item)
{
    if (!item || item->flags() == Qt::NoItemFlags)
    {
        clearEventDetails();
        return;
    }

    QString eventId = item->data(Qt::UserRole).toString();
    if (!eventId.isEmpty())
    {
        displayEventDetails(eventId);
    }
}

void ArchivedEventsDialog::onRestoreClicked()
{
    QStringList selectedIds = getSelectedEventIds();
    if (selectedIds.isEmpty())
    {
        return;
    }

    // Build confirmation message
    QString message;
    if (selectedIds.size() == 1)
    {
        const TimelineEvent* event = model_->getArchivedEvent(selectedIds.first());
        message = QString("Restore event '%1' to the timeline?")
                      .arg(event ? event->title : "Unknown");
    }
    else
    {
        message = QString("Restore %1 events to the timeline?")
        .arg(selectedIds.size());
    }

    // Show confirmation dialog
    auto result = QMessageBox::question(
        this,
        "Restore Events",
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
        );

    if (result != QMessageBox::Yes)
    {
        return;
    }

    // Restore each event using undo commands
    for (const QString& eventId : selectedIds)
    {
        auto* command = new RestoreEventCommand(model_, eventId);
        undoStack_->push(command);
    }

    // Success message
    QString statusMessage;
    if (selectedIds.size() == 1)
    {
        statusMessage = "Event restored successfully (Ctrl+Z to undo)";
    }
    else
    {
        statusMessage = QString("%1 events restored successfully (Ctrl+Z to undo)")
        .arg(selectedIds.size());
    }

    QMessageBox::information(this, "Success", statusMessage);

    // List will auto-refresh via onModelChanged() signal connection
}

void ArchivedEventsDialog::onPermanentDeleteClicked()
{
    QStringList selectedIds = getSelectedEventIds();
    if (selectedIds.isEmpty())
    {
        return;
    }

    // Build confirmation message with strong warning
    QString message;
    if (selectedIds.size() == 1)
    {
        const TimelineEvent* event = model_->getArchivedEvent(selectedIds.first());
        message = QString(
                      "Permanently delete event '%1'?\n\n"
                      "⚠️ This action CANNOT be undone!\n\n"
                      "The event will be removed from the archive permanently."
                      ).arg(event ? event->title : "Unknown");
    }
    else
    {
        message = QString(
                      "Permanently delete %1 events?\n\n"
                      "⚠️ This action CANNOT be undone!\n\n"
                      "The events will be removed from the archive permanently."
                      ).arg(selectedIds.size());
    }

    // Show warning dialog with No as default for safety
    auto result = QMessageBox::warning(
        this,
        "Permanent Deletion",
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No  // Default to No for safety
        );

    if (result != QMessageBox::Yes)
    {
        return;
    }

    // Delete each event permanently (no undo support)
    int successCount = 0;
    QStringList failedEvents;

    for (const QString& eventId : selectedIds)
    {
        const TimelineEvent* event = model_->getArchivedEvent(eventId);
        QString eventTitle = event ? event->title : eventId;

        bool success = model_->permanentlyDeleteArchivedEvent(eventId);
        if (success)
        {
            successCount++;
        }
        else
        {
            failedEvents.append(eventTitle);
        }
    }

    // Report results
    if (failedEvents.isEmpty())
    {
        QString message = (selectedIds.size() == 1)
        ? "Event deleted permanently"
        : QString("%1 events deleted permanently").arg(successCount);

        QMessageBox::information(this, "Deleted", message);
    }
    else
    {
        QString message = QString(
                              "Deleted %1 event(s).\n\nFailed to delete:\n%2"
                              ).arg(successCount).arg(failedEvents.join("\n"));

        QMessageBox::warning(this, "Partial Deletion", message);
    }

    // List will auto-refresh via onModelChanged() signal connection
}

void ArchivedEventsDialog::onCloseClicked()
{
    accept();
}

void ArchivedEventsDialog::onModelChanged()
{
    // Refresh list when archive contents change
    refreshList();

    // Update button states (selection may have changed)
    updateButtonStates();
}

void ArchivedEventsDialog::updateButtonStates()
{
    QStringList selectedIds = getSelectedEventIds();
    bool hasSelection = !selectedIds.isEmpty();

    // Enable/disable buttons based on selection
    ui->restoreButton->setEnabled(hasSelection);
    ui->permanentDeleteButton->setEnabled(hasSelection);

    // Update button tooltips with selection count
    if (hasSelection)
    {
        if (selectedIds.size() == 1)
        {
            ui->restoreButton->setToolTip("Restore selected event to timeline");
            ui->permanentDeleteButton->setToolTip(
                "Permanently delete selected event (cannot be undone)");
        }
        else
        {
            ui->restoreButton->setToolTip(
                QString("Restore %1 selected events to timeline")
                    .arg(selectedIds.size()));
            ui->permanentDeleteButton->setToolTip(
                QString("Permanently delete %1 selected events (cannot be undone)")
                    .arg(selectedIds.size()));
        }
    }
    else
    {
        // Reset to default tooltips when nothing selected
        ui->restoreButton->setToolTip("Restore selected event(s) to timeline");
        ui->permanentDeleteButton->setToolTip(
            "Permanently delete selected event(s) (cannot be undone)");
    }
}

QStringList ArchivedEventsDialog::getSelectedEventIds() const
{
    QStringList eventIds;
    QList<QListWidgetItem*> selectedItems = ui->archivedEventsList->selectedItems();

    for (QListWidgetItem* item : selectedItems)
    {
        // Skip the "No archived events" placeholder item
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

QString ArchivedEventsDialog::formatEventType(int eventType) const
{
    switch (eventType)
    {
    case TimelineEventType_Meeting:
        return "Meeting";
    case TimelineEventType_Action:
        return "Action";
    case TimelineEventType_TestEvent:
        return "Test Event";
    case TimelineEventType_DueDate:
        return "Due Date";
    case TimelineEventType_Reminder:
        return "Reminder";
    default:
        return "Unknown";
    }
}

QString ArchivedEventsDialog::formatDateRange(const QDate& startDate, const QDate& endDate) const
{
    if (startDate == endDate)
    {
        // Single day event
        return startDate.toString("MMM dd, yyyy");
    }
    else
    {
        // Multi-day event
        return QString("%1 - %2")
            .arg(startDate.toString("MMM dd"))
            .arg(endDate.toString("MMM dd, yyyy"));
    }
}
