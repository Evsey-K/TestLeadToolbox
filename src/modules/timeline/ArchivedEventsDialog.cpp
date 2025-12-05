// ArchivedEventsDialog.cpp
// Phase 5 Tier 3: Archived Events Management (MANUAL UI VERSION)


#include "ArchivedEventsDialog.h"
#include "TimelineModel.h"
#include <QUndoStack>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>


ArchivedEventsDialog::ArchivedEventsDialog(TimelineModel* model,
                                           QUndoStack* undoStack,
                                           QWidget* parent)
    : QDialog(parent)
    , model_(model)
    , undoStack_(undoStack)
    , infoLabel_(nullptr)
    , closeButton_(nullptr)
{
    setWindowTitle("Archived Events");
    setMinimumSize(500, 300);

    setupUi();
}


void ArchivedEventsDialog::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Title
    auto titleLabel = new QLabel("Archive Feature - Coming Soon");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    mainLayout->addSpacing(10);

    // Info message
    infoLabel_ = new QLabel(
        "The archive feature requires TimelineModel extension to support soft-delete.\n\n"
        "For now, use the Undo/Redo system to restore deleted events:\n\n"
        "  • Press Ctrl+Z to undo the last deletion\n"
        "  • Use Edit → Undo to see all available undo operations\n"
        "  • The undo stack preserves up to 50 operations\n\n"
        "This feature will be implemented in a future phase."
        );
    infoLabel_->setWordWrap(true);
    infoLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch();

    // Close button
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    closeButton_ = new QPushButton("Close");
    closeButton_->setDefault(true);
    closeButton_->setMinimumWidth(100);
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton_);

    mainLayout->addLayout(buttonLayout);

    // Set focus to close button
    closeButton_->setFocus();
}
