#pragma once

#include "ObservationSession.h"

#include <cstddef>
#include <string>

namespace mouse_engine::comparison {

struct SessionComparison {
    std::string left_id;
    std::string right_id;
    std::size_t left_samples{0};
    std::size_t right_samples{0};
    double delta_median_interval_ms{0.0};
    double delta_p95_interval_ms{0.0};
    double delta_jitter_ms{0.0};
    long long delta_idle_gap_count{0};
    long long delta_active_run_count{0};
    long long delta_longest_active_run_packets{0};
    bool evidence_limited{false};
    std::string methodology;
};

inline SessionComparison compare(
    const model::ObservationSession& left,
    const model::ObservationSession& right) {
    SessionComparison result;
    result.left_id = left.id;
    result.right_id = right.id;
    result.left_samples = left.all.timing.interval_count;
    result.right_samples = right.all.timing.interval_count;

    const bool left_has_timing = left.all.timing.interval_count > 0;
    const bool right_has_timing = right.all.timing.interval_count > 0;
    if (!left_has_timing || !right_has_timing) {
        result.evidence_limited = true;
        result.methodology = "insufficient observed timing samples; no winner inference";
        return result;
    }

    result.delta_median_interval_ms =
        right.all.timing.median_interval_ms - left.all.timing.median_interval_ms;
    result.delta_p95_interval_ms =
        right.all.timing.p95_interval_ms - left.all.timing.p95_interval_ms;
    result.delta_jitter_ms =
        right.all.timing.jitter_p95_minus_median_ms -
        left.all.timing.jitter_p95_minus_median_ms;
    result.delta_idle_gap_count =
        static_cast<long long>(right.all.timing.idle_gap_count_50ms) -
        static_cast<long long>(left.all.timing.idle_gap_count_50ms);
    result.delta_active_run_count =
        static_cast<long long>(right.activity.active_run_count) -
        static_cast<long long>(left.activity.active_run_count);
    result.delta_longest_active_run_packets =
        static_cast<long long>(right.activity.longest_active_run_packets) -
        static_cast<long long>(left.activity.longest_active_run_packets);
    result.methodology = "paired session summary; no winner inference";
    return result;
}

} // namespace mouse_engine::comparison
