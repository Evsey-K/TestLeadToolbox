// DateRangeHighlight.cpp

#include "DateRangeHighlight.h"
#include "TimelineCoordinateMapper.h"
#include <QGraphicsScene>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QTimer>

DateRangeHighlight::DateRangeHighlight(const QDate& centerDate,
                                       int rangeDays,
                                       TimelineCoordinateMapper* mapper,
                                       double timelineHeight,
                                       QGraphicsItem* parent)
    : QObject(nullptr)
    , QGraphicsRectItem(parent)
    , centerDate_(centerDate)
    , rangeDays_(rangeDays)
    , mapper_(mapper)
    , timelineHeight_(timelineHeight)
    , fadeAnimation_(nullptr)
    , currentOpacity_(INITIAL_OPACITY)
{
    // Calculate and set rectangle
    setRect(calculateRect());

    // Set Z-value to appear above date scale but below event items
    // Date scale is at Z=-1, events are at Z=0, so we use Z=-0.5
    setZValue(-0.5);

    // Enable opacity changes
    setOpacity(INITIAL_OPACITY);

    // Create fade animation
    fadeAnimation_ = new QPropertyAnimation(this, "opacity", this);
    fadeAnimation_->setEasingCurve(QEasingCurve::OutCubic);

    connect(fadeAnimation_, &QPropertyAnimation::finished, this, &DateRangeHighlight::onFadeFinished);
}

DateRangeHighlight::~DateRangeHighlight()
{
    if (fadeAnimation_)
    {
        fadeAnimation_->stop();
        delete fadeAnimation_;
    }
}

void DateRangeHighlight::updateRange(const QDate& centerDate, int rangeDays)
{
    centerDate_ = centerDate;
    rangeDays_ = rangeDays;

    // Recalculate rectangle
    setRect(calculateRect());
    update();
}

void DateRangeHighlight::fadeOut(int delayMs, int fadeDurationMs)
{
    // Use QTimer to delay the fade animation
    QTimer::singleShot(delayMs, this, [this, fadeDurationMs]()
                       {
                           if (fadeAnimation_)
                           {
                               fadeAnimation_->setDuration(fadeDurationMs);
                               fadeAnimation_->setStartValue(currentOpacity_);
                               fadeAnimation_->setEndValue(0.0);
                               fadeAnimation_->start();
                           }
                       });
}

void DateRangeHighlight::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Semi-transparent blue fill
    QColor fillColor(74, 144, 226);             // Light blue (#4A90E2)
    fillColor.setAlphaF(currentOpacity_);       //
    painter->setBrush(QBrush(fillColor));       //

    // Dashed border
    QColor borderColor(46, 92, 138);                        // Darker blue (#2E5C8A)
    borderColor.setAlphaF(currentOpacity_ * 2.0);           // Border more visible
    QPen pen(borderColor, BORDER_WIDTH, Qt::DashLine);      //
    painter->setPen(pen);                                   //

    // Draw the rectangle
    painter->drawRect(rect());
}

void DateRangeHighlight::onFadeFinished()
{
    // Emit signal for cleanup
    emit fadeCompleted();

    // Remove from scene and schedule deletion
    if (scene())
    {
        scene()->removeItem(this);
    }
    deleteLater();
}

QRectF DateRangeHighlight::calculateRect() const
{
    if (!mapper_)
    {
        return QRectF();
    }

    // Calculate date range
    QDate startDate = centerDate_.addDays(-rangeDays_);
    QDate endDate = centerDate_.addDays(rangeDays_);

    // Convert to scene coordinates
    double startX = mapper_->dateToX(startDate);
    double endX = mapper_->dateToX(endDate);

    double width = endX - startX;

    // Position below date scale (offset by date scale height)
    // Date scale height is approximately 60-70 pixels
    double yPos = 70.0;

    return QRectF(startX, yPos, width, timelineHeight_ - yPos);
}
