// TimelineModule.h
#pragma once
#include <QWidget>

// Forward declaration to reduce header dependencies
class TimelineView;

/**
 * @class TimelineModule
 * @brief Encapsulates the timeline view and its related UI components.
 *
 * TimelineModule acts as a container widget holding the TimelineView and
 * any additional controls or panels related to timeline interaction.
 */
class TimelineModule : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructs a TimelineModule instance with an optional parent widget.
     * @param parent Pointer to the parent widget, defaults to nullptr.
     */
    explicit TimelineModule(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~TimelineModule();

private:
    TimelineView *view = nullptr; ///< Pointer to the contained TimelineView widget.
};
