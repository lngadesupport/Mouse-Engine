#include "MouseEngine/SessionSerializer.h"

#include <cassert>
#include <string>

int main() {
    mouse_engine::model::ObservationSession session;
    session.id = "session-123";
    session.device_id = "device-1";
    session.started_at_utc = "2026-10-05T15:00:00Z";
    session.ended_at_utc = "2026-10-05T15:00:01Z";
    session.all.packet_count = 2;
    session.all.timing.interval_count = 1;
    session.all.timing.median_interval_ms = 10.0;
    session.all.timing.p95_interval_ms = 10.0;
    session.all.timing.min_interval_ms = 10.0;
    session.all.timing.max_interval_ms = 10.0;

    const std::string json = mouse_engine::session::serialize_json(session);
    assert(json.find(""schemaVersion": 3") != std::string::npos);
    assert(json.find(""id":"session-123"") != std::string::npos);
    assert(json.find(""deviceId":"device-1"") != std::string::npos);
    assert(json.find(""timingScope":"WM_INPUT arrival inter-arrival"") != std::string::npos);
    assert(json.find(""packetCount":2") != std::string::npos);
    return 0;
}
