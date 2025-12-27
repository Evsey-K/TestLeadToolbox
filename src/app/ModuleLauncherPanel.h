// src/app/ModuleLauncherPanel.h
#pragma once

#include <QWidget>

class ModuleManager;
class QVBoxLayout;
class QPushButton;
class QLabel;

/**
 * @class ModuleLauncherPanel
 * @brief Professional navigation panel for selecting application modules
 *
 * Displays a vertical list of module launcher buttons with icons, names,
 * and descriptions. Handles module selection and communicates with the
 * ModuleManager to coordinate module activation.
 */
class ModuleLauncherPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ModuleLauncherPanel(ModuleManager* moduleManager, QWidget* parent = nullptr);
    ~ModuleLauncherPanel();

    void refresh();     ///< Refreshes the panel to reflect current module registrations

signals:
    void moduleLaunchRequested(const QString& moduleId);        ///< Emitted when user requests to launch a module

private slots:
    void onModuleButtonClicked();
    void onCurrentModuleChanged(const QString& moduleId);

private:
    void setupUI();
    void createModuleButtons();
    QPushButton* createModuleButton(const QString& moduleId);

private:
    ModuleManager* moduleManager_;
    QVBoxLayout* buttonLayout_;
    QLabel* titleLabel_;
    QMap<QString, QPushButton*> moduleButtons_;
    QString currentModuleId_;
};
