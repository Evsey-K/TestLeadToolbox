// src/app/ModuleManager.h


#pragma once
#include <QObject>
#include <QHash>
#include <QString>
#include <QList>
#include <memory>


class IModule;
class QWidget;


/**
 * @class ModuleManager
 * @brief Manages registration, lifecycle, and switching between application modules
 */
class ModuleManager : public QObject
{
    Q_OBJECT

public:
    explicit ModuleManager(QObject* parent = nullptr);
    ~ModuleManager();

    void registerModule(std::unique_ptr<IModule> module);
    QStringList moduleIds() const;
    IModule* module(const QString& moduleId) const;
    QWidget* getModuleWidget(const QString& moduleId, QWidget* parent = nullptr);
    QString currentModuleId() const { return currentModuleId_; }
    void activateModule(const QString& moduleId);

signals:
    void moduleActivated(const QString& moduleId);

private:
    QHash<QString, IModule*> modules_;
    std::vector<std::unique_ptr<IModule>> moduleOwnership_;
    QHash<QString, QWidget*> moduleWidgets_;
    QString currentModuleId_;
};
