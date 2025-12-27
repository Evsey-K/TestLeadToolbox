// src/app/MainWindow.h


#pragma once
#include <QMainWindow>
#include <QList>
#include <QMetaObject>


class ModuleManager;
class ModuleLauncherPanel;
class QStackedWidget;
class QSplitter;
class QAction;
class QMenu;


/**
 * @class MainWindow
 * @brief Main application window with module navigation and switching
 *
 * MainWindow now serves as a shell that hosts multiple application modules.
 * It provides a navigation panel for module selection and a central area
 * that displays the active module's widget.
 *
 * MainWindow owns standard File and Edit menu shortcuts (Ctrl+S, Ctrl+Z, etc.)
 * and delegates their execution to the currently active module.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupModules();
    void setupUI();
    void setupMenuBar();
    void setupFileMenu();
    void setupEditMenu();
    void setupViewMenu();
    void setupHelpMenu();
    void connectSignals();

    void connectModuleActions();      ///< @brief Connect MainWindow actions to active module
    void disconnectModuleActions();   ///< @brief Disconnect previous module's actions

private slots:
    void onModuleLaunchRequested(const QString& moduleId);
    void onModuleActivated(const QString& moduleId);
    void onShowArchivedEvents();
    void onShowPreferences();
    void onShowAbout();
    void onSave();
    void onSaveAs();
    void onLoad();

private:
    // Module management
    ModuleManager* moduleManager_;
    ModuleLauncherPanel* launcherPanel_;
    QStackedWidget* moduleStack_;
    QSplitter* mainSplitter_;

    // Edit menu actions
    QAction* undoAction_;
    QAction* redoAction_;
    QAction* preferencesAction_;

    // File menu actions
    QAction* saveAction_;
    QAction* saveAsAction_;
    QAction* loadAction_;

    // View menu actions
    QAction* showArchivedAction_;

    // Menus
    QMenu* fileMenu_;
    QMenu* editMenu_;
    QMenu* viewMenu_;
    QMenu* helpMenu_;

    // Connection tracking for current module
    QList<QMetaObject::Connection> moduleConnections_;  ///< @brief Active module's signal/slot connections
};
