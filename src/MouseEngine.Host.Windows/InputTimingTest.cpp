#include "InputTiming.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using mouse_engine::windows::InputTimingAccumulator;
    using mouse_engine::windows::InputTimingSummary;

    InputTimingAccumulator timing(16);
    timing.record(1000, 100000);
    timing.record(2000, 100000);
    timing.record(3000, 100000);
    timing.record(4000, 100000);
    timing.record(5000, 100000);
    timing.record(6000, 100000);

    InputTimingSummary summary{};
    assert(timing.snapshot(summary));
    assert(summary.interval_count == 5);
    assert(std::abs(summary.min_interval_ms - 10.0) < 0.001);
    assert(std::abs(summary.median_interval_ms - 10.0) < 0.001);
    assert(std::abs(summary.p95_interval_ms - 10.0) < 0.001);
    assert(std::abs(summary.max_interval_ms - 10.0) < 0.001);
    assert(std::abs(summary.jitter_p95_minus_median_ms) < 0.001);

    timing.record(8000, 100000);
    assert(timing.snapshot(summary));
    assert(summary.interval_count == 6);
    assert(summary.max_interval_ms >= 19.999);
    assert(summary.p95_interval_ms >= 10.0);

    InputTimingAccumulator bounded(3);
    bounded.record(1000, 100000);
    bounded.record(2000, 100000);
    bounded.record(4000, 100000);
    bounded.record(7000, 100000);
    bounded.record(6000, 100000); // Non-monotonic sample must not create a negative interval.
    bounded.record(10000, 100000);
    assert(bounded.snapshot(summary));
    assert(summary.interval_count == 3);
    assert(summary.min_interval_ms >= 19.999);
    assert(summary.max_interval_ms >= 29.999);

    InputTimingAccumulator empty(4);
    assert(!empty.snapshot(summary));

    std::cout << "InputTiming test PASS\n";
    return 0;
}
