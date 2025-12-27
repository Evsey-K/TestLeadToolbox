// src/app/MainWindow.cpp

#include "MainWindow.h"
#include "ModuleManager.h"
#include "ModuleLauncherPanel.h"
#include "modules/timeline/TimelineModule.h"
#include "modules/linkbudget/LinkBudgetModule.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStatusBar>
#include <QStackedWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , moduleManager_(nullptr)
    , launcherPanel_(nullptr)
    , moduleStack_(nullptr)
    , mainSplitter_(nullptr)
    , undoAction_(nullptr)
    , redoAction_(nullptr)
{
    setWindowTitle("TestLeadToolbox");
    resize(1400, 900);

    // Create module manager
    moduleManager_ = new ModuleManager(this);

    // Register modules
    setupModules();

    // Setup UI
    setupUI();

    // Setup menu bar
    setupMenuBar();

    // Connect signals
    connectSignals();

    // Status bar
    statusBar()->showMessage("Ready - Select a module to begin");
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupModules()
{
    // Register Timeline Module
    auto timelineModule = std::make_unique<TimelineModule>();
    moduleManager_->registerModule(std::move(timelineModule));

    // Register Link Budget Calculator Module
    auto linkBudgetModule = std::make_unique<LinkBudgetModule>();
    moduleManager_->registerModule(std::move(linkBudgetModule));

    qDebug() << "MainWindow: Registered" << moduleManager_->moduleIds().count() << "modules";
}

void MainWindow::setupUI()
{
    // Create main splitter (launcher panel | module area)
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);

    // Create launcher panel
    launcherPanel_ = new ModuleLauncherPanel(moduleManager_, this);
    mainSplitter_->addWidget(launcherPanel_);

    // Create stacked widget for modules
    moduleStack_ = new QStackedWidget(this);
    moduleStack_->setContentsMargins(0, 0, 0, 0);
    mainSplitter_->addWidget(moduleStack_);

    // Set splitter sizes (launcher gets 250px, rest goes to module)
    mainSplitter_->setSizes({250, 1150});
    mainSplitter_->setCollapsible(0, false); // Don't allow collapsing launcher
    mainSplitter_->setCollapsible(1, false); // Don't allow collapsing module area

    setCentralWidget(mainSplitter_);
}

void MainWindow::setupMenuBar()
{
    setupFileMenu();
    setupEditMenu();
    setupViewMenu();
    setupHelpMenu();
}

void MainWindow::setupFileMenu()
{
    fileMenu_ = menuBar()->addMenu("&File");

    // New
    auto newAction = fileMenu_->addAction("&New");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, [this]() {
        QMessageBox::information(this, "New", "New feature depends on active module");
    });

    fileMenu_->addSeparator();

    // Save
    saveAction_ = fileMenu_->addAction("&Save");
    saveAction_->setShortcut(QKeySequence::Save);
    connect(saveAction_, &QAction::triggered, this, &MainWindow::onSave);

    // Save As
    saveAsAction_ = fileMenu_->addAction("Save &As...");
    saveAsAction_->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction_, &QAction::triggered, this, &MainWindow::onSaveAs);

    // Load
    loadAction_ = fileMenu_->addAction("&Load...");
    loadAction_->setShortcut(QKeySequence::Open);
    connect(loadAction_, &QAction::triggered, this, &MainWindow::onLoad);

    fileMenu_->addSeparator();

    // Exit
    auto exitAction = fileMenu_->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::setupEditMenu()
{
    editMenu_ = menuBar()->addMenu("&Edit");

    // Undo action
    undoAction_ = editMenu_->addAction("&Undo");
    undoAction_->setShortcut(QKeySequence::Undo);
    undoAction_->setEnabled(false); // Module will enable when appropriate

    // Redo action
    redoAction_ = editMenu_->addAction("&Redo");
    redoAction_->setShortcut(QKeySequence::Redo);
    redoAction_->setEnabled(false); // Module will enable when appropriate

    editMenu_->addSeparator();

    // Preferences
    preferencesAction_ = editMenu_->addAction("&Preferences...");
    connect(preferencesAction_, &QAction::triggered, this, &MainWindow::onShowPreferences);
}

void MainWindow::setupViewMenu()
{
    viewMenu_ = menuBar()->addMenu("&View");

    // This menu will be populated by active modules as needed
    showArchivedAction_ = viewMenu_->addAction("Show &Archived Events");
    connect(showArchivedAction_, &QAction::triggered, this, &MainWindow::onShowArchivedEvents);
    showArchivedAction_->setEnabled(false); // Only enabled when timeline is active
}

void MainWindow::setupHelpMenu()
{
    helpMenu_ = menuBar()->addMenu("&Help");

    auto aboutAction = helpMenu_->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onShowAbout);
}

void MainWindow::connectSignals()
{
    // Launcher panel requests module activation
    connect(launcherPanel_, &ModuleLauncherPanel::moduleLaunchRequested,
            this, &MainWindow::onModuleLaunchRequested);

    // Module manager notifies of activation
    connect(moduleManager_, &ModuleManager::moduleActivated,
            this, &MainWindow::onModuleActivated);
}

void MainWindow::onModuleLaunchRequested(const QString& moduleId)
{
    qDebug() << "MainWindow: Module launch requested:" << moduleId;

    // Check for unsaved changes in current module
    QString currentId = moduleManager_->currentModuleId();
    if (!currentId.isEmpty()) {
        IModule* currentMod = moduleManager_->module(currentId);
        if (currentMod && currentMod->hasUnsavedChanges()) {
            auto reply = QMessageBox::question(
                this,
                "Unsaved Changes",
                QString("Module '%1' has unsaved changes. Do you want to save before switching?")
                    .arg(currentMod->name()),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
                );

            if (reply == QMessageBox::Save) {
                if (!currentMod->save()) {
                    QMessageBox::warning(this, "Save Failed", "Failed to save module data");
                    return;
                }
            } else if (reply == QMessageBox::Cancel) {
                return; // Don't switch modules
            }
        }
    }

    // Get or create module widget
    QWidget* moduleWidget = moduleManager_->getModuleWidget(moduleId, moduleStack_);
    if (!moduleWidget) {
        QMessageBox::critical(this, "Error", "Failed to load module: " + moduleId);
        return;
    }

    // Add to stack if not already present
    int index = moduleStack_->indexOf(moduleWidget);
    if (index == -1) {
        index = moduleStack_->addWidget(moduleWidget);
    }

    // Switch to module
    moduleStack_->setCurrentIndex(index);

    // Activate module
    moduleManager_->activateModule(moduleId);
}

void MainWindow::onModuleActivated(const QString& moduleId)
{
    qDebug() << "MainWindow: Module activated:" << moduleId;

    IModule* mod = moduleManager_->module(moduleId);
    if (mod) {
        statusBar()->showMessage("Active: " + mod->name());

        // Enable/disable menu items based on module
        // Timeline-specific actions
        bool isTimeline = (moduleId == "timeline");
        showArchivedAction_->setEnabled(isTimeline);

        // Update window title
        setWindowTitle("TestLeadToolbox - " + mod->name());
    }
}

void MainWindow::onSave()
{
    QString currentId = moduleManager_->currentModuleId();
    if (currentId.isEmpty()) {
        QMessageBox::information(this, "Save", "No active module to save");
        return;
    }

    IModule* mod = moduleManager_->module(currentId);
    if (mod) {
        if (mod->save()) {
            statusBar()->showMessage("Saved successfully", 3000);
        } else {
            QMessageBox::warning(this, "Save Failed", "Failed to save module data");
        }
    }
}

void MainWindow::onSaveAs()
{
    QString currentId = moduleManager_->currentModuleId();
    if (currentId.isEmpty()) {
        QMessageBox::information(this, "Save As", "No active module");
        return;
    }

    // Module-specific save as logic would go here
    QMessageBox::information(this, "Save As", "Save As for current module coming soon");
}

void MainWindow::onLoad()
{
    QString currentId = moduleManager_->currentModuleId();
    if (currentId.isEmpty()) {
        QMessageBox::information(this, "Load", "No active module");
        return;
    }

    // Module-specific load logic would go here
    QMessageBox::information(this, "Load", "Load for current module coming soon");
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Check all modules for unsaved changes
    QStringList modulesWithChanges;
    for (const QString& moduleId : moduleManager_->moduleIds()) {
        IModule* mod = moduleManager_->module(moduleId);
        if (mod && mod->hasUnsavedChanges()) {
            modulesWithChanges.append(mod->name());
        }
    }

    if (!modulesWithChanges.isEmpty()) {
        QString message = "The following modules have unsaved changes:\n\n";
        message += modulesWithChanges.join("\n");
        message += "\n\nDo you want to quit without saving?";

        auto reply = QMessageBox::question(
            this,
            "Unsaved Changes",
            message,
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
    }

    event->accept();
}

void MainWindow::onShowArchivedEvents()
{
    // This is timeline-specific, would need to be refactored
    QMessageBox::information(this, "Archived Events", "Feature available in Timeline module");
}

void MainWindow::onShowPreferences()
{
    QMessageBox::information(this, "Preferences", "Global preferences dialog coming soon");
}

void MainWindow::onShowAbout()
{
    QMessageBox::about(
        this,
        "About TestLeadToolbox",
        "<h2>TestLeadToolbox</h2>"
        "<p>A modular engineering utility suite for test leads</p>"
        "<p>Version 2.0</p>"
        "<p>Built with Qt6 and C++20</p>"
        );
}
