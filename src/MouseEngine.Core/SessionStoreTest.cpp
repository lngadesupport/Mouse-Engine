#include "MouseEngine/SessionStore.h"
#include "MouseEngine/SessionTraceStore.h"

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
    session.all.distribution.sample_count = 119;
    session.all.distribution.mean_interval_ms = 0.993;
    session.all.distribution.bucket_width_ms = 0.050;
    session.all.distribution.buckets.push_back({0.950, 1.000, 80, 0.672268907563});
    session.all.distribution.buckets.push_back({1.000, 1.050, 39, 1.0});
    session.all.timing.idle_gap_count_50ms = 3;
    session.activity.active_run_count = 7;
    session.activity.longest_active_run_packets = 54;
    session.anomalies.push_back({"gap-1","warning","timing-gap","Observed idle gap above threshold","all"});

    std::string error;
    assert(store.save(session, &error));
    assert(error.empty());

    mouse_engine::session::SessionTrace trace;
    trace.session_id = session.id;
    trace.device_id = session.device_id;
    trace.packets.push_back({0.0, mouse_engine::observation::Movement, 3, -2, 0, 0});
    trace.packets.push_back({1.0, mouse_engine::observation::Button, 0, 0, 1, 0});
    assert(mouse_engine::session::SessionTraceStore(
        mouse_engine::workspace::WorkspacePaths::from_root(root)).save(trace, &error));
    assert(error.empty());

    const auto path = store.path_for(session.id);
    assert(path.filename() == "session_unsafe.json");
    assert(fs::is_regular_file(path));

    std::ifstream input(path, std::ios::binary);
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    assert(json.find("\"schemaVersion\":3") != std::string::npos);
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
    assert(summaries.front().distribution_sample_count == 119);
    assert(summaries.front().distribution_mean_interval_ms == 0.993);
    assert(summaries.front().distribution_buckets.size() == 2);
    assert(summaries.front().distribution_buckets[0].count == 80);
    assert(summaries.front().distribution_buckets[1].cumulative_fraction == 1.0);
    assert(summaries.front().anomalies.size() == 1);
    assert(summaries.front().anomalies.front().id == "gap-1");
    assert(summaries.front().anomalies.front().severity == "warning");
    assert(summaries.front().anomalies.front().type == "timing-gap");
    assert(summaries.front().anomalies.front().stream == "all");
    assert(summaries.front().trace_available);
    assert(summaries.front().trace_packet_count == 2);

    fs::remove_all(root, ec);
    return 0;
}
