// src/app/ModuleManager.h


#pragma once
#include <QObject>
#include <QHash>
#include <QString>
#include <QList>
#include <QStringList>
#include <QWidget>
#include <memory>
#include <vector>


class IModule;


/**
 * @class ModuleManager
 * @brief Manages registration, lifecycle, switching, and ordering of application modules
 *
 * Ordering:
 * - Maintains deterministic registration order (canonical default)
 * - Allows user-defined order (drag-and-drop) persisted via QSettings
 * - Safe to call loadModuleOrder() before modules are registered
 */
class ModuleManager : public QObject
{
    Q_OBJECT

public:
    explicit ModuleManager(QObject* parent = nullptr);
    ~ModuleManager();

    void registerModule(std::unique_ptr<IModule> module);

    // Module access
    QStringList moduleIds() const;
    IModule* module(const QString& moduleId) const;
    QWidget* getModuleWidget(const QString& moduleId, QWidget* parent = nullptr);
    QString currentModuleId() const { return currentModuleId_; }
    void activateModule(const QString& moduleId);

    // Ordering / persistence
    void setModuleOrder(const QStringList& orderedIds);
    void saveModuleOrder() const;
    void loadModuleOrder();

signals:
    void moduleActivated(const QString& moduleId);
    void moduleOrderChanged();

private:
    QStringList registrationOrderIds() const;
    void reconcileOrder();   // merges saved/user order + registered modules deterministically

private:
    QHash<QString, IModule*> modules_;
    std::vector<std::unique_ptr<IModule>> moduleOwnership_;
    QHash<QString, QWidget*> moduleWidgets_;
    QString currentModuleId_;

    // Order state
    QStringList orderedModuleIds_;     ///< Current effective order
    QStringList loadedOrderIds_;       ///< Raw loaded order from settings (may include unknown modules)
    bool orderLoaded_;
};
