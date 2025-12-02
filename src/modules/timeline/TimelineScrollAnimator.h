// TimelineScrollAnimator.h

#pragma once
#include <QObject>
#include <QPropertyAnimation>
#include <QDate>

class QGraphicsView;
class QScrollBar;
class TimelineCoordinateMapper;

/**
 * @class TimelineScrollAnimator
 * @brief Handles smooth animated scrolling to specific dates on timeline
 *
 * Features:
 * - Smooth easing animation
 * - Scroll to specific date
 * - Center target date in view
 * - Optional highlight overlay
 */
class TimelineScrollAnimator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int scrollValue READ scrollValue WRITE setScrollValue)

public:
    /**
     * @brief Construct scroll animator
     * @param view Graphics view to animate
     * @param mapper Coordinate mapper for date-to-position conversion
     * @param parent Qt parent
     */
    explicit TimelineScrollAnimator(QGraphicsView* view,
                                    TimelineCoordinateMapper* mapper,
                                    QObject* parent = nullptr);

    /**
     * @brief Smoothly scroll to target date
     * @param targetDate Date to scroll to
     * @param animate If true, uses smooth animation; if false, jumps immediately
     * @param durationMs Animation duration in milliseconds
     */
    void scrollToDate(const QDate& targetDate, bool animate = true, int durationMs = 500);

    /**
     * @brief Get current scroll value (for animation)
     */
    int scrollValue() const;

    /**
     * @brief Set scroll value (for animation)
     */
    void setScrollValue(int value);

signals:
    /**
     * @brief Emitted when scroll animation completes
     */
    void scrollCompleted(const QDate& targetDate);

    /**
     * @brief Emitted when scroll animation is cancelled
     */
    void scrollCancelled();

private slots:
    /**
     * @brief Handle animation finished
     */
    void onAnimationFinished();

private:
    QGraphicsView* view_;                   ///< View to animate (not owned)
    TimelineCoordinateMapper* mapper_;      ///< Coordinate mapper (not owned)
    QScrollBar* scrollBar_;                 ///< Horizontal scroll bar
    QPropertyAnimation* animation_;         ///< Scroll animation
    QDate targetDate_;                      ///< Current target date
};
