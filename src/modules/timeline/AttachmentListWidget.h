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
 *
 * Usage:
 * @code
 *   AttachmentListWidget* widget = new AttachmentListWidget(this);
 *   widget->setEventId(eventId);
 *   widget->refresh();
 * @endcode
 */
class AttachmentListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AttachmentListWidget(QWidget* parent = nullptr);
    ~AttachmentListWidget() override;

    void setEventId(const QString& eventId);                        ///< @brief Set the event ID whose attachments to display
    QString eventId() const { return eventId_; }                    ///< @brief Get the current event ID
    void refresh();                                                 ///< @brief Refresh the attachment list from AttachmentManager
    void clear();                                                   ///< @brief Clear the widget (removes event ID and clears list)

signals:
    void attachmentsChanged();                                      ///< @brief Emitted when attachments are added or removed

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;           ///< @brief Handle drag-enter events for file drops
    void dropEvent(QDropEvent* event) override;                     ///< @brief Handle drop events to add attachments

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
    QListWidget* listWidget_;
    QPushButton* addButton_;
    QPushButton* removeButton_;
    QPushButton* openButton_;
    QPushButton* revealButton_;
    QLabel* statusLabel_;

    // Constants
    static constexpr int ICON_SIZE = 32;
};
