// src/shared/interfaces/IModule.h


#pragma once
#include <QWidget>
#include <QString>
#include <QIcon>


/**
 * @interface IModule
 * @brief Abstract interface for all TestLeadToolbox application modules
 */
class IModule {
public:
    virtual ~IModule() = default;

    virtual QWidget* createWidget(QWidget* parent = nullptr) = 0;
    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QIcon icon() const = 0;
    virtual QString moduleId() const = 0;
    virtual void onActivate() {}
    virtual void onDeactivate() {}
    virtual bool hasUnsavedChanges() const { return false; }
    virtual bool save() { return true; }
};
