// LaneAssigner.cpp


#include "LaneAssigner.h"


/**
 * @file LaneAssigner.cpp
 * @brief Implementation file for LaneAssigner utility class
 *
 * This file is intentionally minimal because LaneAssigner is a header-only
 * utility class with static methods. All implementation is in the header
 * for performance reasons (allows inlining).
 *
 * If you need to add non-template, non-inline methods in the future,
 * implement them here.
 */

// Currently no implementation needed - all methods are static and defined in header




#ifdef LANE_ASSIGNER_ENABLE_TESTS
#include <QDebug>
#include <cassert>

/**
 * @brief Unit tests for LaneAssigner
 *
 * Run these tests to verify collision avoidance logic works correctly
 */
void LaneAssigner_runTests() {
    qDebug() << "=== LaneAssigner Unit Tests ===";

    // Test 1: No events
    {
        QVector<LaneAssigner::EventData> events;
        int maxLane = LaneAssigner::assignLanes(events);
        assert(maxLane == 0);
        qDebug() << "✓ Test 1: Empty events list";
    }

    // Test 2: Single event (should be lane 0)
    {
        QVector<LaneAssigner::EventData> events;
        events.append({"E1", QDate(2025, 1, 1), QDate(2025, 1, 5)});
        int maxLane = LaneAssigner::assignLanes(events);
        assert(maxLane == 0);
        assert(events[0].lane == 0);
        qDebug() << "✓ Test 2: Single event assigned to lane 0";
    }

    // Test 3: Two non-overlapping events (both lane 0)
    {
        QVector<LaneAssigner::EventData> events;
        events.append({"E1", QDate(2025, 1, 1), QDate(2025, 1, 5)});
        events.append({"E2", QDate(2025, 1, 6), QDate(2025, 1, 10)});
        int maxLane = LaneAssigner::assignLanes(events);
        assert(maxLane == 0);
        assert(events[0].lane == 0);
        assert(events[1].lane == 0);
        qDebug() << "✓ Test 3: Non-overlapping events in same lane";
    }

    // Test 4: Two overlapping events (different lanes)
    {
        QVector<LaneAssigner::EventData> events;
        events.append({"E1", QDate(2025, 1, 1), QDate(2025, 1, 10)});
        events.append({"E2", QDate(2025, 1, 5), QDate(2025, 1, 15)});
        int maxLane = LaneAssigner::assignLanes(events);
        assert(maxLane == 1);
        assert(events[0].lane == 0);
        assert(events[1].lane == 1); // Must be in different lane
        qDebug() << "✓ Test 4: Overlapping events in different lanes";
    }

    // Test 5: Three overlapping events (three lanes)
    {
        QVector<LaneAssigner::EventData> events;
        events.append({"E1", QDate(2025, 1, 1), QDate(2025, 1, 20)});
        events.append({"E2", QDate(2025, 1, 5), QDate(2025, 1, 15)});
        events.append({"E3", QDate(2025, 1, 10), QDate(2025, 1, 25)});
        int maxLane = LaneAssigner::assignLanes(events);
        assert(maxLane == 2);
        qDebug() << "✓ Test 5: Three overlapping events in three lanes";
    }

    // Test 6: Lane reuse after gap
    {
        QVector<LaneAssigner::EventData> events;
        events.append({"E1", QDate(2025, 1, 1), QDate(2025, 1, 5)});
        events.append({"E2", QDate(2025, 1, 2), QDate(2025, 1, 6)});
        events.append({"E3", QDate(2025, 1, 10), QDate(2025, 1, 15)}); // After gap
        int maxLane = LaneAssigner::assignLanes(events);
        assert(maxLane == 1); // Only needs 2 lanes
        assert(events[2].lane == 0); // E3 should reuse lane 0
        qDebug() << "✓ Test 6: Lane reuse after gap";
    }

    // Test 7: Complex stacking scenario
    {
        QVector<LaneAssigner::EventData> events;
        events.append({"E1", QDate(2025, 1, 1), QDate(2025, 1, 10)});
        events.append({"E2", QDate(2025, 1, 3), QDate(2025, 1, 7)});
        events.append({"E3", QDate(2025, 1, 5), QDate(2025, 1, 12)});
        events.append({"E4", QDate(2025, 1, 11), QDate(2025, 1, 15)});
        int maxLane = LaneAssigner::assignLanes(events);

        // E1: lane 0 (1-10)
        // E2: lane 1 (3-7)
        // E3: lane 2 (5-12)
        // E4: lane 0 (11-15) - reuses lane 0 after E1 ends
        assert(events[0].lane == 0);
        assert(events[1].lane == 1);
        assert(events[2].lane == 2);
        assert(events[3].lane == 0); // Reuses lane 0
        qDebug() << "✓ Test 7: Complex stacking with lane reuse";
    }

    // Test 8: laneToY calculation
    {
        assert(LaneAssigner::laneToY(0) == 0.0);
        assert(LaneAssigner::laneToY(1) == 35.0); // 30 + 5
        assert(LaneAssigner::laneToY(2) == 70.0); // 2 * 35
        qDebug() << "✓ Test 8: laneToY calculation";
    }

    // Test 9: calculateSceneHeight
    {
        double height = LaneAssigner::calculateSceneHeight(0);
        assert(height == 85.0); // (0+1) * 35 + 50

        height = LaneAssigner::calculateSceneHeight(2);
        assert(height == 155.0); // (2+1) * 35 + 50
        qDebug() << "✓ Test 9: calculateSceneHeight";
    }

    qDebug() << "=== All LaneAssigner Tests Passed ✓ ===\n";
}
#endif // LANE_ASSIGNER_ENABLE_TESTS
