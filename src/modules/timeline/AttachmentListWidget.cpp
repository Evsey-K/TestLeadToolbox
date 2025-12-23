// AttachmentListWidget.cpp


#include "AttachmentListWidget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QDebug>


AttachmentListWidget::AttachmentListWidget(QWidget* parent)
    : QWidget(parent)
    , eventId_("")
    , numericEventId_(-1)
{
    setupUI();
    setAcceptDrops(true);
}


AttachmentListWidget::~AttachmentListWidget()
{
    // Explicitly disconnect from AttachmentManager singleton to prevent
    // crashes when the dialog is destroyed while signals might still be pending
    disconnect(&AttachmentManager::instance(), nullptr, this, nullptr);

    qDebug() << "AttachmentListWidget destroyed for event:" << eventId_;
}


void AttachmentListWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(5);

    // List widget with controlled height
    listWidget_ = new QListWidget();
    listWidget_->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    listWidget_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listWidget_->setAlternatingRowColors(true);
    listWidget_->setDragDropMode(QAbstractItemView::NoDragDrop);

    // Set minimum and maximum heights for the list
    listWidget_->setMinimumHeight(80);
    listWidget_->setMaximumHeight(150);

    mainLayout->addWidget(listWidget_);

    // Button row - using icon-only buttons for compact display
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(5);

    addButton_ = new QPushButton();
    addButton_->setIcon(QIcon::fromTheme("list-add", style()->standardIcon(QStyle::SP_FileDialogNewFolder)));
    addButton_->setToolTip("Add attachment from file system");
    addButton_->setMaximumWidth(32);
    buttonLayout->addWidget(addButton_);

    removeButton_ = new QPushButton();
    removeButton_->setIcon(QIcon::fromTheme("list-remove", style()->standardIcon(QStyle::SP_TrashIcon)));
    removeButton_->setToolTip("Remove selected attachment(s)");
    removeButton_->setEnabled(false);
    removeButton_->setMaximumWidth(32);
    buttonLayout->addWidget(removeButton_);

    openButton_ = new QPushButton();
    openButton_->setIcon(QIcon::fromTheme("document-open", style()->standardIcon(QStyle::SP_FileDialogContentsView)));
    openButton_->setToolTip("Open selected attachment with default application");
    openButton_->setEnabled(false);
    openButton_->setMaximumWidth(32);
    buttonLayout->addWidget(openButton_);

    revealButton_ = new QPushButton();
    revealButton_->setIcon(QIcon::fromTheme("folder-open", style()->standardIcon(QStyle::SP_DirIcon)));
    revealButton_->setToolTip("Reveal attachment location in file explorer");
    revealButton_->setEnabled(false);
    revealButton_->setMaximumWidth(32);
    buttonLayout->addWidget(revealButton_);

    buttonLayout->addStretch();

    statusLabel_ = new QLabel("No attachments");
    statusLabel_->setStyleSheet("QLabel { color: #666; font-style: italic; font-size: 9pt; }");
    buttonLayout->addWidget(statusLabel_);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(addButton_, &QPushButton::clicked, this, &AttachmentListWidget::onAddClicked);
    connect(removeButton_, &QPushButton::clicked, this, &AttachmentListWidget::onRemoveClicked);
    connect(openButton_, &QPushButton::clicked, this, &AttachmentListWidget::onOpenClicked);
    connect(revealButton_, &QPushButton::clicked, this, &AttachmentListWidget::onRevealClicked);
    connect(listWidget_, &QListWidget::itemDoubleClicked, this, &AttachmentListWidget::onItemDoubleClicked);
    connect(listWidget_, &QListWidget::itemSelectionChanged, this, &AttachmentListWidget::onSelectionChanged);
}


void AttachmentListWidget::setEventId(const QString& eventId)
{
    eventId_ = eventId;
    numericEventId_ = qHash(eventId);

    qDebug() << "AttachmentListWidget: Set event ID:" << eventId_
             << "-> hash:" << numericEventId_;

    refresh();
}


void AttachmentListWidget::refresh()
{
    qDebug() << "╔════════════════════════════════════════════════════════════";
    qDebug() << "║ AttachmentListWidget::refresh() called";
    qDebug() << "║ eventId_:" << eventId_;
    qDebug() << "║ numericEventId_:" << numericEventId_;
    qDebug() << "║ isEmpty():" << eventId_.isEmpty();
    qDebug() << "║ numericEventId_ < 0:" << (numericEventId_ < 0);

    listWidget_->clear();

    if (eventId_.isEmpty() || numericEventId_ == 0)
    {
        qDebug() << "║ EARLY RETURN: eventId is empty or numericEventId == 0";
        qDebug() << "╚════════════════════════════════════════════════════════════";

        statusLabel_->setText("No event selected");
        updateButtons();
        return;
    }

    qDebug() << "║ Fetching attachments from AttachmentManager...";

    // Get attachments from AttachmentManager
    QList<Attachment> attachments = AttachmentManager::instance().getAttachments(numericEventId_);

    qDebug() << "║ Found" << attachments.size() << "attachment(s)";

    if (attachments.isEmpty())
    {
        qDebug() << "║ No attachments found";
        qDebug() << "╚════════════════════════════════════════════════════════════";

        statusLabel_->setText("No attachments");
        updateButtons();
        return;
    }

    // Add each attachment to the list
    for (int i = 0; i < attachments.size(); ++i)
    {
        addAttachmentToList(attachments[i], i);
    }

    statusLabel_->setText(QString("%1 attachment(s)").arg(attachments.size()));
    updateButtons();

    qDebug() << "║ AttachmentListWidget: Successfully loaded" << attachments.size() << "attachment(s)";
    qDebug() << "╚════════════════════════════════════════════════════════════";
}


void AttachmentListWidget::clear()
{
    eventId_.clear();
    numericEventId_ = 0;
    listWidget_->clear();
    statusLabel_->setText("No attachments");
    updateButtons();
}


void AttachmentListWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls() && numericEventId_ != 0)
    {
        event->acceptProposedAction();
        qDebug() << "AttachmentListWidget: Drag enter with files";
    }
    else
    {
        event->ignore();
    }
}


void AttachmentListWidget::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasUrls() || numericEventId_ == 0)
    {
        event->ignore();
        return;
    }

    // Extract file paths
    QStringList filePaths;
    for (const QUrl& url : event->mimeData()->urls())
    {
        if (url.isLocalFile())
        {
            filePaths.append(url.toLocalFile());
        }
    }

    if (filePaths.isEmpty())
    {
        event->ignore();
        return;
    }

    qDebug() << "AttachmentListWidget: Dropped" << filePaths.size() << "file(s)";

    // Add attachments
    QStringList errors;
    bool anySuccess = AttachmentManager::instance().addMultipleAttachments(
        numericEventId_,
        filePaths,
        AttachmentStorageMode::InlineEmbedded,
        errors
        );

    // Show feedback
    if (anySuccess && errors.isEmpty())
    {
        qDebug() << "AttachmentListWidget: Successfully added all files";
    }
    else if (!errors.isEmpty())
    {
        QString message = "Failed to add some files:\n\n" + errors.join("\n");
        QMessageBox::warning(this, "Attachment Error", message);
    }

    // Refresh list
    refresh();
    emit attachmentsChanged();

    event->acceptProposedAction();
}


void AttachmentListWidget::onAddClicked()
{
    if (numericEventId_ == 0)
    {
        QMessageBox::warning(this, "No Event", "No event selected. Cannot add attachments.");
        return;
    }

    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Add Attachments",
        QDir::homePath(),
        "All Files (*.*)"
        );

    if (filePaths.isEmpty())
    {
        return;
    }

    qDebug() << "AttachmentListWidget: Adding" << filePaths.size() << "file(s)";

    QStringList errors;
    bool anySuccess = AttachmentManager::instance().addMultipleAttachments(
        numericEventId_,
        filePaths,
        AttachmentStorageMode::InlineEmbedded,
        errors
        );

    if (anySuccess && errors.isEmpty())
    {
        qDebug() << "AttachmentListWidget: Successfully added all files";
    }
    else if (!errors.isEmpty())
    {
        QString message = "Failed to add some files:\n\n" + errors.join("\n");
        QMessageBox::warning(this, "Attachment Error", message);
    }

    refresh();
    emit attachmentsChanged();
}


void AttachmentListWidget::onRemoveClicked()
{
    QList<QListWidgetItem*> selectedItems = listWidget_->selectedItems();

    if (selectedItems.isEmpty())
    {
        return;
    }

    // Confirm deletion
    QString message = (selectedItems.size() == 1)
                          ? QString("Remove attachment \"%1\"?").arg(selectedItems[0]->text())
                          : QString("Remove %1 attachments?").arg(selectedItems.size());

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Removal",
        message,
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply != QMessageBox::Yes)
    {
        return;
    }

    // Get indices to remove (sorted in reverse order to avoid index shifting)
    QList<int> indices;
    for (QListWidgetItem* item : selectedItems)
    {
        indices.append(listWidget_->row(item));
    }
    std::sort(indices.begin(), indices.end(), std::greater<int>());

    // Remove attachments
    for (int index : indices)
    {
        bool success = AttachmentManager::instance().removeAttachment(numericEventId_, index);
        if (!success)
        {
            qWarning() << "Failed to remove attachment at index" << index;
        }
    }

    refresh();
    emit attachmentsChanged();
}


void AttachmentListWidget::onOpenClicked()
{
    QList<QListWidgetItem*> selectedItems = listWidget_->selectedItems();

    if (selectedItems.isEmpty())
    {
        return;
    }

    int index = listWidget_->row(selectedItems[0]);
    QList<Attachment> attachments = AttachmentManager::instance().getAttachments(numericEventId_);

    if (index >= 0 && index < attachments.size())
    {
        bool success = AttachmentManager::instance().openAttachment(attachments[index]);
        if (!success)
        {
            QMessageBox::warning(this, "Open Failed",
                                 "Failed to open attachment. File may not exist.");
        }
    }
}


void AttachmentListWidget::onRevealClicked()
{
    QList<QListWidgetItem*> selectedItems = listWidget_->selectedItems();

    if (selectedItems.isEmpty())
    {
        return;
    }

    int index = listWidget_->row(selectedItems[0]);
    QList<Attachment> attachments = AttachmentManager::instance().getAttachments(numericEventId_);

    if (index >= 0 && index < attachments.size())
    {
        bool success = AttachmentManager::instance().revealInExplorer(attachments[index]);
        if (!success)
        {
            QMessageBox::warning(this, "Reveal Failed",
                                 "Failed to reveal attachment in file explorer.");
        }
    }
}

void AttachmentListWidget::onItemDoubleClicked(QListWidgetItem* item)
{
    if (!item)
    {
        return;
    }

    int index = listWidget_->row(item);
    QList<Attachment> attachments = AttachmentManager::instance().getAttachments(numericEventId_);

    if (index >= 0 && index < attachments.size())
    {
        AttachmentManager::instance().openAttachment(attachments[index]);
    }
}


void AttachmentListWidget::onSelectionChanged()
{
    updateButtons();
}


void AttachmentListWidget::updateButtons()
{
    bool hasSelection = !listWidget_->selectedItems().isEmpty();
    bool hasSingleSelection = listWidget_->selectedItems().size() == 1;

    removeButton_->setEnabled(hasSelection);
    openButton_->setEnabled(hasSingleSelection);
    revealButton_->setEnabled(hasSingleSelection);
}


void AttachmentListWidget::addAttachmentToList(const Attachment& attachment, int index)
{
    QListWidgetItem* item = new QListWidgetItem();

    // Set icon
    item->setIcon(attachment.icon());

    // Set text: filename + size
    QString displayText = QString("%1 (%2)")
                              .arg(attachment.displayName)
                              .arg(attachment.sizeString());
    item->setText(displayText);

    // Set tooltip with full details
    QString tooltip = QString("File: %1\nSize: %2\nType: %3\nAdded: %4")
                          .arg(attachment.filePath)
                          .arg(attachment.sizeString())
                          .arg(attachment.fileType.toUpper())
                          .arg(attachment.attachedDate.toString("yyyy-MM-dd HH:mm"));

    if (!attachment.notes.isEmpty())
    {
        tooltip += QString("\nNotes: %1").arg(attachment.notes);
    }

    if (!attachment.exists())
    {
        tooltip += "\n⚠ WARNING: File no longer exists!";
        item->setForeground(QBrush(QColor(200, 0, 0)));  // Red text for missing files
    }

    item->setToolTip(tooltip);

    // Store index as user data
    item->setData(Qt::UserRole, index);

    listWidget_->addItem(item);
}
