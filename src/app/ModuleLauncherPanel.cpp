// src/app/ModuleLauncherPanel.cpp

#include "ModuleLauncherPanel.h"
#include "ModuleManager.h"
#include "shared/interfaces/IModule.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QFont>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSignalBlocker>

ModuleLauncherPanel::ModuleLauncherPanel(ModuleManager* moduleManager, QWidget* parent)
    : QWidget(parent)
    , moduleManager_(moduleManager)
    , moduleList_(nullptr)
{
    setupUI();

    // Safe even if called before modules registered
    moduleManager_->loadModuleOrder();

    populateList();

    connect(moduleList_, &QListWidget::itemClicked,
            this, &ModuleLauncherPanel::onItemClicked);

    connect(moduleManager_, &ModuleManager::moduleActivated,
            this, &ModuleLauncherPanel::onCurrentModuleChanged);
}

ModuleLauncherPanel::~ModuleLauncherPanel() = default;

void ModuleLauncherPanel::setupUI()
{
    setMinimumWidth(250);
    setMaximumWidth(300);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(6);

    // Title
    titleLabel_ = new QLabel("TestLeadToolbox", this);
    QFont titleFont = titleLabel_->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel_->setFont(titleFont);
    titleLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel_);

    // Subtitle
    auto* subtitle = new QLabel("Select Application Module", this);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("color: gray;");
    mainLayout->addWidget(subtitle);

    // Separator
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(sep);

    mainLayout->addSpacing(8);

    // Module list (industry-standard)
    moduleList_ = new QListWidget(this);
    moduleList_->setSelectionMode(QAbstractItemView::SingleSelection);
    moduleList_->setDragDropMode(QAbstractItemView::InternalMove);
    moduleList_->setDefaultDropAction(Qt::MoveAction);
    moduleList_->setDragEnabled(true);
    moduleList_->setAcceptDrops(true);
    moduleList_->setDropIndicatorShown(true);
    moduleList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    moduleList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    moduleList_->setIconSize(QSize(32, 32));
    moduleList_->setSortingEnabled(false);

    // Styling to match your existing buttons (professional)
    moduleList_->setStyleSheet(
        "QListWidget { border: none; background: transparent; }"
        "QListWidget::item {"
        "  padding: 10px 14px;"
        "  margin-bottom: 6px;"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "  background: #f8f8f8;"
        "}"
        "QListWidget::item:hover {"
        "  background: #e8e8e8;"
        "  border-color: #999;"
        "}"
        "QListWidget::item:selected {"
        "  background: #d0e8ff;"
        "  border-color: #0078d7;"
        "  font-weight: bold;"
        "}"
        );

    mainLayout->addWidget(moduleList_);
    mainLayout->addStretch();

    // Persist order AFTER Qt finishes moving rows
    connect(moduleList_->model(), &QAbstractItemModel::rowsMoved,
            this, &ModuleLauncherPanel::onOrderChanged);
}

void ModuleLauncherPanel::populateList()
{
    QSignalBlocker blocker(moduleList_);
    moduleList_->clear();

    for (const QString& moduleId : moduleManager_->moduleIds()) {
        IModule* mod = moduleManager_->module(moduleId);
        if (!mod) continue;

        auto* item = new QListWidgetItem(mod->icon(), mod->name());
        item->setToolTip(mod->description());
        item->setData(Qt::UserRole, moduleId);
        item->setSizeHint(QSize(0, 60));

        moduleList_->addItem(item);
    }

    onCurrentModuleChanged(currentModuleId_);
}

void ModuleLauncherPanel::onItemClicked(QListWidgetItem* item)
{
    if (!item) return;

    const QString moduleId = item->data(Qt::UserRole).toString();
    if (!moduleId.isEmpty()) {
        emit moduleLaunchRequested(moduleId);
    }
}

void ModuleLauncherPanel::onCurrentModuleChanged(const QString& moduleId)
{
    currentModuleId_ = moduleId;

    for (int i = 0; i < moduleList_->count(); ++i) {
        QListWidgetItem* item = moduleList_->item(i);
        if (item && item->data(Qt::UserRole).toString() == moduleId) {
            moduleList_->setCurrentRow(i);
            break;
        }
    }
}

QStringList ModuleLauncherPanel::currentListOrder() const
{
    QStringList order;
    for (int i = 0; i < moduleList_->count(); ++i) {
        order.append(moduleList_->item(i)->data(Qt::UserRole).toString());
    }
    return order;
}

void ModuleLauncherPanel::onOrderChanged()
{
    const QStringList newOrder = currentListOrder();
    moduleManager_->setModuleOrder(newOrder);
    moduleManager_->saveModuleOrder();
}

void ModuleLauncherPanel::refresh()
{
    populateList();
}
