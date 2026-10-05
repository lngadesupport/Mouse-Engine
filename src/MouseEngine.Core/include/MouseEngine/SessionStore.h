#pragma once

#include "SessionSerializer.h"
#include "Workspace.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace mouse_engine::session {

struct SessionAnomalySummary {
    std::string id;
    std::string severity;
    std::string type;
    std::string message;
    std::string stream;
    std::size_t packet_index{0};
    double timestamp_ms{0.0};
};

struct SessionSummary {
    std::string id;
    std::string device_id;
    std::string started_at_utc;
    std::string ended_at_utc;
    std::size_t packet_count{0};
    std::size_t interval_count{0};
    double median_interval_ms{0.0};
    double p95_interval_ms{0.0};
    double jitter_p95_minus_median_ms{0.0};
    std::size_t idle_gap_count_50ms{0};
    std::size_t active_run_count{0};
    std::size_t longest_active_run_packets{0};
    std::size_t distribution_sample_count{0};
    double distribution_mean_interval_ms{0.0};
    double distribution_bucket_width_ms{0.0};
    std::vector<model::DistributionBucket> distribution_buckets;
    std::vector<SessionAnomalySummary> anomalies;
    bool trace_available{false};
    std::size_t trace_packet_count{0};
    bool complete{false};
};

class SessionStore {
public:
    explicit SessionStore(workspace::WorkspacePaths paths)
        : paths_(std::move(paths)) {}

    std::filesystem::path path_for(const std::string& session_id) const {
        return paths_.sessions / (sanitize_id(session_id) + ".json");
    }

    const workspace::WorkspacePaths& paths() const noexcept { return paths_; }

    std::vector<SessionSummary> list(std::string* error = nullptr) const {
        std::vector<SessionSummary> result;
        std::error_code ec;
        if (!std::filesystem::exists(paths_.sessions, ec)) return result;
        if (ec) {
            fail(error, "cannot inspect sessions directory: " + ec.message());
            return result;
        }

        for (const auto& entry : std::filesystem::directory_iterator(paths_.sessions, ec)) {
            if (ec) break;
            if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

            std::ifstream input(entry.path(), std::ios::binary);
            if (!input) continue;
            const std::string json((std::istreambuf_iterator<char>(input)), {});
            SessionSummary summary;
            if (parse_summary(json, &summary)) result.push_back(std::move(summary));
        }

        std::sort(result.begin(), result.end(), [](const SessionSummary& left, const SessionSummary& right) {
            return left.started_at_utc > right.started_at_utc;
        });

        if (ec) {
            fail(error, "cannot enumerate sessions directory: " + ec.message());
            return {};
        }
        return result;
    }

    bool save(const model::ObservationSession& session, std::string* error = nullptr) const {
        if (!model::observation_session_is_valid(session) || session.id.empty()) {
            return fail(error, "invalid observation session");
        }

        std::error_code ec;
        std::filesystem::create_directories(paths_.sessions, ec);
        if (ec) return fail(error, "cannot create sessions directory: " + ec.message());

        const auto destination = path_for(session.id);
        const auto temporary = destination.string() + ".tmp";

        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) return fail(error, "cannot create temporary session file");
            output << serialize_json(session);
            if (!output) return fail(error, "cannot write temporary session file");
        }

        std::filesystem::remove(destination, ec);
        ec.clear();
        std::filesystem::rename(temporary, destination, ec);
        if (ec) {
            std::filesystem::remove(temporary);
            return fail(error, "cannot commit session file: " + ec.message());
        }
        return true;
    }

private:
    static bool string_field(const std::string& json, const char* key, std::string* value) {
        const std::regex pattern(std::string("\"") + key + "\":\"([^\"]*)\"");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) return false;
        *value = match[1].str();
        return true;
    }

    static bool size_field(const std::string& json, const char* key, std::size_t* value) {
        const std::regex pattern(std::string("\"") + key + "\":([0-9]+)");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) return false;
        try {
            *value = static_cast<std::size_t>(std::stoull(match[1].str()));
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool double_field(const std::string& json, const char* key, double* value) {
        const std::regex pattern(std::string("\"") + key + "\":([-+]?[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?)");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) return false;
        try {
            *value = std::stod(match[1].str());
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool parse_summary(const std::string& json, SessionSummary* summary) {
        if (!summary) return false;

        if (!string_field(json, "id", &summary->id) ||
            !string_field(json, "deviceId", &summary->device_id) ||
            !string_field(json, "startedAtUtc", &summary->started_at_utc) ||
            !string_field(json, "endedAtUtc", &summary->ended_at_utc)) {
            return false;
        }

        const auto all_pos = json.find("\"streams\":{\"all\":");
        if (all_pos == std::string::npos) return false;
        const std::string all = json.substr(all_pos);

        if (!size_field(all, "packetCount", &summary->packet_count) ||
            !size_field(all, "intervalCount", &summary->interval_count) ||
            !double_field(all, "medianIntervalMs", &summary->median_interval_ms) ||
            !double_field(all, "p95IntervalMs", &summary->p95_interval_ms) ||
            !double_field(all, "jitterP95MinusMedianMs", &summary->jitter_p95_minus_median_ms) ||
            !size_field(all, "idleGapCount50ms", &summary->idle_gap_count_50ms) ||
            !size_field(all, "sampleCount", &summary->distribution_sample_count) ||
            !double_field(all, "meanIntervalMs", &summary->distribution_mean_interval_ms) ||
            !double_field(all, "bucketWidthMs", &summary->distribution_bucket_width_ms) ||
            !size_field(json, "activeRunCount", &summary->active_run_count) ||
            !size_field(json, "longestActiveRunPackets", &summary->longest_active_run_packets)) {
            return false;
        }

        const auto distribution_pos = all.find("\"distribution\":{");
        if (distribution_pos == std::string::npos) return false;
        const auto buckets_pos = all.find("\"buckets\":[", distribution_pos);
        if (buckets_pos == std::string::npos) return false;
        const auto buckets_end = all.find("]", buckets_pos);
        if (buckets_end == std::string::npos) return false;
        const std::string buckets = all.substr(buckets_pos, buckets_end - buckets_pos);
        const std::regex bucket_pattern(
            "\\{\\\"lowerBoundMs\\\":([-+]?[0-9]+(?:\\.[0-9]+)?),"
            "\\\"upperBoundMs\\\":([-+]?[0-9]+(?:\\.[0-9]+)?),"
            "\\\"count\\\":([0-9]+),"
            "\\\"cumulativeFraction\\\":([-+]?[0-9]+(?:\\.[0-9]+)?)\\}");
        for (std::sregex_iterator it(buckets.begin(), buckets.end(), bucket_pattern), end; it != end; ++it) {
            model::DistributionBucket bucket;
            bucket.lower_bound_ms = std::stod((*it)[1].str());
            bucket.upper_bound_ms = std::stod((*it)[2].str());
            bucket.count = static_cast<std::size_t>(std::stoull((*it)[3].str()));
            bucket.cumulative_fraction = std::stod((*it)[4].str());
            summary->distribution_buckets.push_back(bucket);
        }

        const auto anomalies_pos = json.find("\"anomalies\":[");
        if (anomalies_pos != std::string::npos) {
            const auto anomalies_end = json.find("]", anomalies_pos);
            if (anomalies_end != std::string::npos) {
                const std::string anomaly_json = json.substr(anomalies_pos, anomalies_end - anomalies_pos);
                const std::regex anomaly_pattern(
                    "\\{\\\"id\\\":\\\"([^\\\"]*)\\\","
                    "\\\"severity\\\":\\\"([^\\\"]*)\\\","
                    "\\\"type\\\":\\\"([^\\\"]*)\\\","
                    "\\\"message\\\":\\\"([^\\\"]*)\\\","
                    "\\\"stream\\\":\\\"([^\\\"]*)\\\","
                    "\\\"packetIndex\\\":([0-9]+),"
                    "\\\"timestampMs\\\":([-+]?[0-9]+(?:\\\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?)\\\}");
                for (std::sregex_iterator it(anomaly_json.begin(), anomaly_json.end(), anomaly_pattern), end; it != end; ++it) {
                    SessionAnomalySummary anomaly;
                    anomaly.id = (*it)[1].str();
                    anomaly.severity = (*it)[2].str();
                    anomaly.type = (*it)[3].str();
                    anomaly.message = (*it)[4].str();
                    anomaly.stream = (*it)[5].str();
                    try {
                        anomaly.packet_index = static_cast<std::size_t>(std::stoull((*it)[6].str()));
                        anomaly.timestamp_ms = std::stod((*it)[7].str());
                    } catch (...) {
                        continue;
                    }
                    summary->anomalies.push_back(std::move(anomaly));
                }
            }
        }

        const auto trace_path = paths_.sessions / (sanitize_id(summary->id) + ".trace.jsonl");
        std::ifstream trace_input(trace_path, std::ios::binary);
        if (trace_input) {
            std::string trace_header;
            if (std::getline(trace_input, trace_header)) {
                std::size_t trace_count = 0;
                if (size_field(trace_header, "packetCount", &trace_count)) {
                    summary->trace_available = true;
                    summary->trace_packet_count = trace_count;
                }
            }
        }

        summary->complete = true;
        return true;
    }

    static std::string sanitize_id(const std::string& id) {
        std::string out;
        out.reserve(id.size());
        for (const char ch : id) {
            const bool safe =
                (ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') ||
                ch == '-' || ch == '_' || ch == '.';
            out += safe ? ch : '_';
        }
        return out.empty() ? "session" : out;
    }

    static bool fail(std::string* error, const std::string& message) {
        if (error) *error = message;
        return false;
    }

    workspace::WorkspacePaths paths_;
};

} // namespace mouse_engine::session
