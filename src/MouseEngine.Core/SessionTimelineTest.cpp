#include "MouseEngine/SessionTimeline.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <string>

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
    anomaly.timestamp_ms = 9999.0;

    assert(std::string(stream_name(observation::Movement | observation::Button | observation::Wheel)) == "movement+button+wheel");

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

    ObservationAnomaly invalid_index = anomaly;
    invalid_index.packet_index = trace.packets.size();
    const auto with_invalid_anomaly = build_timeline(trace, {invalid_index}, 50.0);
    assert(with_invalid_anomaly.available);
    assert(with_invalid_anomaly.events.size() == 6);

    SessionTrace invalid = trace;
    invalid.packets[2].timestamp_ms = -1.0;
    const auto invalid_timeline = build_timeline(invalid, {}, 50.0);
    assert(!invalid_timeline.available);
    assert(invalid_timeline.events.empty());

    SessionTrace invalid_schema = trace;
    invalid_schema.schema_version = SessionTrace::kSchemaVersion + 1;
    const auto invalid_schema_timeline = build_timeline(invalid_schema, {}, 50.0);
    assert(!invalid_schema_timeline.available);
    assert(invalid_schema_timeline.events.empty());

    SessionTrace empty;
    empty.session_id = "session-empty";
    empty.device_id = "device-empty";
    const auto empty_timeline = build_timeline(empty, {}, 50.0);
    assert(!empty_timeline.available);
    assert(empty_timeline.events.empty());

    const auto invalid_threshold = build_timeline(trace, {}, -1.0);
    assert(!invalid_threshold.available);
    assert(invalid_threshold.events.empty());

    const auto nonfinite_threshold = build_timeline(trace, {}, std::numeric_limits<double>::quiet_NaN());
    assert(!nonfinite_threshold.available);
    assert(nonfinite_threshold.events.empty());

    return 0;
}
