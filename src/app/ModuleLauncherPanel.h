// src/app/ModuleLauncherPanel.h


#pragma once
#include <QWidget>


class ModuleManager;
class QListWidget;
class QListWidgetItem;
class QLabel;


/**
 * @class ModuleLauncherPanel
 * @brief Professional module navigation panel using Qt item-view architecture
 *
 * - Native drag & drop reordering (InternalMove)
 * - Persistent module order via ModuleManager
 * - Clean activation & selection handling
 */
class ModuleLauncherPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ModuleLauncherPanel(ModuleManager* moduleManager, QWidget* parent = nullptr);
    ~ModuleLauncherPanel();

    void refresh();

signals:
    void moduleLaunchRequested(const QString& moduleId);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onCurrentModuleChanged(const QString& moduleId);
    void onOrderChanged();

private:
    void setupUI();
    void populateList();
    QStringList currentListOrder() const;

private:
    ModuleManager* moduleManager_;
    QListWidget* moduleList_;
    QLabel* titleLabel_;
    QString currentModuleId_;
};
