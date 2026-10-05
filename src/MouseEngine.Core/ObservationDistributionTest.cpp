#include "MouseEngine/ObservationDistribution.h"

#include <cassert>
#include <cmath>

int main() {
    using mouse_engine::observation::TimedPacket;

    const std::vector<TimedPacket> packets{
        {0.0, 1u}, {1.0, 1u}, {3.0, 1u}, {6.0, 1u},
        {10.0, 1u}, {15.0, 1u}, {21.0, 1u}, {28.0, 1u}
    };

    const auto distribution =
        mouse_engine::observation::build_interval_distribution(packets, 4);

    assert(distribution.sample_count == 7);
    assert(std::abs(distribution.min_interval_ms - 1.0) < 1e-9);
    assert(std::abs(distribution.max_interval_ms - 7.0) < 1e-9);
    assert(std::abs(distribution.mean_interval_ms - 4.0) < 1e-9);
    assert(std::abs(distribution.median_interval_ms - 4.0) < 1e-9);
    assert(distribution.p95_interval_ms > 6.5);
    assert(distribution.p95_interval_ms < 7.0);
    assert(distribution.buckets.size() == 4);

    std::size_t bucket_total = 0;
    for (const auto& bucket : distribution.buckets) {
        assert(bucket.count > 0);
        bucket_total += bucket.count;
        assert(bucket.upper_bound_ms > bucket.lower_bound_ms);
    }
    assert(bucket_total == distribution.sample_count);

    const auto constant =
        mouse_engine::observation::build_interval_distribution(
            {{0.0, 1u}, {5.0, 1u}, {10.0, 1u}, {15.0, 1u}}, 8);
    assert(constant.sample_count == 3);
    assert(constant.min_interval_ms == 5.0);
    assert(constant.max_interval_ms == 5.0);
    assert(constant.buckets.size() == 1);
    assert(constant.buckets.front().count == 3);

    const auto empty =
        mouse_engine::observation::build_interval_distribution({}, 8);
    assert(empty.sample_count == 0);
    assert(empty.buckets.empty());

    return 0;
}
