// src/app/ModuleLauncherPanel.cpp


#include "ModuleLauncherPanel.h"
#include "ModuleManager.h"
#include "shared/interfaces/IModule.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QFont>


ModuleLauncherPanel::ModuleLauncherPanel(ModuleManager* moduleManager, QWidget* parent)
    : QWidget(parent)
    , moduleManager_(moduleManager)
    , buttonLayout_(nullptr)
    , titleLabel_(nullptr)
{
    setupUI();
    createModuleButtons();
    connect(moduleManager_, &ModuleManager::moduleActivated, this, &ModuleLauncherPanel::onCurrentModuleChanged);
}


ModuleLauncherPanel::~ModuleLauncherPanel()
{
}


void ModuleLauncherPanel::setupUI()
{
    setMinimumWidth(250);
    setMaximumWidth(300);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(5);

    // Title
    titleLabel_ = new QLabel("TestLeadToolbox", this);
    QFont titleFont = titleLabel_->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel_->setFont(titleFont);
    titleLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel_);

    // Subtitle
    auto* subtitleLabel = new QLabel("Select Application Module", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPointSize(9);
    subtitleFont.setItalic(true);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color: gray;");
    mainLayout->addWidget(subtitleLabel);

    // Separator
    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator);

    mainLayout->addSpacing(10);

    // Button container
    buttonLayout_ = new QVBoxLayout();
    buttonLayout_->setSpacing(8);
    mainLayout->addLayout(buttonLayout_);

    mainLayout->addStretch();
}


void ModuleLauncherPanel::createModuleButtons()
{
    moduleButtons_.clear();
    QLayoutItem* item;
    while ((item = buttonLayout_->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    QStringList moduleIds = moduleManager_->moduleIds();
    for (const QString& moduleId : moduleIds) {
        QPushButton* button = createModuleButton(moduleId);
        if (button) {
            moduleButtons_[moduleId] = button;
            buttonLayout_->addWidget(button);
        }
    }
}


QPushButton* ModuleLauncherPanel::createModuleButton(const QString& moduleId)
{
    IModule* mod = moduleManager_->module(moduleId);
    if (!mod) {
        return nullptr;
    }

    auto* button = new QPushButton(this);
    button->setText(mod->name());
    button->setIcon(mod->icon());
    button->setIconSize(QSize(32, 32));
    button->setToolTip(mod->description());
    button->setCheckable(true);
    button->setMinimumHeight(60);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Style
    button->setStyleSheet(
        "QPushButton {"
        "    text-align: left;"
        "    padding: 10px 15px;"
        "    border: 1px solid #ccc;"
        "    border-radius: 4px;"
        "    background-color: #f8f8f8;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e8e8e8;"
        "    border-color: #999;"
        "}"
        "QPushButton:checked {"
        "    background-color: #d0e8ff;"
        "    border-color: #0078d7;"
        "    font-weight: bold;"
        "}"
        );

    // Store module ID as property
    button->setProperty("moduleId", moduleId);

    connect(button, &QPushButton::clicked, this, &ModuleLauncherPanel::onModuleButtonClicked);

    return button;
}


void ModuleLauncherPanel::onModuleButtonClicked()
{
    auto* button = qobject_cast<QPushButton*>(sender());
    if (!button) {
        return;
    }

    QString moduleId = button->property("moduleId").toString();
    if (!moduleId.isEmpty()) {
        emit moduleLaunchRequested(moduleId);
    }
}

void ModuleLauncherPanel::onCurrentModuleChanged(const QString& moduleId)
{
    currentModuleId_ = moduleId;

    // Update button checked states
    for (auto it = moduleButtons_.begin(); it != moduleButtons_.end(); ++it) {
        it.value()->setChecked(it.key() == moduleId);
    }
}


void ModuleLauncherPanel::refresh()
{
    createModuleButtons();
}
