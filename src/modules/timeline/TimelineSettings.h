// TimelineSettings.h

#pragma once
#include <QSettings>
#include <QString>

/**
 * @class TimelineSettings
 * @brief Manages user preferences and settings for the Timeline module
 */
class TimelineSettings
{
public:
    static TimelineSettings& instance();

    // Deletion Preferences
    bool showDeleteConfirmation() const;
    void setShowDeleteConfirmation(bool show);
    bool useSoftDelete() const;
    void setUseSoftDelete(bool useSoftDelete);

    // Auto-save Preferences
    int autoSaveInterval() const;
    void setAutoSaveInterval(int intervalMs);
    bool autoSaveEnabled() const;
    void setAutoSaveEnabled(bool enabled);

    // View Preferences
    double defaultPixelsPerDay() const;
    void setDefaultPixelsPerDay(double pixelsPerDay);
    int sidePanelWidth() const;
    void setSidePanelWidth(int width);
    bool sidePanelVisible() const;
    void setSidePanelVisible(bool visible);

    // Scroll-to-Date Preferences
    bool scrollAnimationEnabled() const;
    void setScrollAnimationEnabled(bool enabled);
    bool scrollHighlightEnabled() const;
    void setScrollHighlightEnabled(bool enabled);
    int scrollHighlightRange() const;
    void setScrollHighlightRange(int days);

    // Reset & Utility
    void resetToDefaults();
    QSettings* qSettings() { return &settings_; }

private:
    TimelineSettings();
    ~TimelineSettings() = default;
    TimelineSettings(const TimelineSettings&) = delete;
    TimelineSettings& operator=(const TimelineSettings&) = delete;

    QSettings settings_;

    // Default values
    static constexpr bool DEFAULT_SHOW_DELETE_CONFIRMATION = true;
    static constexpr bool DEFAULT_USE_SOFT_DELETE = true;
    static constexpr int DEFAULT_AUTOSAVE_INTERVAL = 300000;
    static constexpr bool DEFAULT_AUTOSAVE_ENABLED = true;
    static constexpr double DEFAULT_PIXELS_PER_DAY = 20.0;
    static constexpr int DEFAULT_SIDE_PANEL_WIDTH = 350;
    static constexpr bool DEFAULT_SIDE_PANEL_VISIBLE = true;
    static constexpr bool DEFAULT_SCROLL_ANIMATION_ENABLED = true;
    static constexpr bool DEFAULT_SCROLL_HIGHLIGHT_ENABLED = false;
    static constexpr int DEFAULT_SCROLL_HIGHLIGHT_RANGE = 7;
};
