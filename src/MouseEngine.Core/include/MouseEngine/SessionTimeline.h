#pragma once

#include "ObservationSession.h"
#include "SessionTrace.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace mouse_engine::timeline {

enum class TimelineEventKind {
    SessionStart,
    Packet,
    IdleGap,
    Anomaly,
    SessionEnd
};

struct TimelineEvent {
    double offset_ms{0.0};
    TimelineEventKind kind{TimelineEventKind::Packet};
    std::size_t packet_index{0};
    std::string stream;
    std::string severity;
    std::string type;
    std::string message;
};

struct SessionTimeline {
    bool available{false};
    std::vector<TimelineEvent> events;
};

inline const char* stream_name(unsigned int classes) {
    const bool movement = (classes & mouse_engine::observation::Movement) != 0u;
    const bool button = (classes & mouse_engine::observation::Button) != 0u;
    const bool wheel = (classes & mouse_engine::observation::Wheel) != 0u;
    if (movement && button) return "movement+button";
    if (movement && wheel) return "movement+wheel";
    if (button && wheel) return "button+wheel";
    if (movement) return "movement";
    if (button) return "button";
    if (wheel) return "wheel";
    return "all";
}

inline SessionTimeline build_timeline(
    const mouse_engine::session::SessionTrace& trace,
    const std::vector<mouse_engine::model::ObservationAnomaly>& anomalies,
    double idle_gap_threshold_ms = mouse_engine::model::kDefaultIdleGapThresholdMs) {

    SessionTimeline result;
    const auto replay = mouse_engine::session::build_replay(trace);
    if (!replay.available || !(idle_gap_threshold_ms >= 0.0) || !std::isfinite(idle_gap_threshold_ms)) {
        return result;
    }

    result.events.reserve(replay.events.size() + anomalies.size() + 2);
    result.events.push_back({0.0, TimelineEventKind::SessionStart, 0, "all", {}, "session-start", "Session observation started."});

    double previous_timestamp = trace.packets.front().timestamp_ms;
    for (std::size_t index = 0; index < replay.events.size(); ++index) {
        const auto& event = replay.events[index];
        const double gap = event.packet.timestamp_ms - previous_timestamp;
        if (index > 0 && gap >= idle_gap_threshold_ms) {
            result.events.push_back({
                event.offset_ms,
                TimelineEventKind::IdleGap,
                index,
                "all",
                "info",
                "idle-gap",
                "Observed idle gap reached the session segmentation threshold."
            });
        }

        result.events.push_back({
            event.offset_ms,
            TimelineEventKind::Packet,
            index,
            stream_name(event.packet.classes),
            {},
            "packet",
            "Persisted input packet."
        });
        previous_timestamp = event.packet.timestamp_ms;
    }

    for (const auto& anomaly : anomalies) {
        if (anomaly.packet_index >= trace.packets.size()) continue;
        const double timestamp = trace.packets[anomaly.packet_index].timestamp_ms;
        if (!std::isfinite(timestamp) || timestamp < trace.packets.front().timestamp_ms) continue;
        result.events.push_back({
            timestamp - trace.packets.front().timestamp_ms,
            TimelineEventKind::Anomaly,
            anomaly.packet_index,
            anomaly.stream,
            anomaly.severity,
            anomaly.type,
            anomaly.message
        });
    }

    result.events.push_back({
        replay.events.back().offset_ms,
        TimelineEventKind::SessionEnd,
        replay.events.size() - 1,
        "all",
        {},
        "session-end",
        "Last persisted packet reached."
    });

    std::stable_sort(result.events.begin(), result.events.end(), [](const TimelineEvent& left, const TimelineEvent& right) {
        if (left.offset_ms != right.offset_ms) return left.offset_ms < right.offset_ms;
        if (left.kind == TimelineEventKind::SessionStart) return true;
        if (right.kind == TimelineEventKind::SessionStart) return false;
        if (left.kind == TimelineEventKind::SessionEnd) return false;
        if (right.kind == TimelineEventKind::SessionEnd) return true;
        return static_cast<int>(left.kind) < static_cast<int>(right.kind);
    });

    result.available = true;
    return result;
}

} // namespace mouse_engine::timeline
