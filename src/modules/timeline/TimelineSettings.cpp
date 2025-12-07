// TimelineSettings.cpp


#include "TimelineSettings.h"
#include <QDate>


TimelineSettings& TimelineSettings::instance()
{
    static TimelineSettings instance;
    return instance;
}


TimelineSettings::TimelineSettings()
    : settings_("TestLeadToolbox", "Timeline")
{
    // Constructor initializes QSettings with organization and application name
}

// ============================================================================
// Deletion Preferences
// ============================================================================

bool TimelineSettings::showDeleteConfirmation() const
{
    return settings_.value("Deletion/ShowConfirmation", DEFAULT_SHOW_DELETE_CONFIRMATION).toBool();
}


void TimelineSettings::setShowDeleteConfirmation(bool show)
{
    settings_.setValue("Deletion/ShowConfirmation", show);
}


bool TimelineSettings::useSoftDelete() const
{
    return settings_.value("Deletion/UseSoftDelete", DEFAULT_USE_SOFT_DELETE).toBool();
}


void TimelineSettings::setUseSoftDelete(bool useSoftDelete)
{
    settings_.setValue("Deletion/UseSoftDelete", useSoftDelete);
}

// ============================================================================
// Auto-save Preferences
// ============================================================================

int TimelineSettings::autoSaveInterval() const
{
    return settings_.value("AutoSave/Interval", DEFAULT_AUTOSAVE_INTERVAL).toInt();
}


void TimelineSettings::setAutoSaveInterval(int intervalMs)
{
    settings_.setValue("AutoSave/Interval", intervalMs);
}


bool TimelineSettings::autoSaveEnabled() const
{
    return settings_.value("AutoSave/Enabled", DEFAULT_AUTOSAVE_ENABLED).toBool();
}


void TimelineSettings::setAutoSaveEnabled(bool enabled)
{
    settings_.setValue("AutoSave/Enabled", enabled);
}

// ============================================================================
// View Preferences
// ============================================================================

double TimelineSettings::defaultPixelsPerDay() const
{
    return settings_.value("View/PixelsPerDay", DEFAULT_PIXELS_PER_DAY).toDouble();
}


void TimelineSettings::setDefaultPixelsPerDay(double pixelsPerDay)
{
    settings_.setValue("View/PixelsPerDay", pixelsPerDay);
}


int TimelineSettings::sidePanelWidth() const
{
    return settings_.value("View/SidePanelWidth", DEFAULT_SIDE_PANEL_WIDTH).toInt();
}


void TimelineSettings::setSidePanelWidth(int width)
{
    settings_.setValue("View/SidePanelWidth", width);
}


bool TimelineSettings::sidePanelVisible() const
{
    return settings_.value("View/SidePanelVisible", DEFAULT_SIDE_PANEL_VISIBLE).toBool();
}


void TimelineSettings::setSidePanelVisible(bool visible)
{
    settings_.setValue("View/SidePanelVisible", visible);
}

// ============================================================================
// Scroll-to-Date Preferences
// ============================================================================

bool TimelineSettings::scrollAnimationEnabled() const
{
    return settings_.value("ScrollToDate/Animate", DEFAULT_SCROLL_ANIMATION_ENABLED).toBool();
}


void TimelineSettings::setScrollAnimationEnabled(bool enabled)
{
    settings_.setValue("ScrollToDate/Animate", enabled);
}


bool TimelineSettings::scrollHighlightEnabled() const
{
    return settings_.value("ScrollToDate/Highlight", DEFAULT_SCROLL_HIGHLIGHT_ENABLED).toBool();
}


void TimelineSettings::setScrollHighlightEnabled(bool enabled)
{
    settings_.setValue("ScrollToDate/Highlight", enabled);
}


int TimelineSettings::scrollHighlightRange() const
{
    return settings_.value("ScrollToDate/HighlightRange", DEFAULT_SCROLL_HIGHLIGHT_RANGE).toInt();
}


void TimelineSettings::setScrollHighlightRange(int days)
{
    settings_.setValue("ScrollToDate/HighlightRange", days);
}

// ============================================================================
// Today Tab Preferences
// ============================================================================

bool TimelineSettings::todayTabUseCustomDate() const
{
    return settings_.value("TodayTab/UseCustomDate", DEFAULT_TODAY_TAB_USE_CUSTOM_DATE).toBool();
}


void TimelineSettings::setTodayTabUseCustomDate(bool useCustom)
{
    settings_.setValue("TodayTab/UseCustomDate", useCustom);
}


QDate TimelineSettings::todayTabCustomDate() const
{
    QString dateStr = settings_.value("TodayTab/CustomDate", QString()).toString();

    if (dateStr.isEmpty())
    {
        return QDate::currentDate();
    }

    QDate date = QDate::fromString(dateStr, Qt::ISODate);

    return date.isValid() ? date : QDate::currentDate();
}


void TimelineSettings::setTodayTabCustomDate(const QDate& date)
{
    if (date.isValid())
    {
        settings_.setValue("TodayTab/CustomDate", date.toString(Qt::ISODate));
    }
}


// ============================================================================
// Lookahead Tab Preferences
// ============================================================================

int TimelineSettings::lookaheadTabDays() const
{
    return settings_.value("LookaheadTab/Days", DEFAULT_LOOKAHEAD_TAB_DAYS).toInt();
}


void TimelineSettings::setLookaheadTabDays(int days)
{
    if (days >= 1 && days <= 365)  // Reasonable range
    {
        settings_.setValue("LookaheadTab/Days", days);
    }
}


// ============================================================================
// Reset & Utility
// ============================================================================

void TimelineSettings::resetToDefaults()
{
    settings_.clear();

    // Explicitly set all defaults (optional, as they'll be used anyway)
    setShowDeleteConfirmation(DEFAULT_SHOW_DELETE_CONFIRMATION);
    setUseSoftDelete(DEFAULT_USE_SOFT_DELETE);
    setAutoSaveInterval(DEFAULT_AUTOSAVE_INTERVAL);
    setAutoSaveEnabled(DEFAULT_AUTOSAVE_ENABLED);
    setDefaultPixelsPerDay(DEFAULT_PIXELS_PER_DAY);
    setSidePanelWidth(DEFAULT_SIDE_PANEL_WIDTH);
    setSidePanelVisible(DEFAULT_SIDE_PANEL_VISIBLE);
    setScrollAnimationEnabled(DEFAULT_SCROLL_ANIMATION_ENABLED);
    setScrollHighlightEnabled(DEFAULT_SCROLL_HIGHLIGHT_ENABLED);
    setScrollHighlightRange(DEFAULT_SCROLL_HIGHLIGHT_RANGE);
    setTodayTabUseCustomDate(DEFAULT_TODAY_TAB_USE_CUSTOM_DATE);
    setLookaheadTabDays(DEFAULT_LOOKAHEAD_TAB_DAYS);
}


// ============================================================================
// Side Panel Sort/Filter Preferences
// ============================================================================

TimelineSettings::SortMode TimelineSettings::sidePanelSortMode() const
{
    int mode = settings_.value("SidePanel/SortMode", DEFAULT_SORT_MODE).toInt();
    return static_cast<SortMode>(mode);
}


void TimelineSettings::setSidePanelSortMode(SortMode mode)
{
    settings_.setValue("SidePanel/SortMode", static_cast<int>(mode));
}


QSet<TimelineEventType> TimelineSettings::sidePanelFilterTypes() const
{
    QSet<TimelineEventType> types;

    // Load saved filter types
    QStringList savedTypes = settings_.value("SidePanel/FilterTypes").toStringList();

    if (savedTypes.isEmpty())
    {
        // Default: show all types
        types.insert(TimelineEventType_Meeting);
        types.insert(TimelineEventType_Action);
        types.insert(TimelineEventType_TestEvent);
        types.insert(TimelineEventType_Reminder);
        types.insert(TimelineEventType_JiraTicket);
    }
    else
    {
        for (const QString& typeStr : savedTypes)
        {
            bool ok;
            int typeInt = typeStr.toInt(&ok);
            if (ok)
            {
                types.insert(static_cast<TimelineEventType>(typeInt));
            }
        }

        // *** FIX: Auto-include newly added event types for backward compatibility ***
        // Define all known event types
        QSet<TimelineEventType> allKnownTypes = {
            TimelineEventType_Meeting,
            TimelineEventType_Action,
            TimelineEventType_TestEvent,
            TimelineEventType_Reminder,
            TimelineEventType_JiraTicket
        };

        // Add any missing types (ensures new types appear by default)
        for (TimelineEventType knownType : allKnownTypes)
        {
            if (!types.contains(knownType))
            {
                types.insert(knownType);
                qDebug() << "Auto-adding new event type to filter:" << static_cast<int>(knownType);
            }
        }
    }

    return types;
}


void TimelineSettings::setSidePanelFilterTypes(const QSet<TimelineEventType>& types)
{
    QStringList typeStrings;
    for (TimelineEventType type : types)
    {
        typeStrings.append(QString::number(static_cast<int>(type)));
    }
    settings_.setValue("SidePanel/FilterTypes", typeStrings);
}
