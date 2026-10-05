#pragma once

#include "ObservationAnalysis.h"
#include "ObservationDistribution.h"
#include "ObservationSession.h"

#include <cstddef>
#include <vector>

namespace mouse_engine::observation {

inline model::TimingMeasurement timing_measurement_from_distribution(
    const IntervalDistribution& distribution,
    double idle_gap_threshold_ms = model::kDefaultIdleGapThresholdMs) {

    model::TimingMeasurement timing{};
    timing.interval_count = distribution.sample_count;
    timing.min_interval_ms = distribution.min_interval_ms;
    timing.median_interval_ms = distribution.median_interval_ms;
    timing.p95_interval_ms = distribution.p95_interval_ms;
    timing.max_interval_ms = distribution.max_interval_ms;
    timing.jitter_p95_minus_median_ms =
        (std::max)(0.0, distribution.p95_interval_ms - distribution.median_interval_ms);

    if (distribution.sample_count == 0) return timing;

    return timing;
}

inline void apply_distribution_to_timing(
    const IntervalDistribution& distribution,
    model::TimingMeasurement& timing) {

    timing.interval_count = distribution.sample_count;
    timing.min_interval_ms = distribution.min_interval_ms;
    timing.median_interval_ms = distribution.median_interval_ms;
    timing.p95_interval_ms = distribution.p95_interval_ms;
    timing.max_interval_ms = distribution.max_interval_ms;
    timing.jitter_p95_minus_median_ms =
        (std::max)(0.0, distribution.p95_interval_ms - distribution.median_interval_ms);
}

} // namespace mouse_engine::observation
