// MainWindow.h

#pragma once
#include <QMainWindow>

class TimelineModule;
class QAction;
class QMenu;

/**
 * @class MainWindow
 * @brief Main application window with Edit menu and undo/redo support
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
    void setupMenuBar();
    void setupFileMenu();
    void setupEditMenu();
    void setupViewMenu();
    void setupHelpMenu();
    void connectUndoStack();

private slots:
    void onShowArchivedEvents();
    void onShowPreferences();
    void onShowAbout();

private:
    TimelineModule* timelineModule_;

    // Edit menu actions
    QAction* undoAction_;
    QAction* redoAction_;
    QAction* preferencesAction_;

    // View menu actions
    QAction* showArchivedAction_;

    // Menus
    QMenu* fileMenu_;
    QMenu* editMenu_;
    QMenu* viewMenu_;
    QMenu* helpMenu_;
};
