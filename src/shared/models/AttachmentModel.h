// AttachmentModel.h

#pragma once

#include <QString>
#include <QDateTime>
#include <QIcon>
#include <QPixmap>
#include <QList>
#include <QMap>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>

/**
 * @brief Attachment storage mode
 */
enum class AttachmentStorageMode {
    ExternalLink,    ///< Store only file path (link to external file)
    InlineEmbedded   ///< Copy file into project directory
};

/**
 * @brief Represents a single file attachment
 */
struct Attachment {
    QString filePath;           ///< Current path to the file
    QString originalPath;       ///< Original source path (for inline mode)
    QString displayName;        ///< User-friendly name
    QString fileType;           ///< Extension or MIME type
    qint64 fileSize;            ///< Size in bytes
    QDateTime attachedDate;     ///< When it was attached
    QString notes;              ///< Optional user notes
    AttachmentStorageMode storageMode;

    Attachment()
        : fileSize(0)
        , storageMode(AttachmentStorageMode::InlineEmbedded)
    {}

    // Helper methods
    bool isValid() const;
    QString sizeString() const;
    QIcon icon() const;
    bool exists() const;

    // Preview support
    bool supportsPreview() const;
    bool isImage() const;
    bool isTextFile() const;
    bool isDocument() const;

    // Serialization
    QJsonObject toJson() const;
    static Attachment fromJson(const QJsonObject& json);
};

/**
 * @brief Manages preview generation for attachments
 */
class AttachmentPreviewGenerator {
public:
    /**
     * @brief Generate thumbnail for supported file types
     * @param attachment Attachment to generate thumbnail for
     * @param size Desired thumbnail size (default: 200x200)
     * @return QPixmap containing thumbnail, or null pixmap if unsupported
     */
    static QPixmap generateThumbnail(const Attachment& attachment, const QSize& size = QSize(200, 200));

    /**
     * @brief Generate text preview for text-based files
     * @param attachment Attachment to generate preview for
     * @param maxChars Maximum characters to read (default: 500)
     * @return QString containing preview text, or empty if unsupported
     */
    static QString generateTextPreview(const Attachment& attachment, int maxChars = 500);

    /**
     * @brief Check if thumbnail generation is supported
     */
    static bool canGenerateThumbnail(const Attachment& attachment);

    /**
     * @brief Check if text preview generation is supported
     */
    static bool canGenerateTextPreview(const Attachment& attachment);

private:
    static QPixmap createImageThumbnail(const QString& filePath, const QSize& size);
    static QString readTextPreview(const QString& filePath, int maxChars);
};

/**
 * @brief Manages attachments for timeline events
 *
 * This singleton class provides centralized management of file attachments
 * for timeline events, including:
 * - Adding/removing attachments
 * - Inline storage (copying files into project directory)
 * - External link storage (reference only)
 * - File operations (open, reveal in explorer)
 * - Serialization support
 */
class AttachmentManager : public QObject {
    Q_OBJECT

public:
    static AttachmentManager& instance();

    // Configuration
    void setProjectDirectory(const QString& projectDir);
    QString projectDirectory() const { return projectDirectory_; }

    void setDefaultStorageMode(AttachmentStorageMode mode);
    AttachmentStorageMode defaultStorageMode() const { return defaultStorageMode_; }

    // Core operations
    bool addAttachment(int eventId, const QString& filePath,
                       AttachmentStorageMode mode, QString& errorMsg);
    bool removeAttachment(int eventId, int attachmentIndex);
    QList<Attachment> getAttachments(int eventId) const;
    int getAttachmentCount(int eventId) const;

    // Bulk operations
    bool addMultipleAttachments(int eventId, const QStringList& filePaths,
                                AttachmentStorageMode mode, QStringList& errors);
    void clearAttachments(int eventId);

    // File operations
    bool openAttachment(const Attachment& attachment);
    bool revealInExplorer(const Attachment& attachment);

    // Storage management
    QString getAttachmentDirectory(int eventId) const;
    bool ensureAttachmentDirectory(int eventId);
    qint64 getTotalAttachmentsSize(int eventId) const;
    qint64 getTotalProjectAttachmentsSize() const;

    // Cleanup operations
    bool removeEventAttachmentDirectory(int eventId);

    // Validation
    static bool isFileTypeSupported(const QString& filePath);
    static QStringList supportedExtensions();
    static QString getFileType(const QString& filePath);

    // Serialization support
    QJsonArray serializeAttachments(int eventId) const;
    bool deserializeAttachments(int eventId, const QJsonArray& jsonArray);

signals:
    void attachmentsChanged(int eventId);
    void attachmentAdded(int eventId, const Attachment& attachment);
    void attachmentRemoved(int eventId, int index);

private:
    AttachmentManager();
    ~AttachmentManager() = default;

    // Prevent copying
    AttachmentManager(const AttachmentManager&) = delete;
    AttachmentManager& operator=(const AttachmentManager&) = delete;

    QMap<int, QList<Attachment>> attachments_;  ///< eventId -> attachments
    QString projectDirectory_;
    AttachmentStorageMode defaultStorageMode_ = AttachmentStorageMode::InlineEmbedded;

    bool copyFileToProject(const QString& sourcePath, int eventId,
                           QString& destinationPath, QString& errorMsg);
    QString generateUniqueFileName(int eventId, const QString& originalName) const;
};
