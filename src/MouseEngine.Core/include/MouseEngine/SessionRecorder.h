#pragma once

#include "ObservationAnalysis.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace mouse_engine::session {

class SessionRecorder {
public:
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

    bool record(const observation::TimedPacket& packet) {
        if (!recording_) return false;
        packets_.push_back(packet);
        return true;
    }

    model::ObservationSession stop(const std::string& ended_at_utc) {
        model::ObservationSession session;
        if (!recording_) return session;
        session = observation::build_session(session_id_, device_id_, packets_);
        session.started_at_utc = started_at_utc_;
        session.ended_at_utc = ended_at_utc;
        recording_ = false;
        packets_.clear();
        device_id_.clear();
        started_at_utc_.clear();
        session_id_.clear();
        return session;
    }

private:
    static std::string make_session_id(const std::string& device_id, const std::string& started_at_utc) {
        return "session-" + std::to_string(std::hash<std::string>{}(device_id + "|" + started_at_utc));
    }

    bool recording_{false};
    std::string session_id_;
    std::string device_id_;
    std::string started_at_utc_;
    std::vector<observation::TimedPacket> packets_;
};

} // namespace mouse_engine::session
