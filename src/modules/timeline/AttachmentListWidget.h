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
#include <QCheckBox>
#include <QTextEdit>
#include <QStackedWidget>
#include <QScrollArea>


/**
 * @class AttachmentListWidget
 * @brief Reusable widget for displaying and managing event attachments
 *
 * Features:
 * - List view with file icons, names, and sizes
 * - Add/Remove/Open/Reveal in Explorer buttons
 * - Drag-and-drop file addition
 * - Double-click to open files
 * - Preview pane for images and text files
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
    ~AttachmentListWidget() override = default;

    /**
     * @brief Set the event ID whose attachments to display
     * @param eventId Event ID (UUID format or "event_N" format)
     */
    void setEventId(const QString& eventId);

    /**
     * @brief Get the current event ID
     */
    QString eventId() const { return eventId_; }

    /**
     * @brief Refresh the attachment list from AttachmentManager
     */
    void refresh();

    /**
     * @brief Clear the widget (removes event ID and clears list)
     */
    void clear();

signals:
    /**
     * @brief Emitted when attachments are added or removed
     */
    void attachmentsChanged();

protected:
    /**
     * @brief Handle drag-enter events for file drops
     */
    void dragEnterEvent(QDragEnterEvent* event) override;

    /**
     * @brief Handle drop events to add attachments
     */
    void dropEvent(QDropEvent* event) override;

private slots:
    void onAddClicked();
    void onRemoveClicked();
    void onOpenClicked();
    void onRevealClicked();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onSelectionChanged();
    void onShowPreviewToggled(bool checked);

private:
    void setupUI();
    void updateButtons();
    void addAttachmentToList(const Attachment& attachment, int index);
    void showPreview(const Attachment& attachment);
    void clearPreview();

    QString eventId_;
    int numericEventId_ = -1;  // Cached hash of eventId_

    // UI Components
    QListWidget* listWidget_;
    QPushButton* addButton_;
    QPushButton* removeButton_;
    QPushButton* openButton_;
    QPushButton* revealButton_;
    QLabel* statusLabel_;
    QCheckBox* showPreviewCheckbox_;

    // Preview pane
    QWidget* previewPane_;
    QLabel* previewLabel_;
    QTextEdit* previewTextEdit_;
    QStackedWidget* previewStack_;

    // Constants
    static constexpr int ICON_SIZE = 32;
    static constexpr int PREVIEW_HEIGHT = 200;
};
