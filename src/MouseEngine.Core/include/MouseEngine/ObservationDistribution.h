#pragma once

#include "ObservationAnalysis.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace mouse_engine::observation {

struct DistributionBucket {
    double lower_bound_ms{0.0};
    double upper_bound_ms{0.0};
    std::size_t count{0};
    double cumulative_fraction{0.0};
};

struct IntervalDistribution {
    std::size_t sample_count{0};
    double min_interval_ms{0.0};
    double max_interval_ms{0.0};
    double mean_interval_ms{0.0};
    double median_interval_ms{0.0};
    double p95_interval_ms{0.0};
    double bucket_width_ms{0.0};
    std::vector<DistributionBucket> buckets;
};

inline IntervalDistribution build_interval_distribution(
    const std::vector<TimedPacket>& packets,
    std::size_t requested_bucket_count = 24) {

    IntervalDistribution distribution;
    const auto intervals = sorted_intervals(packets);
    if (intervals.empty()) return distribution;

    distribution.sample_count = intervals.size();
    distribution.min_interval_ms = intervals.front();
    distribution.max_interval_ms = intervals.back();

    double sum = 0.0;
    for (const double value : intervals) sum += value;
    distribution.mean_interval_ms = sum / static_cast<double>(intervals.size());
    distribution.median_interval_ms = percentile(intervals, 0.50);
    distribution.p95_interval_ms = percentile(intervals, 0.95);

    if (distribution.min_interval_ms == distribution.max_interval_ms) {
        distribution.bucket_width_ms = 0.0;
        distribution.buckets.push_back({
            distribution.min_interval_ms,
            distribution.max_interval_ms,
            distribution.sample_count,
            1.0
        });
        return distribution;
    }

    const std::size_t bucket_count =
        (std::max)(std::size_t{1}, (std::min)(requested_bucket_count, intervals.size()));
    distribution.bucket_width_ms =
        (distribution.max_interval_ms - distribution.min_interval_ms) /
        static_cast<double>(bucket_count);

    distribution.buckets.resize(bucket_count);
    for (std::size_t index = 0; index < bucket_count; ++index) {
        auto& bucket = distribution.buckets[index];
        bucket.lower_bound_ms =
            distribution.min_interval_ms + distribution.bucket_width_ms * static_cast<double>(index);
        bucket.upper_bound_ms =
            index + 1 == bucket_count
                ? distribution.max_interval_ms
                : distribution.min_interval_ms + distribution.bucket_width_ms * static_cast<double>(index + 1);
    }

    for (const double value : intervals) {
        std::size_t index = static_cast<std::size_t>(
            (value - distribution.min_interval_ms) / distribution.bucket_width_ms);
        if (index >= bucket_count) index = bucket_count - 1;
        ++distribution.buckets[index].count;
    }

    std::size_t cumulative = 0;
    for (auto& bucket : distribution.buckets) {
        cumulative += bucket.count;
        bucket.cumulative_fraction =
            static_cast<double>(cumulative) / static_cast<double>(distribution.sample_count);
    }
    return distribution;
}

} // namespace mouse_engine::observation
