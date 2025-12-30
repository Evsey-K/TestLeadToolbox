// src/app/ModuleManager.cpp

#include "ModuleManager.h"
#include "shared/interfaces/IModule.h"
#include <QDebug>
#include <QSettings>


namespace {
constexpr const char* kSettingsGroup = "Modules";
constexpr const char* kSettingsKeyOrder = "order";
}


ModuleManager::ModuleManager(QObject* parent)
    : QObject(parent)
    , orderLoaded_(false)
{
    // std::vector handles move-only types (like unique_ptr) properly
    moduleOwnership_.reserve(10);
}

ModuleManager::~ModuleManager()
{
    moduleWidgets_.clear();
    modules_.clear();
    moduleOwnership_.clear();
}

void ModuleManager::registerModule(std::unique_ptr<IModule> module)
{
    if (!module) {
        qWarning() << "ModuleManager::registerModule - Attempted to register null module";
        return;
    }

    QString moduleId = module->moduleId();
    if (modules_.contains(moduleId)) {
        qWarning() << "ModuleManager::registerModule - Module already registered:" << moduleId;
        return;
    }

    qDebug() << "ModuleManager: Registering module:" << module->name() << "(" << moduleId << ")";

    // Store raw pointer in hash for fast lookup
    IModule* rawPtr = module.get();
    modules_[moduleId] = rawPtr;

    // Store ownership in vector using push_back which properly moves
    moduleOwnership_.push_back(std::move(module));

    // Keep ordering coherent as modules arrive (important if loadModuleOrder() ran early)
    reconcileOrder();
}

QStringList ModuleManager::moduleIds() const
{
    return orderedModuleIds_;
}

IModule* ModuleManager::module(const QString& moduleId) const
{
    return modules_.value(moduleId, nullptr);
}

QWidget* ModuleManager::getModuleWidget(const QString& moduleId, QWidget* parent)
{
    if (moduleWidgets_.contains(moduleId)) {
        return moduleWidgets_[moduleId];
    }

    IModule* mod = module(moduleId);
    if (!mod) {
        qWarning() << "ModuleManager::getModuleWidget - Module not found:" << moduleId;
        return nullptr;
    }

    QWidget* widget = mod->createWidget(parent);
    if (widget) {
        moduleWidgets_[moduleId] = widget;
        qDebug() << "ModuleManager: Created widget for module:" << moduleId;
    } else {
        qWarning() << "ModuleManager::getModuleWidget - Failed to create widget for:" << moduleId;
    }

    return widget;
}

void ModuleManager::activateModule(const QString& moduleId)
{
    if (currentModuleId_ == moduleId) {
        return;
    }

    if (!currentModuleId_.isEmpty()) {
        IModule* currentMod = module(currentModuleId_);
        if (currentMod) {
            currentMod->onDeactivate();
        }
    }

    IModule* newMod = module(moduleId);
    if (newMod) {
        newMod->onActivate();
        currentModuleId_ = moduleId;
        emit moduleActivated(moduleId);
        qDebug() << "ModuleManager: Activated module:" << moduleId;
    } else {
        qWarning() << "ModuleManager::activateModule - Module not found:" << moduleId;
    }
}

void ModuleManager::setModuleOrder(const QStringList& orderedIds)
{
    // Store as "loaded/user order" source, then reconcile deterministically
    loadedOrderIds_ = orderedIds;
    orderLoaded_ = true;

    const QStringList before = orderedModuleIds_;
    reconcileOrder();

    if (orderedModuleIds_ != before) {
        emit moduleOrderChanged();
        qDebug() << "ModuleManager: Module order updated:" << orderedModuleIds_;
    }
}

void ModuleManager::saveModuleOrder() const
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kSettingsKeyOrder, orderedModuleIds_);
    settings.endGroup();

    qDebug() << "ModuleManager: Saved module order:" << orderedModuleIds_;
}

void ModuleManager::loadModuleOrder()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    loadedOrderIds_.clear();
    if (settings.contains(kSettingsKeyOrder)) {
        loadedOrderIds_ = settings.value(kSettingsKeyOrder).toStringList();
        orderLoaded_ = true;
        qDebug() << "ModuleManager: Loaded raw module order:" << loadedOrderIds_;
    } else {
        orderLoaded_ = false;
    }

    settings.endGroup();

    const QStringList before = orderedModuleIds_;
    reconcileOrder();

    if (orderedModuleIds_ != before) {
        emit moduleOrderChanged();
        qDebug() << "ModuleManager: Applied module order after load:" << orderedModuleIds_;
    }
}

QStringList ModuleManager::registrationOrderIds() const
{
    QStringList ids;
    ids.reserve(static_cast<int>(moduleOwnership_.size()));

    for (const auto& owned : moduleOwnership_) {
        if (owned) {
            ids.append(owned->moduleId());
        }
    }

    return ids;
}

void ModuleManager::reconcileOrder()
{
    // Canonical deterministic base is always registration order
    const QStringList regOrder = registrationOrderIds();

    QStringList result;
    result.reserve(regOrder.size());

    // If we have a loaded/user order, apply it first (only for modules that exist)
    if (orderLoaded_) {
        for (const QString& id : loadedOrderIds_) {
            if (modules_.contains(id) && !result.contains(id)) {
                result.append(id);
            }
        }
    }

    // Append any remaining modules deterministically in registration order
    for (const QString& id : regOrder) {
        if (modules_.contains(id) && !result.contains(id)) {
            result.append(id);
        }
    }

    // If nothing is registered yet, keep it empty (safe early-load)
    orderedModuleIds_ = result;
}
