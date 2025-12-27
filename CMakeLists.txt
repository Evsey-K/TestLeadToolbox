# ============================================================================
#  TestLeadToolbox - CMake Build Configuration
#
#  This CMakeLists.txt defines the build configuration for the TestLeadToolbox
#  desktop application, a Qt-based tool that provides engineering utilities
#  for software test leads. This file sets compiler requirements, locates
#  Qt6 dependencies, organizes the project source tree, and generates the
#  application executable.
#
#  CMake is the authoritative build specification for this project and must be
#  kept in sync with the source-code structure. Any new module, UI screen,
#  or component should be added to the SRC list to ensure proper compilation.
# ============================================================================

cmake_minimum_required(VERSION 3.16)

# ----------------------------------------------------------------------------
# Project Metadata
# ----------------------------------------------------------------------------
# LANGUAGES CXX specifies that this project only uses modern C++.
# The project name is used as the executable target name unless overridden.

project(TestLeadToolbox LANGUAGES CXX)

# ----------------------------------------------------------------------------
# C++ Standard Configuration
# ----------------------------------------------------------------------------
# Enforces C++20, required for modern Qt idioms, smart pointers, std::optional,
# ranges, and other features used across the codebase.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------------------------------------------------
# Locate Qt6 Framework Dependencies
# ----------------------------------------------------------------------------
# This ensures the project links against Qt6 Core (base framework),
# Qt6 Gui (visual rendering), and Qt6 Widgets (UI widgets framework).
# REQUIRED means the build will fail immediately if Qt6 is not installed
# or cannot be located by CMake.

find_package(Qt6 REQUIRED COMPONENTS
                Core
                Gui
                Svg
                Xml
                Widgets
                PrintSupport)

# ----------------------------------------------------------------------------
# Qt Auto-Generation Settings
# ----------------------------------------------------------------------------

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ----------------------------------------------------------------------------
# Source File List
# ----------------------------------------------------------------------------
# Centralized list of .cpp implementation files that form the TestLeadToolbox
# binary. Headers are not strictly required in this list but may be added
# to improve IDE visibility.
#
# File organization follows a modular architecture:
#   - src/app/      → Application entry points and primary UI shell
#   - src/modules/  → Feature modules (timeline, reporting, automation, etc.)
#
# Additional modules should append their source files here.
# ----------------------------------------------------------------------------

set(SRC
    # -------------------- APP SHELL -----------------------------
    # ------------------------------------------------------------

    # App Shell - Main Entry Point
    src/app/main.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/app/mainwindow.ui

    # App Shell - Module Management
    src/app/ModuleManager.h
    src/app/ModuleManager.cpp
    src/app/ModuleLauncherPanel.h
    src/app/ModuleLauncherPanel.cpp

    # -------------------- TIMELINE MODULE -----------------------
    # ------------------------------------------------------------

    # Timeline Module - Core
    src/modules/timeline/TimelineModule.cpp
    src/modules/timeline/TimelineModule.h
    src/modules/timeline/TimelineModel.cpp
    src/modules/timeline/TimelineModel.h
    src/modules/timeline/TimelineCoordinateMapper.cpp
    src/modules/timeline/TimelineCoordinateMapper.h
    src/modules/timeline/TimelineView.cpp
    src/modules/timeline/TimelineView.h
    src/modules/timeline/TimelineScene.cpp
    src/modules/timeline/TimelineScene.h
    src/modules/timeline/TimelineItem.cpp
    src/modules/timeline/TimelineItem.h

    # Timeline Module - Dialogs
    src/modules/timeline/AddEventDialog.cpp
    src/modules/timeline/AddEventDialog.h
    src/modules/timeline/AddEventDialog.ui
    src/modules/timeline/EditEventDialog.h
    src/modules/timeline/EditEventDialog.cpp
    src/modules/timeline/EditEventDialog.ui
    src/modules/timeline/VersionSettingsDialog.cpp
    src/modules/timeline/VersionSettingsDialog.h
    src/modules/timeline/VersionSettingsDialog.ui
    src/modules/timeline/ScrollToDateDialog.h
    src/modules/timeline/ScrollToDateDialog.cpp
    src/modules/timeline/ScrollToDateDialog.ui

    # Timeline Module - UI Components
    src/modules/timeline/TimelineSidePanel.cpp
    src/modules/timeline/TimelineSidePanel.h
    src/modules/timeline/TimelineSidePanel.ui
    src/modules/timeline/TimelineLegend.h
    src/modules/timeline/TimelineLegend.cpp

    # Attachment UI Component
    src/modules/timeline/AttachmentListWidget.h
    src/modules/timeline/AttachmentListWidget.cpp

    # Timeline Module - Utilities
    src/modules/timeline/LaneAssigner.h
    src/modules/timeline/LaneAssigner.cpp
    src/modules/timeline/TimelineDateScale.cpp
    src/modules/timeline/TimelineDateScale.h
    src/modules/timeline/CurrentDateMarker.cpp
    src/modules/timeline/CurrentDateMarker.h
    src/modules/timeline/VersionBoundaryMarker.cpp
    src/modules/timeline/VersionBoundaryMarker.h

    # Timeline Module - Persistence
    src/modules/timeline/TimelineSerializer.h
    src/modules/timeline/TimelineSerializer.cpp
    src/modules/timeline/AutoSaveManager.h
    src/modules/timeline/AutoSaveManager.cpp
    src/modules/timeline/TimelineExporter.h
    src/modules/timeline/TimelineExporter.cpp

    # Timeline Module - Animation & Effects
    src/modules/timeline/TimelineScrollAnimator.h
    src/modules/timeline/TimelineScrollAnimator.cpp
    src/modules/timeline/DateRangeHighlight.h
    src/modules/timeline/DateRangeHighlight.cpp

    # Timeline Module - Undo/Redo
    src/modules/timeline/TimelineCommands.h
    src/modules/timeline/TimelineCommands.cpp

    # Timeline Module - Settings
    src/modules/timeline/TimelineSettings.h
    src/modules/timeline/TimelineSettings.cpp

    # Timeline Module - Enhanced Dialogs
    src/modules/timeline/ConfirmationDialog.h    
    src/modules/timeline/ArchivedEventsDialog.h
    src/modules/timeline/ArchivedEventsDialog.cpp
    src/modules/timeline/ArchivedEventsDialog.ui

    src/modules/timeline/SetTodayDateDialog.h
    src/modules/timeline/SetTodayDateDialog.cpp
    src/modules/timeline/SetTodayDateDialog.ui

    src/modules/timeline/SetLookaheadRangeDialog.h
    src/modules/timeline/SetLookaheadRangeDialog.cpp
    src/modules/timeline/SetLookaheadRangeDialog.ui

    # -------------------- LINK BUDGET MODULE --------------------
    # ------------------------------------------------------------

    # Link Budget Module - Core
    src/modules/linkbudget/LinkBudgetModule.h
    src/modules/linkbudget/LinkBudgetModule.cpp
    src/modules/linkbudget/LinkBudgetModel.h
    src/modules/linkbudget/LinkBudgetModel.cpp

    # -------------------- SHARED COMPONENTS ---------------------
    # ------------------------------------------------------------

    # Shared - Interfaces
    src/shared/interfaces/IModule.h

    # Shared - Models
    src/shared/models/AttachmentModel.h
    src/shared/models/AttachmentModel.cpp
)

# ----------------------------------------------------------------------------
# Executable Target Declaration
# ----------------------------------------------------------------------------
# Generates the main application executable using the source files defined
# above. ${PROJECT_NAME} automatically expands to "TestLeadToolbox".
# ----------------------------------------------------------------------------

add_executable(${PROJECT_NAME} ${SRC})

# ----------------------------------------------------------------------------
# Include Directory Setup
# ----------------------------------------------------------------------------
# Makes the entire 'src/' folder available for header lookups. This allows
# code to include headers with paths such as:
#
#       #include "app/MainWindow.h"
#       #include "modules/timeline/TimelineView.h"
#
# PRIVATE limits the include path visibility to this target only.
# ----------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PRIVATE src)

# ----------------------------------------------------------------------------
# Link Qt6 Framework Libraries
# ----------------------------------------------------------------------------
# Links the application against required Qt modules. Without this step,
# linker errors would occur because the Qt symbol implementations would not
# be available at build time.
# ----------------------------------------------------------------------------

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Svg
        Qt6::Xml
        Qt6::Widgets
        Qt6::PrintSupport
)
