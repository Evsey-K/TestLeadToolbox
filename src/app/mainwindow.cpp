// MainWindow.cpp


#include "MainWindow.h"
#include "modules/timeline/TimelineModule.h"
#include "modules/timeline/ArchivedEventsDialog.h"
#include "modules/timeline/TimelineSettings.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QStatusBar>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , timelineModule_(nullptr)
    , undoAction_(nullptr)
    , redoAction_(nullptr)
{
    setWindowTitle("Test Lead Toolbox");
    resize(1200, 800);

    // Create timeline module and set as central widget
    timelineModule_ = new TimelineModule(this);
    setCentralWidget(timelineModule_);

    // Setup menu bar
    setupMenuBar();

    // Connect undo stack to menu actions
    connectUndoStack();

    // Status bar
    statusBar()->showMessage("Ready");
}


MainWindow::~MainWindow()
{
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

    // New Timeline
    auto newAction = fileMenu_->addAction("&New Timeline");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, [this]() {
        QMessageBox::information(this, "New Timeline", "New timeline feature coming soon!");
    });

    fileMenu_->addSeparator();

    // Save
    auto saveAction = fileMenu_->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, [this]() {
        if (timelineModule_) {
            // Trigger the save action in TimelineModule
            // You'll need to expose onSaveClicked() as a public slot or add a save() method
        }
    });

    // Save As
    auto saveAsAction = fileMenu_->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, [this]() {
        if (timelineModule_) {
            // Trigger the save as action in TimelineModule
        }
    });

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
    undoAction_->setShortcut(QKeySequence::Undo);  // Ctrl+Z
    undoAction_->setEnabled(false);

    // Redo action
    redoAction_ = editMenu_->addAction("&Redo");
    redoAction_->setShortcut(QKeySequence::Redo);  // Ctrl+Y or Ctrl+Shift+Z
    redoAction_->setEnabled(false);

    editMenu_->addSeparator();

    // Preferences
    preferencesAction_ = editMenu_->addAction("&Preferences...");
    preferencesAction_->setShortcut(QKeySequence::Preferences);
    connect(preferencesAction_, &QAction::triggered, this, &MainWindow::onShowPreferences);
}


void MainWindow::setupViewMenu()
{
    viewMenu_ = menuBar()->addMenu("&View");

    // Show Archived Events
    showArchivedAction_ = viewMenu_->addAction("📦 &Archived Events...");
    showArchivedAction_->setShortcut(QKeySequence("Ctrl+Shift+A"));
    showArchivedAction_->setToolTip("View and restore archived events");
    connect(showArchivedAction_, &QAction::triggered, this, &MainWindow::onShowArchivedEvents);

    viewMenu_->addSeparator();

    // Refresh View
    auto refreshAction = viewMenu_->addAction("&Refresh");
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, [this]() {
        if (timelineModule_)
        {
            statusBar()->showMessage("View refreshed", 2000);
        }
    });
}


void MainWindow::setupHelpMenu()
{
    helpMenu_ = menuBar()->addMenu("&Help");

    // About
    auto aboutAction = helpMenu_->addAction("&About...");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onShowAbout);

    // Keyboard Shortcuts
    auto shortcutsAction = helpMenu_->addAction("&Keyboard Shortcuts");
    connect(shortcutsAction, &QAction::triggered, [this]() {
        QString shortcuts =
            "<h3>Keyboard Shortcuts</h3>"
            "<table>"
            "<tr><td><b>Ctrl+Z</b></td><td>Undo</td></tr>"
            "<tr><td><b>Ctrl+Y</b></td><td>Redo</td></tr>"
            "<tr><td><b>Ctrl+Shift+A</b></td><td>View Archived Events</td></tr>"
            "<tr><td><b>Delete</b></td><td>Delete selected event(s)</td></tr>"
            "<tr><td><b>Ctrl+Wheel</b></td><td>Zoom timeline</td></tr>"
            "</table>";

        QMessageBox::information(this, "Keyboard Shortcuts", shortcuts);
    });
}


void MainWindow::connectUndoStack()
{
    if (!timelineModule_)
    {
        return;
    }

    QUndoStack* undoStack = timelineModule_->undoStack();
    if (!undoStack)
    {
        qWarning() << "TimelineModule has no undo stack!";
        return;
    }

    // Connect undo/redo actions to stack
    connect(undoAction_, &QAction::triggered, undoStack, &QUndoStack::undo);
    connect(redoAction_, &QAction::triggered, undoStack, &QUndoStack::redo);

    // Update action states based on stack
    connect(undoStack, &QUndoStack::canUndoChanged, undoAction_, &QAction::setEnabled);
    connect(undoStack, &QUndoStack::canRedoChanged, redoAction_, &QAction::setEnabled);

    // Update action text with command description
    connect(undoStack, &QUndoStack::undoTextChanged, [this](const QString& text) {
        if (text.isEmpty())
        {
            undoAction_->setText("&Undo");
        }
        else
        {
            undoAction_->setText(QString("&Undo %1").arg(text));
        }
    });

    connect(undoStack, &QUndoStack::redoTextChanged, [this](const QString& text) {
        if (text.isEmpty())
        {
            redoAction_->setText("&Redo");
        }
        else
        {
            redoAction_->setText(QString("&Redo %1").arg(text));
        }
    });

    // Initial state update
    undoAction_->setEnabled(undoStack->canUndo());
    redoAction_->setEnabled(undoStack->canRedo());
}


void MainWindow::onShowArchivedEvents()
{
    if (!timelineModule_)
    {
        return;
    }

    ArchivedEventsDialog dialog(
        timelineModule_->model(),
        timelineModule_->undoStack(),
        this
        );

    dialog.exec();
}


void MainWindow::onShowPreferences()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Preferences");
    dialog.setMinimumWidth(400);

    auto layout = new QVBoxLayout(&dialog);

    // Deletion preferences group
    auto deletionGroup = new QGroupBox("Deletion");
    auto deletionLayout = new QVBoxLayout(deletionGroup);

    auto confirmCheckbox = new QCheckBox("Show confirmation dialog when deleting events");
    confirmCheckbox->setChecked(TimelineSettings::instance().showDeleteConfirmation());

    auto softDeleteCheckbox = new QCheckBox("Use archive mode (soft delete) - allows event restoration");
    softDeleteCheckbox->setChecked(TimelineSettings::instance().useSoftDelete());

    deletionLayout->addWidget(confirmCheckbox);
    deletionLayout->addWidget(softDeleteCheckbox);
    layout->addWidget(deletionGroup);

    // Auto-save preferences group
    auto autoSaveGroup = new QGroupBox("Auto-Save");
    auto autoSaveLayout = new QVBoxLayout(autoSaveGroup);

    auto autoSaveCheckbox = new QCheckBox("Enable auto-save");
    autoSaveCheckbox->setChecked(TimelineSettings::instance().autoSaveEnabled());

    autoSaveLayout->addWidget(autoSaveCheckbox);
    autoSaveLayout->addWidget(new QLabel("Auto-save occurs every 5 minutes when enabled"));
    layout->addWidget(autoSaveGroup);

    layout->addStretch();

    // Buttons
    auto buttonLayout = new QHBoxLayout();
    auto okButton = new QPushButton("OK");
    auto cancelButton = new QPushButton("Cancel");
    auto resetButton = new QPushButton("Reset to Defaults");

    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // Connections
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(resetButton, &QPushButton::clicked, [&]() {
        auto result = QMessageBox::question(
            &dialog,
            "Reset Preferences",
            "Are you sure you want to reset all preferences to defaults?",
            QMessageBox::Yes | QMessageBox::No
            );

        if (result == QMessageBox::Yes)
        {
            TimelineSettings::instance().resetToDefaults();

            confirmCheckbox->setChecked(TimelineSettings::instance().showDeleteConfirmation());
            softDeleteCheckbox->setChecked(TimelineSettings::instance().useSoftDelete());
            autoSaveCheckbox->setChecked(TimelineSettings::instance().autoSaveEnabled());
        }
    });

    // Execute dialog
    if (dialog.exec() == QDialog::Accepted)
    {
        TimelineSettings::instance().setShowDeleteConfirmation(confirmCheckbox->isChecked());
        TimelineSettings::instance().setUseSoftDelete(softDeleteCheckbox->isChecked());
        TimelineSettings::instance().setAutoSaveEnabled(autoSaveCheckbox->isChecked());

        statusBar()->showMessage("Preferences saved", 2000);
    }
}


void MainWindow::onShowAbout()
{
    QString aboutText =
        "<h2>Test Lead Toolbox</h2>"
        "<p>Version 1.0</p>"
        "<p>A comprehensive timeline management tool for test leads.</p>"
        "<p>&copy; 2025 Test Lead Toolbox Team</p>"
        "<br>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Interactive timeline visualization</li>"
        "<li>Event management with drag &amp; drop</li>"
        "<li>Undo/Redo support</li>"
        "<li>Archive and restore events</li>"
        "<li>Export to CSV, PDF, PNG</li>"
        "</ul>";

    QMessageBox::about(this, "About Test Lead Toolbox", aboutText);
}


void MainWindow::closeEvent(QCloseEvent* event)
{
    // TimelineModule handles auto-save on destruction
    event->accept();
}
