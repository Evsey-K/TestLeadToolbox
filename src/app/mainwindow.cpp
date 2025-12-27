// MainWindow.cpp - COMPLETE CORRECTED VERSION
// Key fix: Use moduleStack_->currentWidget() to get the actual widget instance

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

    // Save - Delegates to active module
    saveAction_ = fileMenu_->addAction("&Save");
    saveAction_->setShortcut(QKeySequence::Save);
    saveAction_->setEnabled(false);  // Disabled until module active
    connect(saveAction_, &QAction::triggered, this, &MainWindow::onSave);

    // Save As - Delegates to active module
    saveAsAction_ = fileMenu_->addAction("Save &As...");
    saveAsAction_->setShortcut(QKeySequence::SaveAs);
    saveAsAction_->setEnabled(false);  // Disabled until module active
    connect(saveAsAction_, &QAction::triggered, this, &MainWindow::onSaveAs);

    // Load - Delegates to active module
    loadAction_ = fileMenu_->addAction("&Open...");
    loadAction_->setShortcut(QKeySequence::Open);
    loadAction_->setEnabled(false);  // Disabled until module active
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

    // Undo action - Will be connected to module's undo stack
    undoAction_ = editMenu_->addAction("&Undo");
    undoAction_->setShortcut(QKeySequence::Undo);
    undoAction_->setEnabled(false);

    // Redo action - Will be connected to module's undo stack
    redoAction_ = editMenu_->addAction("&Redo");
    redoAction_->setShortcut(QKeySequence::Redo);
    redoAction_->setEnabled(false);

    editMenu_->addSeparator();

    // Preferences
    preferencesAction_ = editMenu_->addAction("&Preferences...");
    connect(preferencesAction_, &QAction::triggered, this, &MainWindow::onShowPreferences);
}

void MainWindow::setupViewMenu()
{
    viewMenu_ = menuBar()->addMenu("&View");

    // REMOVED: showArchivedAction - use toolbar Ctrl+Shift+A instead
    // This was causing issues and is redundant with the Timeline toolbar button
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
                    // Need Save As - get widget and call saveAs()
                    QWidget* currentWidget = moduleStack_->currentWidget();
                    TimelineModule* timelineModule = qobject_cast<TimelineModule*>(currentWidget);
                    if (timelineModule) {
                        timelineModule->saveAs();
                    }
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
    if (!mod) {
        qWarning() << "MainWindow: Module not found:" << moduleId;
        return;
    }

    // Update status and title
    statusBar()->showMessage("Active: " + mod->name());
    setWindowTitle("TestLeadToolbox - " + mod->name());

    // Disconnect previous module's actions and connect new module
    disconnectModuleActions();
    connectModuleActions();

    // Notify module it's been activated
    mod->onActivate();
}

void MainWindow::connectModuleActions()
{
    QString currentId = moduleManager_->currentModuleId();
    if (currentId.isEmpty()) return;

    // Enable file menu actions
    saveAction_->setEnabled(true);
    saveAsAction_->setEnabled(true);
    loadAction_->setEnabled(true);

    // Get the current widget from the stack
    QWidget* currentWidget = moduleStack_->currentWidget();
    if (!currentWidget) return;

    // Try to cast to TimelineModule for undo/redo support
    TimelineModule* timelineModule = qobject_cast<TimelineModule*>(currentWidget);

    if (timelineModule && timelineModule->undoStack()) {
        QUndoStack* stack = timelineModule->undoStack();

        // Connect undo/redo actions directly to stack
        moduleConnections_.append(
            connect(undoAction_, &QAction::triggered, stack, &QUndoStack::undo)
            );
        moduleConnections_.append(
            connect(redoAction_, &QAction::triggered, stack, &QUndoStack::redo)
            );

        // Update enabled state dynamically
        moduleConnections_.append(
            connect(stack, &QUndoStack::canUndoChanged,
                    undoAction_, &QAction::setEnabled)
            );
        moduleConnections_.append(
            connect(stack, &QUndoStack::canRedoChanged,
                    redoAction_, &QAction::setEnabled)
            );

        // Update action text with command names
        moduleConnections_.append(
            connect(stack, &QUndoStack::undoTextChanged, [this](const QString& text) {
                undoAction_->setText(text.isEmpty() ? "&Undo" : QString("&Undo %1").arg(text));
            })
            );
        moduleConnections_.append(
            connect(stack, &QUndoStack::redoTextChanged, [this](const QString& text) {
                redoAction_->setText(text.isEmpty() ? "&Redo" : QString("&Redo %1").arg(text));
            })
            );

        // Set initial state
        undoAction_->setEnabled(stack->canUndo());
        redoAction_->setEnabled(stack->canRedo());
        undoAction_->setText(stack->undoText().isEmpty() ? "&Undo" : QString("&Undo %1").arg(stack->undoText()));
        redoAction_->setText(stack->redoText().isEmpty() ? "&Redo" : QString("&Redo %1").arg(stack->redoText()));

        qDebug() << "MainWindow: Connected undo/redo to module's undo stack";
    } else {
        // No undo stack available
        undoAction_->setEnabled(false);
        redoAction_->setEnabled(false);
        undoAction_->setText("&Undo");
        redoAction_->setText("&Redo");
        qDebug() << "MainWindow: Module does not provide undo stack";
    }
}

void MainWindow::disconnectModuleActions()
{
    // Disconnect all previous module connections
    for (const QMetaObject::Connection& conn : moduleConnections_) {
        disconnect(conn);
    }
    moduleConnections_.clear();

    // Disable actions when no module connected
    saveAction_->setEnabled(false);
    saveAsAction_->setEnabled(false);
    loadAction_->setEnabled(false);
    undoAction_->setEnabled(false);
    redoAction_->setEnabled(false);
}

void MainWindow::onSave()
{
    QString currentId = moduleManager_->currentModuleId();
    if (currentId.isEmpty()) {
        QMessageBox::information(this, "Save", "No active module to save");
        return;
    }

    IModule* mod = moduleManager_->module(currentId);
    if (!mod) return;

    // Try to save using module's save() method
    bool success = mod->save();

    if (success) {
        statusBar()->showMessage("Saved successfully", 3000);
    } else {
        // Module doesn't have a file path yet, trigger Save As
        QWidget* currentWidget = moduleStack_->currentWidget();
        TimelineModule* timelineModule = qobject_cast<TimelineModule*>(currentWidget);
        if (timelineModule) {
            timelineModule->saveAs();
        } else {
            QMessageBox::warning(this, "Save Failed", "Module cannot be saved (no file path)");
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

    // Get the current widget and try to cast to TimelineModule
    QWidget* currentWidget = moduleStack_->currentWidget();
    TimelineModule* timelineModule = qobject_cast<TimelineModule*>(currentWidget);
    if (timelineModule) {
        timelineModule->saveAs();
    } else {
        QMessageBox::information(this, "Save As", "Save As not implemented for this module");
    }
}

void MainWindow::onLoad()
{
    QString currentId = moduleManager_->currentModuleId();
    if (currentId.isEmpty()) {
        QMessageBox::information(this, "Load", "No active module");
        return;
    }

    // Get the current widget and try to cast to TimelineModule
    QWidget* currentWidget = moduleStack_->currentWidget();
    TimelineModule* timelineModule = qobject_cast<TimelineModule*>(currentWidget);
    if (timelineModule) {
        timelineModule->load();
    } else {
        QMessageBox::information(this, "Load", "Load not implemented for this module");
    }
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
    // This method is no longer used - archived events accessible via toolbar Ctrl+Shift+A
    QMessageBox::information(this, "Archived Events",
                             "Use the Archive button in the Timeline toolbar (Ctrl+Shift+A)");
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
