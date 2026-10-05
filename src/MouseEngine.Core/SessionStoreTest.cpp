#include "MouseEngine/SessionStore.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    namespace fs = std::filesystem;

    const fs::path root = fs::temp_directory_path() / "Mouse Engine Session Store Test";
    std::error_code ec;
    fs::remove_all(root, ec);

    mouse_engine::session::SessionStore store(
        mouse_engine::workspace::WorkspacePaths::from_root(root));

    mouse_engine::model::ObservationSession session;
    session.id = "session/unsafe";
    session.device_id = "device-1";
    session.started_at_utc = "2026-10-05T15:00:00Z";
    session.ended_at_utc = "2026-10-05T15:00:01Z";
    session.all.packet_count = 120;
    session.all.timing.interval_count = 119;
    session.all.timing.median_interval_ms = 0.982;
    session.all.timing.p95_interval_ms = 1.104;
    session.all.timing.jitter_p95_minus_median_ms = 0.122;
    session.all.timing.idle_gap_count_50ms = 3;
    session.activity.active_run_count = 7;
    session.activity.longest_active_run_packets = 54;

    std::string error;
    assert(store.save(session, &error));
    assert(error.empty());

    const auto path = store.path_for(session.id);
    assert(path.filename() == "session_unsafe.json");
    assert(fs::is_regular_file(path));

    std::ifstream input(path, std::ios::binary);
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    assert(json.find(""schemaVersion":3") != std::string::npos);
    assert(json.find("\"id\":\"session/unsafe\"") != std::string::npos);

    auto summaries = store.list(&error);
    assert(error.empty());
    assert(summaries.size() == 1);
    assert(summaries.front().id == session.id);
    assert(summaries.front().device_id == "device-1");
    assert(summaries.front().started_at_utc == "2026-10-05T15:00:00Z");
    assert(summaries.front().ended_at_utc == "2026-10-05T15:00:01Z");
    assert(summaries.front().complete);
    assert(summaries.front().packet_count == 120);
    assert(summaries.front().interval_count == 119);
    assert(summaries.front().median_interval_ms == 0.982);
    assert(summaries.front().p95_interval_ms == 1.104);
    assert(summaries.front().jitter_p95_minus_median_ms == 0.122);
    assert(summaries.front().idle_gap_count_50ms == 3);
    assert(summaries.front().active_run_count == 7);
    assert(summaries.front().longest_active_run_packets == 54);

    fs::remove_all(root, ec);
    return 0;
}
