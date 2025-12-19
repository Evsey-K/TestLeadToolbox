// AttachmentListWidget.cpp


#include "AttachmentListWidget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QCheckBox>
#include <QStackedWidget>
#include <QTextEdit>
#include <QScrollArea>
#include <QSplitter>
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

    // Button row
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(5);

    addButton_ = new QPushButton("Add...");
    addButton_->setToolTip("Add attachment from file system");
    buttonLayout->addWidget(addButton_);

    removeButton_ = new QPushButton("Remove");
    removeButton_->setToolTip("Remove selected attachment(s)");
    removeButton_->setEnabled(false);
    buttonLayout->addWidget(removeButton_);

    openButton_ = new QPushButton("Open");
    openButton_->setToolTip("Open selected attachment with default application");
    openButton_->setEnabled(false);
    buttonLayout->addWidget(openButton_);

    revealButton_ = new QPushButton("Show in Folder");
    revealButton_->setToolTip("Reveal attachment location in file explorer");
    revealButton_->setEnabled(false);
    buttonLayout->addWidget(revealButton_);

    buttonLayout->addStretch();

    statusLabel_ = new QLabel("No attachments");
    statusLabel_->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    buttonLayout->addWidget(statusLabel_);

    mainLayout->addLayout(buttonLayout);

    // Preview pane (collapsible)
    showPreviewCheckbox_ = new QCheckBox("Show Preview");
    showPreviewCheckbox_->setChecked(false);
    mainLayout->addWidget(showPreviewCheckbox_);

    previewPane_ = new QWidget();
    QVBoxLayout* previewLayout = new QVBoxLayout(previewPane_);
    previewLayout->setContentsMargins(5, 5, 5, 5);
    previewLayout->setSpacing(5);

    QLabel* previewTitle = new QLabel("<b>Preview:</b>");
    previewLayout->addWidget(previewTitle);

    previewStack_ = new QStackedWidget();

    // Image preview page
    QWidget* imagePage = new QWidget();
    QVBoxLayout* imageLayout = new QVBoxLayout(imagePage);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    previewLabel_ = new QLabel("No preview available");
    previewLabel_->setAlignment(Qt::AlignCenter);
    previewLabel_->setMinimumHeight(100);
    previewLabel_->setMaximumHeight(150);  // Limit preview height
    previewLabel_->setScaledContents(false);
    previewLabel_->setStyleSheet("QLabel { border: 1px solid #ccc; background-color: #f5f5f5; }");
    imageLayout->addWidget(previewLabel_);
    previewStack_->addWidget(imagePage);

    // Text preview page
    QWidget* textPage = new QWidget();
    QVBoxLayout* textLayout = new QVBoxLayout(textPage);
    textLayout->setContentsMargins(0, 0, 0, 0);
    previewTextEdit_ = new QTextEdit();
    previewTextEdit_->setReadOnly(true);
    previewTextEdit_->setMaximumHeight(150);  // Limit preview height
    previewTextEdit_->setStyleSheet("QTextEdit { border: 1px solid #ccc; background-color: #f5f5f5; }");
    textLayout->addWidget(previewTextEdit_);
    previewStack_->addWidget(textPage);

    previewLayout->addWidget(previewStack_);
    previewPane_->setVisible(false);  // Hidden by default

    mainLayout->addWidget(previewPane_);

    // Connect signals
    connect(addButton_, &QPushButton::clicked, this, &AttachmentListWidget::onAddClicked);
    connect(removeButton_, &QPushButton::clicked, this, &AttachmentListWidget::onRemoveClicked);
    connect(openButton_, &QPushButton::clicked, this, &AttachmentListWidget::onOpenClicked);
    connect(revealButton_, &QPushButton::clicked, this, &AttachmentListWidget::onRevealClicked);
    connect(listWidget_, &QListWidget::itemDoubleClicked, this, &AttachmentListWidget::onItemDoubleClicked);
    connect(listWidget_, &QListWidget::itemSelectionChanged, this, &AttachmentListWidget::onSelectionChanged);
    connect(showPreviewCheckbox_, &QCheckBox::toggled, this, &AttachmentListWidget::onShowPreviewToggled);
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
    clearPreview();

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
    clearPreview();
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

    // Update preview if enabled
    if (showPreviewCheckbox_->isChecked())
    {
        QList<QListWidgetItem*> selectedItems = listWidget_->selectedItems();

        if (!selectedItems.isEmpty())
        {
            int index = listWidget_->row(selectedItems[0]);
            QList<Attachment> attachments = AttachmentManager::instance().getAttachments(numericEventId_);

            if (index >= 0 && index < attachments.size())
            {
                showPreview(attachments[index]);
            }
        }
        else
        {
            clearPreview();
        }
    }
}


void AttachmentListWidget::onShowPreviewToggled(bool checked)
{
    previewPane_->setVisible(checked);

    if (checked)
    {
        onSelectionChanged();  // Show preview for current selection
    }
    else
    {
        clearPreview();
    }
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


void AttachmentListWidget::showPreview(const Attachment& attachment)
{
    if (attachment.isImage() && AttachmentPreviewGenerator::canGenerateThumbnail(attachment))
    {
        // Show image preview
        QPixmap thumbnail = AttachmentPreviewGenerator::generateThumbnail(
            attachment,
            QSize(PREVIEW_HEIGHT, PREVIEW_HEIGHT)
            );

        if (!thumbnail.isNull())
        {
            previewLabel_->setPixmap(thumbnail);
            previewStack_->setCurrentIndex(0);
            return;
        }
    }
    else if (attachment.isTextFile() && AttachmentPreviewGenerator::canGenerateTextPreview(attachment))
    {
        // Show text preview
        QString preview = AttachmentPreviewGenerator::generateTextPreview(attachment, 1000);

        if (!preview.isEmpty())
        {
            previewTextEdit_->setPlainText(preview);
            previewStack_->setCurrentIndex(1);
            return;
        }
    }

    // No preview available
    clearPreview();
}


void AttachmentListWidget::clearPreview()
{
    previewLabel_->setText("No preview available");
    previewLabel_->setPixmap(QPixmap());
    previewTextEdit_->clear();
    previewStack_->setCurrentIndex(0);
}
