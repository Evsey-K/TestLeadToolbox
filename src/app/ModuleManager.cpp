// src/app/ModuleManager.cpp

#include "ModuleManager.h"
#include "shared/interfaces/IModule.h"
#include <QDebug>

ModuleManager::ModuleManager(QObject* parent)
    : QObject(parent)
{
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

    // Store ownership in list using push_back which properly moves
    moduleOwnership_.push_back(std::move(module));
}

QStringList ModuleManager::moduleIds() const
{
    return modules_.keys();
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
