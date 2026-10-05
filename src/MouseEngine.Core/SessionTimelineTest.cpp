#include "MouseEngine/SessionTimeline.h"

#include <cassert>
#include <cmath>
#include <vector>
#include <limits>

int main() {
    using namespace mouse_engine::session;
    using namespace mouse_engine::model;
    using namespace mouse_engine::timeline;

    SessionTrace trace;
    trace.session_id = "session-1";
    trace.device_id = "device-1";
    trace.packets = {
        {0.0, 1u, 4, 0, 0, 0},
        {1.0, 1u, 1, 0, 0, 0},
        {60.0, 2u, 0, 0, 1, 0},
        {61.0, 4u, 0, 0, 0, 1},
    };

    ObservationAnomaly anomaly;
    anomaly.id = "timing-irregularity";
    anomaly.severity = "info";
    anomaly.type = "interval-outlier";
    anomaly.message = "Observed interval is materially longer than the session median.";
    anomaly.stream = "all";
    anomaly.packet_index = 2;
    anomaly.timestamp_ms = 60.0;

    const auto timeline = build_timeline(trace, {anomaly}, 50.0);
    assert(timeline.available);
    assert(timeline.events.size() == 8);

    assert(timeline.events[0].kind == TimelineEventKind::SessionStart);
    assert(timeline.events[1].kind == TimelineEventKind::Packet);
    assert(timeline.events[1].packet_index == 0);
    assert(timeline.events[2].kind == TimelineEventKind::Packet);
    assert(timeline.events[3].kind == TimelineEventKind::Packet);
    assert(timeline.events[4].kind == TimelineEventKind::IdleGap);
    assert(std::abs(timeline.events[4].offset_ms - 60.0) < 1e-9);
    assert(timeline.events[5].kind == TimelineEventKind::Anomaly);
    assert(timeline.events[5].packet_index == 2);
    assert(std::abs(timeline.events[5].offset_ms - 60.0) < 1e-9);
    assert(timeline.events[6].kind == TimelineEventKind::Packet);
    assert(timeline.events[7].kind == TimelineEventKind::SessionEnd);
    assert(timeline.events[6].packet_index == 3);

    SessionTrace invalid = trace;
    invalid.packets[2].timestamp_ms = -1.0;
    const auto invalid_timeline = build_timeline(invalid, {}, 50.0);
    assert(!invalid_timeline.available);
    assert(invalid_timeline.events.empty());

    const auto invalid_threshold = build_timeline(trace, {}, -1.0);
    assert(!invalid_threshold.available);
    assert(invalid_threshold.events.empty());

    const auto nonfinite_threshold = build_timeline(trace, {}, std::numeric_limits<double>::quiet_NaN());
    assert(!nonfinite_threshold.available);
    assert(nonfinite_threshold.events.empty());

    return 0;
}
