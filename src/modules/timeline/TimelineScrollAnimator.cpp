// TimelineScrollAnimator.cpp

#include "TimelineScrollAnimator.h"
#include "TimelineCoordinateMapper.h"
#include <QGraphicsView>
#include <QScrollBar>
#include <QEasingCurve>

TimelineScrollAnimator::TimelineScrollAnimator(QGraphicsView* view,
                                               TimelineCoordinateMapper* mapper,
                                               QObject* parent)
    : QObject(parent)
    , view_(view)
    , mapper_(mapper)
    , scrollBar_(view->horizontalScrollBar())
{
    // Create animation for smooth scrolling
    animation_ = new QPropertyAnimation(this, "scrollValue", this);
    animation_->setEasingCurve(QEasingCurve::InOutCubic);

    connect(animation_, &QPropertyAnimation::finished, this, &TimelineScrollAnimator::onAnimationFinished);
}

void TimelineScrollAnimator::scrollToDate(const QDate& targetDate, bool animate, int durationMs)
{
    if (!targetDate.isValid() || !mapper_)
    {
        qWarning() << "Invalid target date or null mapper";
        return;
    }

    targetDate_ = targetDate;

    // Calculate X position for target date
    double targetX = mapper_->dateToX(targetDate);

    // Calculate scroll position to center the date in view
    int viewportWidth = view_->viewport()->width();
    int targetScrollValue = static_cast<int>(targetX - viewportWidth / 2.0);

    // Clamp to valid scroll range
    targetScrollValue = qBound(scrollBar_->minimum(),
                               targetScrollValue,
                               scrollBar_->maximum());

    if (animate)
    {
        // Stop any existing animation
        if (animation_->state() == QAbstractAnimation::Running)
        {
            animation_->stop();
        }

        // Setup and start animation
        animation_->setDuration(durationMs);
        animation_->setStartValue(scrollBar_->value());
        animation_->setEndValue(targetScrollValue);
        animation_->start();
    }
    else
    {
        // Jump directly to target
        scrollBar_->setValue(targetScrollValue);
        emit scrollCompleted(targetDate);
    }
}

int TimelineScrollAnimator::scrollValue() const
{
    return scrollBar_->value();
}

void TimelineScrollAnimator::setScrollValue(int value)
{
    scrollBar_->setValue(value);
}

void TimelineScrollAnimator::onAnimationFinished()
{
    emit scrollCompleted(targetDate_);
}
