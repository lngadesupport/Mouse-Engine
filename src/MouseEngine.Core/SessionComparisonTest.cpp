#include "MouseEngine/SessionComparison.h"

#include <cassert>
#include <cmath>

int main() {
    using namespace mouse_engine::comparison;
    mouse_engine::model::ObservationSession a;
    mouse_engine::model::ObservationSession b;

    a.id = "A";
    b.id = "B";
    a.all.timing.interval_count = 100;
    a.all.timing.median_interval_ms = 1.00;
    a.all.timing.p95_interval_ms = 1.20;
    a.all.timing.jitter_p95_minus_median_ms = 0.20;
    a.all.timing.idle_gap_count_50ms = 4;
    a.activity.active_run_count = 10;
    a.activity.longest_active_run_packets = 80;

    b.all.timing.interval_count = 120;
    b.all.timing.median_interval_ms = 0.90;
    b.all.timing.p95_interval_ms = 1.10;
    b.all.timing.jitter_p95_minus_median_ms = 0.20;
    b.all.timing.idle_gap_count_50ms = 7;
    b.activity.active_run_count = 8;
    b.activity.longest_active_run_packets = 95;

    const auto result = compare(a, b);
    assert(result.left_id == "A");
    assert(result.right_id == "B");
    assert(std::abs(result.delta_median_interval_ms + 0.10) < 1e-9);
    assert(std::abs(result.delta_p95_interval_ms + 0.10) < 1e-9);
    assert(std::abs(result.delta_jitter_ms) < 1e-9);
    assert(result.delta_idle_gap_count == 3);
    assert(result.delta_active_run_count == -2);
    assert(result.delta_longest_active_run_packets == 15);
    assert(result.methodology == "paired session summary; no winner inference");

    mouse_engine::model::ObservationSession empty;
    const auto empty_result = compare(a, empty);
    assert(empty_result.evidence_limited);
    assert(empty_result.methodology.find("insufficient") != std::string::npos);
    return 0;
}
