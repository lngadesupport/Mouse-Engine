#pragma once

#include "Evidence.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mouse_engine::model {

struct TimingMeasurement {
    std::size_t interval_count{0};
    double min_interval_ms{0.0};
    double median_interval_ms{0.0};
    double p95_interval_ms{0.0};
    double max_interval_ms{0.0};
    double jitter_p95_minus_median_ms{0.0};
    std::size_t idle_gap_count_50ms{0};
    double longest_idle_gap_ms{0.0};
};

struct ObservationStream {
    std::size_t packet_count{0};
    TimingMeasurement timing{};
};

struct ActivitySummary {
    std::size_t active_run_count{0};
    std::size_t longest_active_run_packets{0};
    double longest_active_run_ms{0.0};
    double idle_gap_threshold_ms{50.0};
};

struct ObservationAnomaly {
    std::string id;
    std::string severity;
    std::string type;
    std::string message;
    std::string stream;
    Evidence evidence{};
};

struct ObservationSession {
    static constexpr int kSchemaVersion = 3;

    std::string id;
    std::string device_id;
    std::string started_at_utc;
    std::string ended_at_utc;
    std::string timing_scope{"WM_INPUT arrival inter-arrival"};
    ObservationStream all{};
    ObservationStream movement{};
    ObservationStream button{};
    ObservationStream wheel{};
    ActivitySummary activity{};
    std::vector<ObservationAnomaly> anomalies;
};

inline bool timing_is_non_negative(const TimingMeasurement& timing) {
    return timing.min_interval_ms >= 0.0 &&
           timing.median_interval_ms >= 0.0 &&
           timing.p95_interval_ms >= 0.0 &&
           timing.max_interval_ms >= 0.0 &&
           timing.jitter_p95_minus_median_ms >= 0.0 &&
           timing.longest_idle_gap_ms >= 0.0;
}

inline bool observation_session_is_valid(const ObservationSession& session) {
    return !session.id.empty() &&
           !session.timing_scope.empty() &&
           timing_is_non_negative(session.all.timing) &&
           timing_is_non_negative(session.movement.timing) &&
           timing_is_non_negative(session.button.timing) &&
           timing_is_non_negative(session.wheel.timing) &&
           session.activity.idle_gap_threshold_ms >= 0.0;
}

} // namespace mouse_engine::model
