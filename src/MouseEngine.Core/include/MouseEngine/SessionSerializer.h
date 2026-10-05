#pragma once

#include "ObservationSession.h"

#include <iomanip>
#include <sstream>
#include <string>

namespace mouse_engine::session {

inline std::string json_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char ch : value) {
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += ch; break;
        }
    }
    return out;
}

inline void append_timing(std::ostringstream& out, const model::TimingMeasurement& t) {
    out << "{"
        << "\"intervalCount\":" << t.interval_count
        << ",\"minIntervalMs\":" << std::setprecision(15) << t.min_interval_ms
        << ",\"medianIntervalMs\":" << t.median_interval_ms
        << ",\"p95IntervalMs\":" << t.p95_interval_ms
        << ",\"maxIntervalMs\":" << t.max_interval_ms
        << ",\"jitterP95MinusMedianMs\":" << t.jitter_p95_minus_median_ms
        << ",\"idleGapCount50ms\":" << t.idle_gap_count_50ms
        << ",\"longestIdleGapMs\":" << t.longest_idle_gap_ms
        << "}";
}

inline void append_distribution(std::ostringstream& out, const model::TimingDistribution& distribution) {
    out << "{\"sampleCount\":" << distribution.sample_count
        << ",\"meanIntervalMs\":" << std::setprecision(15) << distribution.mean_interval_ms
        << ",\"bucketWidthMs\":" << distribution.bucket_width_ms
        << ",\"buckets\":[";
    for (std::size_t i = 0; i < distribution.buckets.size(); ++i) {
        if (i != 0) out << ",";
        const auto& bucket = distribution.buckets[i];
        out << "{\"lowerBoundMs\":" << bucket.lower_bound_ms
            << ",\"upperBoundMs\":" << bucket.upper_bound_ms
            << ",\"count\":" << bucket.count
            << ",\"cumulativeFraction\":" << bucket.cumulative_fraction << "}";
    }
    out << "]}";
}

inline void append_stream(std::ostringstream& out, const model::ObservationStream& stream) {
    out << "{\"packetCount\":" << stream.packet_count << ",\"timing\":";
    append_timing(out, stream.timing);
    out << ",\"distribution\":";
    append_distribution(out, stream.distribution);
    out << "}";
}

inline std::string serialize_json(const model::ObservationSession& session) {
    std::ostringstream out;
    out << "{"
        << "\"schemaVersion\":" << model::ObservationSession::kSchemaVersion
        << ",\"id\":\"" << json_escape(session.id) << "\""
        << ",\"deviceId\":\"" << json_escape(session.device_id) << "\""
        << ",\"startedAtUtc\":\"" << json_escape(session.started_at_utc) << "\""
        << ",\"endedAtUtc\":\"" << json_escape(session.ended_at_utc) << "\""
        << ",\"timingScope\":\"" << json_escape(session.timing_scope) << "\""
        << ",\"streams\":{\"all\":";
    append_stream(out, session.all);
    out << ",\"movement\":";
    append_stream(out, session.movement);
    out << ",\"button\":";
    append_stream(out, session.button);
    out << ",\"wheel\":";
    append_stream(out, session.wheel);
    out << "}"
        << ",\"activity\":{\"activeRunCount\":" << session.activity.active_run_count
        << ",\"longestActiveRunPackets\":" << session.activity.longest_active_run_packets
        << ",\"longestActiveRunMs\":" << session.activity.longest_active_run_ms
        << ",\"idleGapThresholdMs\":" << session.activity.idle_gap_threshold_ms
        << "}"
        << ",\"anomalies\":[";
    for (std::size_t i = 0; i < session.anomalies.size(); ++i) {
        if (i != 0) out << ",";
        const auto& a = session.anomalies[i];
        out << "{\"id\":\"" << json_escape(a.id)
            << "\",\"severity\":\"" << json_escape(a.severity)
            << "\",\"type\":\"" << json_escape(a.type)
            << "\",\"message\":\"" << json_escape(a.message)
            << "\",\"stream\":\"" << json_escape(a.stream)
            << "\",\"packetIndex\":" << a.packet_index
            << ",\"timestampMs\":" << a.timestamp_ms << "}";
    }
    out << "]}";
    return out.str();
}

} // namespace mouse_engine::session
