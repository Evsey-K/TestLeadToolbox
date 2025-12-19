// AttachmentModel.cpp


#include "AttachmentModel.h"
#include <QFileInfo>
#include <QFile>
#include <QFileIconProvider>
#include <QImageReader>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>


// ============================================================================
// Attachment Struct Implementation
// ============================================================================

bool Attachment::isValid() const {
    return !filePath.isEmpty() && fileSize >= 0;
}


QString Attachment::sizeString() const {
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (fileSize >= GB) {
        return QString("%1 GB").arg(fileSize / double(GB), 0, 'f', 2);
    } else if (fileSize >= MB) {
        return QString("%1 MB").arg(fileSize / double(MB), 0, 'f', 2);
    } else if (fileSize >= KB) {
        return QString("%1 KB").arg(fileSize / double(KB), 0, 'f', 1);
    } else {
        return QString("%1 bytes").arg(fileSize);
    }
}


QIcon Attachment::icon() const {
    QFileIconProvider iconProvider;
    QFileInfo fileInfo(filePath);
    return iconProvider.icon(fileInfo);
}


bool Attachment::exists() const {
    return QFileInfo::exists(filePath);
}


bool Attachment::supportsPreview() const {
    return isImage() || isTextFile();
}


bool Attachment::isImage() const {
    static QStringList imageExtensions = {"png", "jpg", "jpeg", "gif", "bmp", "webp", "svg"};
    return imageExtensions.contains(fileType.toLower());
}


bool Attachment::isTextFile() const {
    static QStringList textExtensions = {"txt", "log", "csv", "json", "xml", "md", "ini", "cfg"};
    return textExtensions.contains(fileType.toLower());
}


bool Attachment::isDocument() const {
    static QStringList docExtensions = {"pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx"};
    return docExtensions.contains(fileType.toLower());
}


QJsonObject Attachment::toJson() const {
    QJsonObject json;
    json["filePath"] = filePath;
    json["originalPath"] = originalPath;
    json["displayName"] = displayName;
    json["fileType"] = fileType;
    json["fileSize"] = fileSize;
    json["attachedDate"] = attachedDate.toString(Qt::ISODate);
    json["notes"] = notes;
    json["storageMode"] = (storageMode == AttachmentStorageMode::InlineEmbedded) ? "inline" : "external";
    return json;
}


Attachment Attachment::fromJson(const QJsonObject& json) {
    Attachment attachment;
    attachment.filePath = json["filePath"].toString();
    attachment.originalPath = json["originalPath"].toString();
    attachment.displayName = json["displayName"].toString();
    attachment.fileType = json["fileType"].toString();
    attachment.fileSize = json["fileSize"].toInteger();
    attachment.attachedDate = QDateTime::fromString(json["attachedDate"].toString(), Qt::ISODate);
    attachment.notes = json["notes"].toString();

    QString storageModeStr = json["storageMode"].toString();
    attachment.storageMode = (storageModeStr == "inline")
                                 ? AttachmentStorageMode::InlineEmbedded
                                 : AttachmentStorageMode::ExternalLink;

    return attachment;
}


// ============================================================================
// AttachmentPreviewGenerator Implementation
// ============================================================================

QPixmap AttachmentPreviewGenerator::generateThumbnail(const Attachment& attachment, const QSize& size) {
    if (!canGenerateThumbnail(attachment)) {
        return QPixmap();
    }

    return createImageThumbnail(attachment.filePath, size);
}


QString AttachmentPreviewGenerator::generateTextPreview(const Attachment& attachment, int maxChars) {
    if (!canGenerateTextPreview(attachment)) {
        return QString();
    }

    return readTextPreview(attachment.filePath, maxChars);
}


bool AttachmentPreviewGenerator::canGenerateThumbnail(const Attachment& attachment) {
    return attachment.isImage() && attachment.exists();
}


bool AttachmentPreviewGenerator::canGenerateTextPreview(const Attachment& attachment) {
    return attachment.isTextFile() && attachment.exists();
}


QPixmap AttachmentPreviewGenerator::createImageThumbnail(const QString& filePath, const QSize& size) {
    QImageReader reader(filePath);

    // Check if image can be read
    if (!reader.canRead()) {
        qWarning() << "Cannot read image:" << filePath;
        return QPixmap();
    }

    // Scale image to fit within size while maintaining aspect ratio
    QSize imageSize = reader.size();
    imageSize.scale(size, Qt::KeepAspectRatio);
    reader.setScaledSize(imageSize);

    QImage image = reader.read();
    if (image.isNull()) {
        qWarning() << "Failed to load image:" << filePath;
        return QPixmap();
    }

    return QPixmap::fromImage(image);
}


QString AttachmentPreviewGenerator::readTextPreview(const QString& filePath, int maxChars) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for preview:" << filePath;
        return QString();
    }

    QTextStream in(&file);
    QString preview = in.read(maxChars);

    if (!in.atEnd()) {
        preview += "\n... (truncated)";
    }

    file.close();
    return preview;
}


// ============================================================================
// AttachmentManager Implementation
// ============================================================================

AttachmentManager::AttachmentManager()
    : QObject(nullptr)
{
}


AttachmentManager& AttachmentManager::instance() {
    static AttachmentManager instance;
    return instance;
}


void AttachmentManager::setProjectDirectory(const QString& projectDir) {
    projectDirectory_ = projectDir;
    qDebug() << "AttachmentManager: Project directory set to:" << projectDir;
}


void AttachmentManager::setDefaultStorageMode(AttachmentStorageMode mode) {
    defaultStorageMode_ = mode;
}


bool AttachmentManager::addAttachment(int eventId, const QString& filePath,
                                      AttachmentStorageMode mode, QString& errorMsg) {
    // Validate file exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        errorMsg = QString("File does not exist: %1").arg(filePath);
        return false;
    }

    // Check file type is supported
    if (!isFileTypeSupported(filePath)) {
        errorMsg = QString("File type not supported: %1").arg(fileInfo.suffix());
        return false;
    }

    // Create attachment object
    Attachment attachment;
    attachment.displayName = fileInfo.fileName();
    attachment.fileType = fileInfo.suffix().toLower();
    attachment.fileSize = fileInfo.size();
    attachment.attachedDate = QDateTime::currentDateTime();
    attachment.storageMode = mode;

    // Handle storage mode
    if (mode == AttachmentStorageMode::InlineEmbedded) {
        QString destinationPath;
        if (!copyFileToProject(filePath, eventId, destinationPath, errorMsg)) {
            return false;
        }
        attachment.filePath = destinationPath;
        attachment.originalPath = filePath;
    } else {
        // External link mode - just store the path
        attachment.filePath = filePath;
        attachment.originalPath = "";
    }

    // Add to storage
    attachments_[eventId].append(attachment);

    emit attachmentAdded(eventId, attachment);
    emit attachmentsChanged(eventId);

    qDebug() << "AttachmentManager: Added attachment to event" << eventId
             << ":" << attachment.displayName;

    return true;
}


bool AttachmentManager::removeAttachment(int eventId, int attachmentIndex) {
    if (!attachments_.contains(eventId)) {
        return false;
    }

    QList<Attachment>& eventAttachments = attachments_[eventId];
    if (attachmentIndex < 0 || attachmentIndex >= eventAttachments.size()) {
        return false;
    }

    Attachment removedAttachment = eventAttachments[attachmentIndex];

    // If inline attachment, delete the file
    if (removedAttachment.storageMode == AttachmentStorageMode::InlineEmbedded) {
        QFile::remove(removedAttachment.filePath);
        qDebug() << "AttachmentManager: Deleted inline file:" << removedAttachment.filePath;
    }

    eventAttachments.removeAt(attachmentIndex);

    emit attachmentRemoved(eventId, attachmentIndex);
    emit attachmentsChanged(eventId);

    qDebug() << "AttachmentManager: Removed attachment from event" << eventId;

    return true;
}


QList<Attachment> AttachmentManager::getAttachments(int eventId) const {
    return attachments_.value(eventId, QList<Attachment>());
}


int AttachmentManager::getAttachmentCount(int eventId) const {
    return attachments_.value(eventId, QList<Attachment>()).size();
}


bool AttachmentManager::addMultipleAttachments(int eventId, const QStringList& filePaths,
                                               AttachmentStorageMode mode, QStringList& errors) {
    bool anySuccess = false;
    errors.clear();

    for (const QString& filePath : filePaths) {
        QString errorMsg;
        if (addAttachment(eventId, filePath, mode, errorMsg)) {
            anySuccess = true;
        } else {
            errors.append(QString("%1: %2").arg(filePath, errorMsg));
        }
    }

    return anySuccess;
}


void AttachmentManager::clearAttachments(int eventId) {
    if (!attachments_.contains(eventId)) {
        return;
    }

    // Delete inline files
    for (const Attachment& attachment : attachments_[eventId]) {
        if (attachment.storageMode == AttachmentStorageMode::InlineEmbedded) {
            QFile::remove(attachment.filePath);
        }
    }

    attachments_.remove(eventId);
    emit attachmentsChanged(eventId);

    qDebug() << "AttachmentManager: Cleared all attachments for event" << eventId;
}


bool AttachmentManager::openAttachment(const Attachment& attachment) {
    if (!attachment.exists()) {
        qWarning() << "AttachmentManager: Cannot open non-existent file:" << attachment.filePath;
        return false;
    }

    QUrl url = QUrl::fromLocalFile(attachment.filePath);
    return QDesktopServices::openUrl(url);
}


bool AttachmentManager::revealInExplorer(const Attachment& attachment) {
    if (!attachment.exists()) {
        qWarning() << "AttachmentManager: Cannot reveal non-existent file:" << attachment.filePath;
        return false;
    }

#ifdef Q_OS_WIN
    // Windows: Use explorer.exe /select
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(attachment.filePath);
    return QProcess::startDetached("explorer.exe", args);
#elif defined(Q_OS_MAC)
    // macOS: Use "open -R"
    QStringList args;
    args << "-R" << attachment.filePath;
    return QProcess::startDetached("open", args);
#else
    // Linux: Open containing directory
    QFileInfo fileInfo(attachment.filePath);
    QUrl url = QUrl::fromLocalFile(fileInfo.absolutePath());
    return QDesktopServices::openUrl(url);
#endif
}


QString AttachmentManager::getAttachmentDirectory(int eventId) const {
    if (projectDirectory_.isEmpty()) {
        return QString();
    }

    return QDir(projectDirectory_).filePath(QString("attachments/%1").arg(eventId));
}


bool AttachmentManager::ensureAttachmentDirectory(int eventId) {
    QString attachmentDir = getAttachmentDirectory(eventId);
    if (attachmentDir.isEmpty()) {
        qWarning() << "AttachmentManager: Project directory not set";
        return false;
    }

    QDir dir;
    if (!dir.exists(attachmentDir)) {
        if (!dir.mkpath(attachmentDir)) {
            qWarning() << "AttachmentManager: Failed to create directory:" << attachmentDir;
            return false;
        }
        qDebug() << "AttachmentManager: Created attachment directory:" << attachmentDir;
    }

    return true;
}


qint64 AttachmentManager::getTotalAttachmentsSize(int eventId) const {
    qint64 total = 0;
    for (const Attachment& attachment : getAttachments(eventId)) {
        total += attachment.fileSize;
    }
    return total;
}


qint64 AttachmentManager::getTotalProjectAttachmentsSize() const {
    qint64 total = 0;
    for (const QList<Attachment>& attachmentList : attachments_) {
        for (const Attachment& attachment : attachmentList) {
            total += attachment.fileSize;
        }
    }
    return total;
}


bool AttachmentManager::removeEventAttachmentDirectory(int eventId) {
    QString attachmentDir = getAttachmentDirectory(eventId);
    if (attachmentDir.isEmpty() || !QDir(attachmentDir).exists()) {
        return true; // Nothing to remove
    }

    QDir dir(attachmentDir);
    return dir.removeRecursively();
}


bool AttachmentManager::isFileTypeSupported(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    return supportedExtensions().contains(extension);
}


QStringList AttachmentManager::supportedExtensions() {
    static QStringList extensions = {
        // Documents
        "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx",
        "odt", "ods", "odp",

        // Text files
        "txt", "log", "csv", "json", "xml", "md", "rst",
        "ini", "cfg", "conf", "yaml", "yml",

        // Images
        "png", "jpg", "jpeg", "gif", "bmp", "webp", "svg", "ico",

        // Archives
        "zip", "tar", "gz", "7z", "rar",

        // Code
        "cpp", "h", "py", "js", "html", "css", "java", "c", "cs",

        // Other
        "url", "lnk"
    };
    return extensions;
}


QString AttachmentManager::getFileType(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    return fileInfo.suffix().toLower();
}


QJsonArray AttachmentManager::serializeAttachments(int eventId) const {
    QJsonArray jsonArray;

    for (const Attachment& attachment : getAttachments(eventId)) {
        jsonArray.append(attachment.toJson());
    }

    return jsonArray;
}


bool AttachmentManager::deserializeAttachments(int eventId, const QJsonArray& jsonArray) {
    QList<Attachment> loadedAttachments;

    for (const QJsonValue& value : jsonArray) {
        if (!value.isObject()) {
            continue;
        }

        Attachment attachment = Attachment::fromJson(value.toObject());
        if (attachment.isValid()) {
            loadedAttachments.append(attachment);
        }
    }

    if (!loadedAttachments.isEmpty()) {
        attachments_[eventId] = loadedAttachments;
        emit attachmentsChanged(eventId);
        qDebug() << "AttachmentManager: Loaded" << loadedAttachments.size()
                 << "attachments for event" << eventId;
    }

    return true;
}


bool AttachmentManager::copyFileToProject(const QString& sourcePath, int eventId,
                                          QString& destinationPath, QString& errorMsg) {
    if (projectDirectory_.isEmpty()) {
        errorMsg = "Project directory not set";
        return false;
    }

    // Ensure attachment directory exists
    if (!ensureAttachmentDirectory(eventId)) {
        errorMsg = "Failed to create attachment directory";
        return false;
    }

    // Generate unique destination filename
    QFileInfo sourceInfo(sourcePath);
    QString uniqueFileName = generateUniqueFileName(eventId, sourceInfo.fileName());

    // Build full destination path
    QString attachmentDir = getAttachmentDirectory(eventId);
    destinationPath = QDir(attachmentDir).filePath(uniqueFileName);

    // Copy file
    if (!QFile::copy(sourcePath, destinationPath)) {
        errorMsg = QString("Failed to copy file to: %1").arg(destinationPath);
        return false;
    }

    qDebug() << "AttachmentManager: Copied file from" << sourcePath << "to" << destinationPath;
    return true;
}


QString AttachmentManager::generateUniqueFileName(int eventId, const QString& originalName) const {
    QString attachmentDir = getAttachmentDirectory(eventId);
    QDir dir(attachmentDir);

    // If file doesn't exist, use original name
    if (!dir.exists(originalName)) {
        return originalName;
    }

    // Generate unique name with suffix
    QFileInfo fileInfo(originalName);
    QString baseName = fileInfo.completeBaseName();
    QString extension = fileInfo.suffix();

    int counter = 1;
    QString uniqueName;
    do {
        uniqueName = QString("%1_%2.%3").arg(baseName).arg(counter).arg(extension);
        counter++;
    } while (dir.exists(uniqueName));

    return uniqueName;
}
