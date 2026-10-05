#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>

namespace mouse_engine::windows {

struct InputTimingSummary {
    std::size_t interval_count{0};
    double min_interval_ms{0.0};
    double median_interval_ms{0.0};
    double p95_interval_ms{0.0};
    double max_interval_ms{0.0};
    double jitter_p95_minus_median_ms{0.0};
    std::size_t idle_gap_count_50ms{0};
    double longest_idle_gap_ms{0.0};
};

class InputTimingAccumulator {
public:
    explicit InputTimingAccumulator(std::size_t capacity = 256);
    void record(std::uint64_t timestamp_ticks, std::uint64_t frequency_ticks);
    bool snapshot(InputTimingSummary& out) const;
    void clear();

private:
    std::size_t capacity_;
    std::uint64_t last_ticks_{0};
    bool has_last_{false};
    std::deque<double> intervals_ms_;
};

} // namespace mouse_engine::windows
