#include "InputTiming.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace mouse_engine::windows {

InputTimingAccumulator::InputTimingAccumulator(std::size_t capacity)
    : capacity_(std::max<std::size_t>(capacity, 1)) {}

void InputTimingAccumulator::record(
    std::uint64_t timestamp_ticks,
    std::uint64_t frequency_ticks) {
    if (frequency_ticks == 0) return;

    if (has_last_) {
        if (timestamp_ticks <= last_ticks_) {
            last_ticks_ = timestamp_ticks;
            return;
        }

        const std::uint64_t delta_ticks = timestamp_ticks - last_ticks_;
        const long double interval_ms =
            (static_cast<long double>(delta_ticks) * 1000.0L) /
            static_cast<long double>(frequency_ticks);

        if (std::isfinite(static_cast<double>(interval_ms)) &&
            interval_ms > 0.0L) {
            intervals_ms_.push_back(static_cast<double>(interval_ms));
            while (intervals_ms_.size() > capacity_) intervals_ms_.pop_front();
        }
    }

    last_ticks_ = timestamp_ticks;
    has_last_ = true;
}

bool InputTimingAccumulator::snapshot(InputTimingSummary& out) const {
    out = {};
    if (intervals_ms_.empty()) return false;

    std::vector<double> sorted(intervals_ms_.begin(), intervals_ms_.end());
    std::sort(sorted.begin(), sorted.end());

    const auto percentile = [&sorted](double p) {
        const double position = p * static_cast<double>(sorted.size() - 1);
        const std::size_t lower = static_cast<std::size_t>(std::floor(position));
        const std::size_t upper = static_cast<std::size_t>(std::ceil(position));
        if (lower == upper) return sorted[lower];
        const double weight = position - static_cast<double>(lower);
        return sorted[lower] + (sorted[upper] - sorted[lower]) * weight;
    };

    out.interval_count = sorted.size();
    out.min_interval_ms = sorted.front();
    out.median_interval_ms = percentile(0.50);
    out.p95_interval_ms = percentile(0.95);
    out.max_interval_ms = sorted.back();
    out.jitter_p95_minus_median_ms =
        std::max(0.0, out.p95_interval_ms - out.median_interval_ms);
    return true;
}

void InputTimingAccumulator::clear() {
    intervals_ms_.clear();
    last_ticks_ = 0;
    has_last_ = false;
}

} // namespace mouse_engine::windows
