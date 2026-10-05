#pragma once

#include "ObservationSession.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace mouse_engine::observation {

struct TimedPacket {
    double timestamp_ms{0.0};
    unsigned int classes{0};
};

enum PacketClass : unsigned int {
    Movement = 1u << 0,
    Button = 1u << 1,
    Wheel = 1u << 2
};

struct ActivityRules {
    double idle_gap_threshold_ms{mouse_engine::model::kDefaultIdleGapThresholdMs};
};

struct TimingAnomalyRules {
    std::size_t minimum_samples{32};
    double interval_ratio_threshold{2.0};
};

struct ActiveRun {
    std::size_t packet_count{0};
    double started_at_ms{0.0};
    double ended_at_ms{0.0};
};

inline std::vector<double> sorted_intervals(const std::vector<TimedPacket>& packets) {
    std::vector<double> intervals;
    if (packets.size() < 2) return intervals;
    intervals.reserve(packets.size() - 1);
    for (std::size_t i = 1; i < packets.size(); ++i) {
        const double delta = packets[i].timestamp_ms - packets[i - 1].timestamp_ms;
        if (delta >= 0.0) intervals.push_back(delta);
    }
    std::sort(intervals.begin(), intervals.end());
    return intervals;
}

inline double percentile(const std::vector<double>& sorted, double p) {
    if (sorted.empty()) return 0.0;
    if (p <= 0.0) return sorted.front();
    if (p >= 1.0) return sorted.back();
    const double position = p * static_cast<double>(sorted.size() - 1);
    const auto lower = static_cast<std::size_t>(std::floor(position));
    const auto upper = static_cast<std::size_t>(std::ceil(position));
    if (lower == upper) return sorted[lower];
    const double weight = position - static_cast<double>(lower);
    return sorted[lower] + (sorted[upper] - sorted[lower]) * weight;
}

inline mouse_engine::model::TimingMeasurement summarize_timing(
    const std::vector<TimedPacket>& packets,
    double idle_gap_threshold_ms) {

    mouse_engine::model::TimingMeasurement summary{};
    const auto intervals = sorted_intervals(packets);
    if (intervals.empty()) return summary;

    summary.interval_count = intervals.size();
    summary.min_interval_ms = intervals.front();
    summary.median_interval_ms = percentile(intervals, 0.50);
    summary.p95_interval_ms = percentile(intervals, 0.95);
    summary.max_interval_ms = intervals.back();
    summary.jitter_p95_minus_median_ms =
        (std::max)(0.0, summary.p95_interval_ms - summary.median_interval_ms);

    for (const double interval : intervals) {
        if (interval >= idle_gap_threshold_ms) {
            ++summary.idle_gap_count_50ms;
            summary.longest_idle_gap_ms = (std::max)(summary.longest_idle_gap_ms, interval);
        }
    }
    return summary;
}

inline std::vector<ActiveRun> find_active_runs(
    const std::vector<TimedPacket>& packets,
    const ActivityRules& rules = {}) {

    std::vector<ActiveRun> runs;
    if (packets.empty()) return runs;

    ActiveRun current{1, packets.front().timestamp_ms, packets.front().timestamp_ms};
    for (std::size_t i = 1; i < packets.size(); ++i) {
        const double gap = packets[i].timestamp_ms - packets[i - 1].timestamp_ms;
        if (gap >= 0.0 && gap < rules.idle_gap_threshold_ms) {
            ++current.packet_count;
            current.ended_at_ms = packets[i].timestamp_ms;
        } else {
            runs.push_back(current);
            current = ActiveRun{1, packets[i].timestamp_ms, packets[i].timestamp_ms};
        }
    }
    runs.push_back(current);
    return runs;
}

inline mouse_engine::model::ObservationSession build_session(
    const std::string& session_id,
    const std::string& device_id,
    const std::vector<TimedPacket>& packets,
    const ActivityRules& rules = {}) {

    mouse_engine::model::ObservationSession session;
    session.id = session_id;
    session.device_id = device_id;
    session.activity.idle_gap_threshold_ms = rules.idle_gap_threshold_ms;
    session.all.packet_count = packets.size();
    session.all.timing = summarize_timing(packets, rules.idle_gap_threshold_ms);

    std::vector<TimedPacket> movement;
    std::vector<TimedPacket> button;
    std::vector<TimedPacket> wheel;
    movement.reserve(packets.size());
    button.reserve(packets.size());
    wheel.reserve(packets.size());

    for (const auto& packet : packets) {
        if ((packet.classes & PacketClass::Movement) != 0u) movement.push_back(packet);
        if ((packet.classes & PacketClass::Button) != 0u) button.push_back(packet);
        if ((packet.classes & PacketClass::Wheel) != 0u) wheel.push_back(packet);
    }

    session.movement.packet_count = movement.size();
    session.movement.timing = summarize_timing(movement, rules.idle_gap_threshold_ms);
    session.button.packet_count = button.size();
    session.button.timing = summarize_timing(button, rules.idle_gap_threshold_ms);
    session.wheel.packet_count = wheel.size();
    session.wheel.timing = summarize_timing(wheel, rules.idle_gap_threshold_ms);

    const auto runs = find_active_runs(packets, rules);
    session.activity.active_run_count = runs.size();
    for (const auto& run : runs) {
        session.activity.longest_active_run_packets =
            (std::max)(session.activity.longest_active_run_packets, run.packet_count);
        session.activity.longest_active_run_ms =
            (std::max)(session.activity.longest_active_run_ms, run.ended_at_ms - run.started_at_ms);
    }

    return session;
}

inline std::vector<mouse_engine::model::ObservationAnomaly> detect_timing_irregularities(
    const std::vector<TimedPacket>& packets,
    const TimingAnomalyRules& rules = {}) {

    std::vector<mouse_engine::model::ObservationAnomaly> anomalies;
    const auto intervals = sorted_intervals(packets);
    if (intervals.size() < rules.minimum_samples) return anomalies;

    const double median = percentile(intervals, 0.50);
    if (!(median > 0.0)) return anomalies;

    for (const double interval : intervals) {
        if (interval < rules.interval_ratio_threshold * median) continue;

        mouse_engine::model::ObservationAnomaly anomaly;
        anomaly.id = "timing-irregularity";
        anomaly.severity = "info";
        anomaly.type = "interval-outlier";
        anomaly.message = "Observed interval is materially longer than the session median; this is an observation, not a hardware-failure diagnosis.";
        anomaly.stream = "all";
        anomaly.evidence.source = mouse_engine::model::EvidenceSource::RawInput;
        anomaly.evidence.confidence = mouse_engine::model::EvidenceConfidence::Medium;
        anomaly.evidence.method = "session interval ratio";
        anomaly.evidence.scope = "WM_INPUT arrival inter-arrival";
        anomalies.push_back(std::move(anomaly));
        break;
    }
    return anomalies;
}

} // namespace mouse_engine::observation
