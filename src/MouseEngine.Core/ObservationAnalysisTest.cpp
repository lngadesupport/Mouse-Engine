#include "MouseEngine/ObservationAnalysis.h"

#include <cassert>
#include <cmath>
#include <vector>

int main() {
    using namespace mouse_engine::observation;

    const std::vector<TimedPacket> packets{
        {0.0, Movement},
        {10.0, Movement},
        {20.0, Movement | Button},
        {30.0, Button},
        {100.0, Wheel},
        {110.0, Wheel},
    };

    const auto timing = summarize_timing(packets, 50.0);
    assert(timing.interval_count == 5);
    assert(std::abs(timing.median_interval_ms - 10.0) < 1e-9);
    assert(timing.idle_gap_count_50ms == 1);
    assert(std::abs(timing.longest_idle_gap_ms - 70.0) < 1e-9);

    const auto runs = find_active_runs(packets);
    assert(runs.size() == 2);
    assert(runs[0].packet_count == 4);
    assert(runs[1].packet_count == 2);

    const auto session = build_session("session-1", "device-1", packets);
    assert(session.id == "session-1");
    assert(session.all.packet_count == 6);
    assert(session.movement.packet_count == 3);
    assert(session.button.packet_count == 2);
    assert(session.wheel.packet_count == 2);
    assert(session.activity.active_run_count == 2);
    assert(session.activity.longest_active_run_packets == 4);
    assert(session.activity.longest_active_run_ms == 30.0);
    assert(mouse_engine::model::observation_session_is_valid(session));

    std::vector<TimedPacket> irregular;
    for (int i = 0; i < 32; ++i) {
        irregular.push_back({static_cast<double>(i * 10), Movement});
    }
    irregular.push_back({5000.0, Movement});
    const auto anomalies = detect_timing_irregularities(irregular);
    assert(anomalies.size() == 1);
    assert(anomalies.front().severity == "info");
    assert(anomalies.front().evidence.source == mouse_engine::model::EvidenceSource::RawInput);

    return 0;
}
