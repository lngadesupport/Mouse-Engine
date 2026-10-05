#include "MouseEngine/SessionTraceStore.h"

#include <cassert>
#include <filesystem>
#include <string>
#include <limits>

int main() {
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / "Mouse Engine Session Trace Store Test";
    std::error_code ec;
    fs::remove_all(root, ec);

    mouse_engine::session::SessionTraceStore store(
        mouse_engine::workspace::WorkspacePaths::from_root(root));

    mouse_engine::session::SessionTrace trace;
    trace.session_id = "session/trace-1";
    trace.device_id = "device-1";
    trace.truncated = true;
    trace.packets.push_back({0.0, mouse_engine::observation::Movement, 2, -1, 0, 0});
    trace.packets.push_back({1.25, mouse_engine::observation::Button, 0, 0, 1, 0});

    std::string error;
    assert(store.save(trace, &error));
    assert(error.empty());
    assert(fs::is_regular_file(store.path_for(trace.session_id)));
    assert(store.exists(trace.session_id));

    mouse_engine::session::SessionTrace loaded;
    assert(store.load(trace.session_id, &loaded, &error));
    assert(error.empty());
    assert(loaded.schema_version == 1);
    assert(loaded.session_id == trace.session_id);
    assert(loaded.device_id == trace.device_id);
    assert(loaded.truncated);
    assert(loaded.packets.size() == 2);
    assert(loaded.packets[0].dx == 2);
    assert(loaded.packets[0].dy == -1);
    assert(loaded.packets[1].timestamp_ms == 1.25);
    assert(loaded.packets[1].classes == mouse_engine::observation::Button);

    {
        std::ofstream corrupt(store.path_for(trace.session_id), std::ios::binary | std::ios::trunc);
        corrupt << "{\"schemaVersion\":1,\"sessionId\":\"session/trace-1\",\"deviceId\":\"device-1\",\"packetCount\":2,\"truncated\":false}\n";
        corrupt << "not-a-packet\n";
    }
    error.clear();
    assert(!store.load(trace.session_id, &loaded, &error));
    assert(!error.empty());

    mouse_engine::session::SessionTrace invalid_timestamp = trace;
    invalid_timestamp.session_id = "session-invalid-timestamp";
    invalid_timestamp.packets[1].timestamp_ms = -1.0;
    error.clear();
    assert(!store.save(invalid_timestamp, &error));
    assert(!error.empty());

    mouse_engine::session::SessionTrace nan_timestamp = trace;
    nan_timestamp.session_id = "session-nan-timestamp";
    nan_timestamp.packets[1].timestamp_ms = std::numeric_limits<double>::quiet_NaN();
    error.clear();
    assert(!store.save(nan_timestamp, &error));
    assert(!error.empty());

    mouse_engine::session::SessionTrace empty_trace;
    empty_trace.session_id = "session-empty";
    empty_trace.device_id = "device-1";
    error.clear();
    assert(store.save(empty_trace, &error));
    assert(error.empty());

    fs::remove_all(root, ec);
    return 0;
}
