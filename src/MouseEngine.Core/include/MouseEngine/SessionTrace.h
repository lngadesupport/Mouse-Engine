#pragma once

#include "ObservationAnalysis.h"

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace mouse_engine::session {

struct TracePacket {
    double timestamp_ms{0.0};
    unsigned int classes{0};
    std::int32_t dx{0};
    std::int32_t dy{0};
    std::uint32_t buttons{0};
    std::int32_t wheel{0};
};

struct SessionTrace {
    static constexpr int kSchemaVersion = 1;
    static constexpr std::size_t kMaxPackets = 65536;

    int schema_version{kSchemaVersion};
    std::string session_id;
    std::string device_id;
    std::vector<TracePacket> packets;
    bool truncated{false};
};

class SessionTraceRecorder {
public:
    explicit SessionTraceRecorder(std::size_t max_packets = SessionTrace::kMaxPackets)
        : max_packets_(max_packets) {}

    bool start(const std::string& session_id, const std::string& device_id) {
        if (recording_) return false;
        session_id_ = session_id;
        device_id_ = device_id;
        packets_.clear();
        truncated_ = false;
        recording_ = true;
        return true;
    }

    bool is_recording() const noexcept { return recording_; }

    bool record(const TracePacket& packet) {
        if (!recording_) return false;
        if (packets_.size() >= max_packets_) {
            truncated_ = true;
            return false;
        }
        packets_.push_back(packet);
        return true;
    }

    SessionTrace snapshot() const {
        SessionTrace trace;
        if (!recording_) return trace;
        trace.session_id = session_id_;
        trace.device_id = device_id_;
        trace.packets = packets_;
        trace.truncated = truncated_;
        return trace;
    }

    void finish() noexcept {
        recording_ = false;
        session_id_.clear();
        device_id_.clear();
        packets_.clear();
        truncated_ = false;
    }

private:
    bool recording_{false};
    std::string session_id_;
    std::string device_id_;
    std::vector<TracePacket> packets_;
    std::size_t max_packets_{65536};
    bool truncated_{false};
};

struct ReplayEvent {
    double offset_ms{0.0};
    TracePacket packet{};
};

struct ReplayResult {
    bool available{false};
    bool monotonic{true};
    std::vector<ReplayEvent> events;
};

inline ReplayResult build_replay(const SessionTrace& trace) {
    ReplayResult result;
    if (trace.schema_version != SessionTrace::kSchemaVersion ||
        trace.session_id.empty() ||
        trace.device_id.empty() ||
        trace.packets.empty()) {
        return result;
    }

    const double origin = trace.packets.front().timestamp_ms;
    if (!std::isfinite(origin) || origin < 0.0) return result;

    result.events.reserve(trace.packets.size());
    double previous = origin;
    for (const auto& packet : trace.packets) {
        if (!std::isfinite(packet.timestamp_ms) || packet.timestamp_ms < 0.0 ||
            packet.timestamp_ms < previous) {
            result.monotonic = false;
            result.events.clear();
            return result;
        }
        result.events.push_back({packet.timestamp_ms - origin, packet});
        previous = packet.timestamp_ms;
    }

    result.available = true;
    return result;
}

} // namespace mouse_engine::session
