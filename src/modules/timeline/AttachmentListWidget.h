// AttachmentListWidget.h

#pragma once
#include "../../shared/models/AttachmentModel.h"
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>

class QDragEnterEvent;
class QDropEvent;

/**
 * @class AttachmentListWidget
 * @brief Reusable widget for displaying and managing event attachments
 *
 * Features:
 * - List view with file icons, names, and sizes
 * - Add/Remove/Open/Reveal in Explorer buttons
 * - Drag-and-drop file addition
 * - Double-click to open files
 * - Real-time updates when attachments change
 */
class AttachmentListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AttachmentListWidget(QWidget* parent = nullptr);
    ~AttachmentListWidget() override;

    void setEventId(const QString& eventId);                        ///< Set the event ID whose attachments to display
    QString eventId() const { return eventId_; }                    ///< Get the current event ID
    void refresh();                                                 ///< Refresh the attachment list from AttachmentManager
    void clear();                                                   ///< Clear the widget (removes event ID and clears list)

    void setAddButtonVisible(bool visible);                         ///< Show/hide the internal Add button (for header-driven UIs)

public slots:
    void requestAddAttachments();                                   ///< External "Add" trigger (header button calls this)

signals:
    void attachmentsChanged();                                      ///< Emitted when attachments are added or removed

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onAddClicked();
    void onRemoveClicked();
    void onOpenClicked();
    void onRevealClicked();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onSelectionChanged();

private:
    void setupUI();
    void updateButtons();
    void addAttachmentToList(const Attachment& attachment, int index);

    QString eventId_;
    uint numericEventId_ = -1;  // Cached hash of eventId_

    // UI Components
    QListWidget* listWidget_ = nullptr;

    QPushButton* addButton_ = nullptr;
    QPushButton* removeButton_ = nullptr;
    QPushButton* openButton_ = nullptr;
    QPushButton* revealButton_ = nullptr;

    QLabel* statusLabel_ = nullptr;

    // Constants
    static constexpr int ICON_SIZE = 32;
};
