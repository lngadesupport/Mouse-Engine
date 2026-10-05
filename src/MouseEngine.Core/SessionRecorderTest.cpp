#include "MouseEngine/SessionRecorder.h"

#include <cassert>

int main() {
    using namespace mouse_engine::session;
    SessionRecorder recorder;

    assert(!recorder.is_recording());
    assert(recorder.start("device-1", "2026-10-05T15:00:00Z"));
    assert(recorder.is_recording());
    assert(!recorder.session_id().empty());
    assert(!recorder.start("device-1", "2026-10-05T15:00:01Z"));

    assert(recorder.record({0.0, mouse_engine::observation::Movement}));
    assert(recorder.record({10.0, mouse_engine::observation::Movement}));
    assert(recorder.record({20.0, mouse_engine::observation::Button}));

    const auto session = recorder.stop("2026-10-05T15:00:01Z");
    assert(!recorder.is_recording());
    assert(session.device_id == "device-1");
    assert(session.started_at_utc == "2026-10-05T15:00:00Z");
    assert(session.ended_at_utc == "2026-10-05T15:00:01Z");
    assert(session.all.packet_count == 3);
    assert(session.movement.packet_count == 2);
    assert(session.button.packet_count == 1);
    assert(mouse_engine::model::observation_session_is_valid(session));
    assert(!recorder.record({30.0, mouse_engine::observation::Movement}));
    return 0;
}
