#include "MouseEngine/SessionTrace.h"

#include <cassert>
#include <limits>

int main() {
    using namespace mouse_engine::session;

    SessionTraceRecorder recorder(3);
    assert(recorder.start("session-1", "device-1"));
    assert(recorder.record({0.0, observation::Movement, 4, -2, 0, 0}));
    assert(recorder.record({1.5, observation::Button, 0, 0, 1, 0}));
    assert(recorder.record({3.0, observation::Wheel, 0, 0, 0, 1}));
    assert(!recorder.record({4.0, observation::Movement, 1, 1, 0, 0}));

    const auto trace = recorder.snapshot();
    assert(trace.schema_version == 1);
    assert(trace.session_id == "session-1");
    assert(trace.device_id == "device-1");
    assert(trace.packets.size() == 3);
    assert(trace.truncated);

    const auto replay = build_replay(trace);
    assert(replay.available);
    assert(replay.events.size() == 3);
    assert(replay.events[0].offset_ms == 0.0);
    assert(replay.events[1].offset_ms == 1.5);
    assert(replay.events[2].offset_ms == 3.0);
    assert(replay.events[1].packet.classes == observation::Button);
    assert(replay.events[0].packet.dx == 4);
    assert(replay.events[0].packet.dy == -2);

    SessionTrace invalid = trace;
    invalid.packets[1].timestamp_ms = -1.0;
    const auto invalid_replay = build_replay(invalid);
    assert(!invalid_replay.available);
    assert(invalid_replay.events.empty());

    SessionTrace nan_trace = trace;
    nan_trace.packets[1].timestamp_ms = std::numeric_limits<double>::quiet_NaN();
    const auto nan_replay = build_replay(nan_trace);
    assert(!nan_replay.available);
    assert(nan_replay.events.empty());

    recorder.finish();
    assert(!recorder.is_recording());
    return 0;
}
