#pragma once

#include "SessionRecorder.h"
#include "SessionStore.h"

#include <cstddef>
#include <string>
#include <utility>

namespace mouse_engine::session {

class SessionCapture {
public:
    explicit SessionCapture(SessionStore store)
        : store_(std::move(store)) {}

    bool start(const std::string& device_id, const std::string& started_at_utc) {
        return recorder_.start(device_id, started_at_utc);
    }

    bool is_recording() const noexcept {
        return recorder_.is_recording();
    }

    const std::string& session_id() const noexcept {
        return recorder_.session_id();
    }

    const std::string& device_id() const noexcept {
        return recorder_.device_id();
    }

    std::size_t packet_count() const noexcept {
        return recorder_.packet_count();
    }

    model::ObservationSession snapshot(const std::string& observed_at_utc = {}) const {
        return recorder_.snapshot(observed_at_utc);
    }

    bool record(const observation::TimedPacket& packet) {
        return recorder_.record(packet);
    }

    bool stop(
        const std::string& ended_at_utc,
        model::ObservationSession* saved_session = nullptr,
        std::string* error = nullptr) {

        if (!recorder_.is_recording()) {
            return false;
        }

        model::ObservationSession session = recorder_.snapshot(ended_at_utc);
        if (!store_.save(session, error)) {
            return false;
        }

        recorder_.finish();
        if (saved_session) *saved_session = std::move(session);
        return true;
    }

private:
    SessionRecorder recorder_;
    SessionStore store_;
};

} // namespace mouse_engine::session
