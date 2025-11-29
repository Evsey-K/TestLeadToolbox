// LaneAssigner.h


#pragma once
#include <QDate>
#include <QVector>
#include <QMap>
#include <algorithm>


/**
 * @class LaneAssigner
 * @brief Implements collision avoidance algorithm for timeline events
 *
 * This utility class assigns vertical "lanes" to timeline events to prevent
 * visual overlap when events have overlapping date ranges. The algorithm:
 * 1. Sorts events by start date
 * 2. For each event, finds the lowest available lane
 * 3. Tracks when each lane becomes free
 * 4. Assigns events to lanes ensuring no date-range overlaps in same lane
 *
 * Usage:
 * @code
 *   LaneAssigner assigner;
 *   assigner.assignLanes(events);
 *   // events now have .lane field populated
 * @endcode
 */
class LaneAssigner
{
public:
    /**
     * @brief Event data structure for lane assignment
     */
    struct EventData
    {
        QString id;
        QDate startDate;
        QDate endDate;
        int lane = 0;               ///< Assigned lane (0 = top)
        void* userData = nullptr;   ///< Optional pointer to original event object

        EventData() = default;
        EventData(const QString& id_, const QDate& start, const QDate& end, void* user = nullptr)
            : id(id_)
            , startDate(start)
            , endDate(end)
            , userData(user)
        {}
    };

    /**
     * @brief Assigns lanes to all events to prevent visual overlap
     * @param events Vector of events to assign lanes (modified in place)
     * @return Maximum lane number used (useful for calculating scene height)
     */
    static int assignLanes(QVector<EventData>& events)
    {
        if (events.isEmpty())
        {
            return 0;
        }

        // Step 1: Sort events by start date (earlier dates first)
        std::sort(events.begin(), events.end(),
                  [] (const EventData& a, const EventData& b)
                  {
                      if (a.startDate == b.startDate)
                      {
                          // If same start date, longer events go first
                          return a.endDate > b.endDate;
                      }

                      return a.startDate < b.startDate;
                  });


        // Step 2: Track when each lane becomes available
        // Key = lane number, Value = date when lane becomes free
        QMap<int, QDate> laneEndDates;

        int maxLane = 0;

        // Step 3: Assign each event to the lowest available lane
        for (EventData& event : events)
        {
            int assignedLane = findAvailableLane(laneEndDates, event.startDate);
            event.lane = assignedLane;

            // Mark this lane as occupied
            laneEndDates[assignedLane] = event.endDate;

            if (assignedLane > maxLane)
            {
                maxLane = assignedLane
            }
        }

        return maxLane;
    }


    /**
     * @brief Calculates the vertical Y position for a given lane
     * @param lane Lane number (0-based)
     * @param laneHeight Height of each lane in pixels
     * @param laneSpacing Vertical spacing between lanes
     * @return Y coordinate for the lane
     */
    static double laneToY(int lane, double laneHeight = 30.0, double laneSpacing = 5.0)
    {
        return lane * (laneHeight + laneSpacing);
    }

    /**
     * @brief Calculates total scene height needed for given lane count
     * @param maxLane Highest lane number used (0-based)
     * @param laneHeight Height of each lane in pixels
     * @param laneSpacing Vertical spacing between lanes
     * @return Total height needed for the scene
     */
    double calculateSceneHeight(int maxLane, double laneHeight = 30.0, double laneSpacing = 5.0)
    {
        return (maxLane + 1) * (laneHeight + laneSpacing) + 50;     // +50 for padding
    }

private:
    /**
     * @brief Finds the lowest available lane for an event starting on given date
     * @param laneEndDates Map tracking when each lane becomes free
     * @param eventStartDate Start date of event to place
     * @return Lane number (0-based) that is available
     */
    static int findAvailableLane(const QMap<int, QDate>& laneEndDates, const QDate& eventStartDate)
    {
        int lane = 0;

        // Keep checking lanes until we find one that's free
        while(laneEndDates.contains(lane))
        {
            QDate laneEndDate = laneEndDates[lane];

            // Lane is available if it ends before our event starts
            if (laneEndDate < eventStartDate)
            {
                return lane;
            }

            // This lane is occupied, try next one
            lane++;
        }

        // No existing lane is available, return new lane
        return lane;
    }
};


