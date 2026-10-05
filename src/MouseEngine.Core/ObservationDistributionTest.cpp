#include "MouseEngine/ObservationDistribution.h"
#include "MouseEngine/SessionSerializer.h"

#include <cassert>
#include <cmath>

int main() {
    using mouse_engine::observation::TimedPacket;
    using mouse_engine::observation::build_interval_distribution;

    const std::vector<TimedPacket> packets{
        {0.0, 1u}, {1.0, 1u}, {3.0, 1u}, {6.0, 1u},
        {10.0, 1u}, {15.0, 1u}, {21.0, 1u}, {28.0, 1u}
    };

    const auto distribution = build_interval_distribution(packets, 4);
    assert(distribution.sample_count == 7);
    assert(std::abs(distribution.min_interval_ms - 1.0) < 1e-9);
    assert(std::abs(distribution.max_interval_ms - 7.0) < 1e-9);
    assert(std::abs(distribution.mean_interval_ms - 4.0) < 1e-9);
    assert(std::abs(distribution.median_interval_ms - 4.0) < 1e-9);
    assert(distribution.p95_interval_ms > 6.5);
    assert(distribution.p95_interval_ms < 7.0);

    std::size_t bucket_total = 0;
    for (const auto& bucket : distribution.buckets) bucket_total += bucket.count;
    assert(bucket_total == distribution.sample_count);

    mouse_engine::model::ObservationSession session;
    session.id = "session-distribution";
    session.device_id = "mouse-instance";
    session.started_at_utc = "2026-10-05T16:00:00.000Z";
    session.ended_at_utc = "2026-10-05T16:00:01.000Z";
    session.all.packet_count = 8;
    session.all.timing.interval_count = distribution.sample_count;
    session.all.timing.min_interval_ms = distribution.min_interval_ms;
    session.all.timing.median_interval_ms = distribution.median_interval_ms;
    session.all.timing.p95_interval_ms = distribution.p95_interval_ms;
    session.all.timing.max_interval_ms = distribution.max_interval_ms;
    session.all.distribution = mouse_engine::observation::to_model_distribution(distribution);

    const auto json = mouse_engine::session::serialize_json(session);
    assert(json.find("\"intervalCount\":7") != std::string::npos);
    assert(json.find("\"p95IntervalMs\":") != std::string::npos);
    assert(json.find("\"distribution\":") != std::string::npos);
    assert(json.find("\"cumulativeFraction\":1") != std::string::npos);

    return 0;
}
