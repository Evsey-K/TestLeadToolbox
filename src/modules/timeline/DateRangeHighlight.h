// DateRangeHighlight.h

#pragma once
#include <QGraphicsRectItem>
#include <QObject>
#include <QDate>
#include <QPropertyAnimation>

class TimelineCoordinateMapper;

/**
 * @class DateRangeHighlight
 * @brief Semi-transparent overlay highlighting a date range on the timeline
 *
 * Visual feedback for "Go to Date" feature showing the target date range.
 * Features:
 * - Semi-transparent colored rectangle
 * - Dashed border
 * - Automatic fade-out after delay
 * - Clean removal after animation
 */
class DateRangeHighlight : public QObject, public QGraphicsRectItem
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    /**
     * @brief Construct a date range highlight overlay
     * @param centerDate Target date (center of highlight)
     * @param rangeDays Number of days before/after center to highlight
     * @param mapper Coordinate mapper for date-to-pixel conversion
     * @param timelineHeight Height of timeline area
     * @param parent Optional parent graphics item
     */
    explicit DateRangeHighlight(const QDate& centerDate,
                                int rangeDays,
                                TimelineCoordinateMapper* mapper,
                                double timelineHeight,
                                QGraphicsItem* parent = nullptr);

    /**
     * @brief Destructor - ensures animation cleanup
     */
    ~DateRangeHighlight();

    /**
     * @brief Update the highlighted range
     * @param centerDate New center date
     * @param rangeDays New range in days
     */
    void updateRange(const QDate& centerDate, int rangeDays);

    /**
     * @brief Start fade-out animation
     * @param delayMs Delay before starting fade (default: 2000ms)
     * @param fadeDurationMs Duration of fade animation (default: 1000ms)
     */
    void fadeOut(int delayMs = 2000, int fadeDurationMs = 1000);

    /**
     * @brief Paint the highlight overlay
     */
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

signals:
    /**
     * @brief Emitted when fade-out animation completes
     */
    void fadeCompleted();

private slots:
    /**
     * @brief Handle animation completion
     */
    void onFadeFinished();

private:
    /**
     * @brief Calculate rectangle from date range
     */
    QRectF calculateRect() const;

    QDate centerDate_;                      ///< Center date of highlight
    int rangeDays_;                         ///< Days before/after center
    TimelineCoordinateMapper* mapper_;      ///< Coordinate mapper (not owned)
    double timelineHeight_;                 ///< Height of timeline
    QPropertyAnimation* fadeAnimation_;     ///< Fade-out animation
    qreal currentOpacity_;                  ///< Current opacity (0.0 - 1.0)

    // Visual styling constants
    static constexpr qreal INITIAL_OPACITY = 0.25;      ///< Starting opacity
    static constexpr int BORDER_WIDTH = 2;               ///< Border thickness
};
