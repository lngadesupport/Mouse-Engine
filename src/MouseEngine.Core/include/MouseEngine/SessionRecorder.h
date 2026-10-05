#pragma once

#include "ObservationAnalysis.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mouse_engine::session {

class SessionRecorder {
public:
    static constexpr std::size_t kDefaultMaxPackets = 65536;

    explicit SessionRecorder(std::size_t max_packets = kDefaultMaxPackets)
        : max_packets_(max_packets) {}
    bool start(const std::string& device_id, const std::string& started_at_utc) {
        if (recording_) return false;
        device_id_ = device_id;
        started_at_utc_ = started_at_utc;
        packets_.clear();
        session_id_ = make_session_id(device_id_, started_at_utc_);
        recording_ = true;
        return true;
    }

    bool is_recording() const noexcept { return recording_; }
    const std::string& session_id() const noexcept { return session_id_; }
    const std::string& device_id() const noexcept { return device_id_; }
    std::size_t packet_count() const noexcept { return packets_.size(); }

    bool record(const observation::TimedPacket& packet) {
        if (!recording_) return false;
        if (packets_.size() >= max_packets_) return false;
        packets_.push_back(packet);
        return true;
    }

    model::ObservationSession snapshot(const std::string& ended_at_utc) const {
        model::ObservationSession session;
        if (!recording_) return session;
        session = observation::build_session(session_id_, device_id_, packets_);
        session.started_at_utc = started_at_utc_;
        session.ended_at_utc = ended_at_utc;
        return session;
    }

    void finish() noexcept {
        recording_ = false;
        packets_.clear();
        device_id_.clear();
        started_at_utc_.clear();
        session_id_.clear();
    }

    model::ObservationSession stop(const std::string& ended_at_utc) {
        const auto session = snapshot(ended_at_utc);
        if (session.id.empty()) return session;
        finish();
        return session;
    }

private:
    static std::string make_session_id(const std::string& device_id, const std::string& started_at_utc) {
        const std::string input = device_id + "|" + started_at_utc;
        std::uint64_t hash = 1469598103934665603ull;
        for (const unsigned char byte : input) {
            hash ^= byte;
            hash *= 1099511628211ull;
        }
        return "session-" + std::to_string(hash);
    }

    bool recording_{false};
    std::string session_id_;
    std::string device_id_;
    std::string started_at_utc_;
    std::vector<observation::TimedPacket> packets_;
    std::size_t max_packets_{kDefaultMaxPackets};
};

} // namespace mouse_engine::session
