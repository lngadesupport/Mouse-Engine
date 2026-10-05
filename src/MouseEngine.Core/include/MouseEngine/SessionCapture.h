#pragma once

#include "SessionRecorder.h"
#include "SessionStore.h"
#include "SessionTraceStore.h"

#include <cstddef>
#include <string>
#include <utility>

namespace mouse_engine::session {

class SessionCapture {
public:
    explicit SessionCapture(SessionStore store, bool trace_enabled = false)
        : store_(std::move(store)),
          trace_store_(store_.paths()),
          trace_enabled_(trace_enabled) {}

    bool start(const std::string& device_id, const std::string& started_at_utc) {
        if (!recorder_.start(device_id, started_at_utc)) return false;
        if (trace_enabled_ &&
            !trace_recorder_.start(recorder_.session_id(), device_id)) {
            recorder_.finish();
            return false;
        }
        return true;
    }

    bool is_recording() const noexcept { return recorder_.is_recording(); }
    const std::string& session_id() const noexcept { return recorder_.session_id(); }
    const std::string& device_id() const noexcept { return recorder_.device_id(); }
    std::size_t packet_count() const noexcept { return recorder_.packet_count(); }
    bool trace_enabled() const noexcept { return trace_enabled_; }
    std::size_t trace_packet_count() const noexcept {
        return trace_recorder_.snapshot().packets.size();
    }

    model::ObservationSession snapshot(const std::string& observed_at_utc = {}) const {
        return recorder_.snapshot(observed_at_utc);
    }

    bool record(const observation::TimedPacket& packet) {
        return recorder_.record(packet);
    }

    bool record(
        const observation::TimedPacket& packet,
        const TracePacket& trace_packet) {
        if (!recorder_.record(packet)) return false;
        if (trace_enabled_ && !trace_recorder_.record(trace_packet)) {
            return false;
        }
        return true;
    }

    bool stop(
        const std::string& ended_at_utc,
        model::ObservationSession* saved_session = nullptr,
        std::string* error = nullptr) {

        if (!recorder_.is_recording()) return false;

        model::ObservationSession session = recorder_.snapshot(ended_at_utc);
        if (!store_.save(session, error)) return false;

        if (trace_enabled_) {
            const SessionTrace trace = trace_recorder_.snapshot();
            if (!trace_store_.save(trace, error)) return false;
        }

        recorder_.finish();
        trace_recorder_.finish();
        if (saved_session) *saved_session = std::move(session);
        return true;
    }

private:
    SessionStore store_;
    SessionTraceStore trace_store_;
    SessionRecorder recorder_;
    SessionTraceRecorder trace_recorder_;
    bool trace_enabled_{false};
};

} // namespace mouse_engine::session
