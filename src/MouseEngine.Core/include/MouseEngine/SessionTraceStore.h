#pragma once

#include "SessionTrace.h"
#include "Workspace.h"

#include <filesystem>
#include <fstream>
#include <limits>
#include <cmath>
#include <regex>
#include <string>
#include <utility>

namespace mouse_engine::session {

class SessionTraceStore {
public:
    explicit SessionTraceStore(workspace::WorkspacePaths paths)
        : paths_(std::move(paths)) {}

    std::filesystem::path path_for(const std::string& session_id) const {
        return paths_.sessions / (sanitize_id(session_id) + ".trace.jsonl");
    }

    bool exists(const std::string& session_id) const {
        std::error_code ec;
        return std::filesystem::is_regular_file(path_for(session_id), ec);
    }

    bool save(const SessionTrace& trace, std::string* error = nullptr) const {
        if (trace.schema_version != SessionTrace::kSchemaVersion ||
            trace.session_id.empty() || trace.device_id.empty() ||
            trace.packets.size() > SessionTrace::kMaxPackets) {
            return fail(error, "invalid session trace");
        }

        double previous_timestamp = -1.0;
        for (const auto& packet : trace.packets) {
            if (!std::isfinite(packet.timestamp_ms) ||
                packet.timestamp_ms < 0.0 ||
                packet.timestamp_ms < previous_timestamp) {
                return fail(error, "invalid session trace timestamp sequence");
            }
            previous_timestamp = packet.timestamp_ms;
        }

        std::error_code ec;
        std::filesystem::create_directories(paths_.sessions, ec);
        if (ec) return fail(error, "cannot create sessions directory: " + ec.message());

        const auto destination = path_for(trace.session_id);
        const auto temporary = destination.string() + ".tmp";
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) return fail(error, "cannot create temporary trace file");

            output << "{\"schemaVersion\":" << trace.schema_version
                   << ",\"sessionId\":\"" << json_escape(trace.session_id)
                   << "\",\"deviceId\":\"" << json_escape(trace.device_id)
                   << "\",\"packetCount\":" << trace.packets.size()
                   << ",\"truncated\":" << (trace.truncated ? "true" : "false") << "}\n";

            for (const auto& packet : trace.packets) {
                output << "{\"timestampMs\":" << packet.timestamp_ms
                       << ",\"classes\":" << packet.classes
                       << ",\"dx\":" << packet.dx
                       << ",\"dy\":" << packet.dy
                       << ",\"buttons\":" << packet.buttons
                       << ",\"wheel\":" << packet.wheel << "}\n";
            }

            if (!output) return fail(error, "cannot write temporary trace file");
        }

        std::filesystem::remove(destination, ec);
        ec.clear();
        std::filesystem::rename(temporary, destination, ec);
        if (ec) {
            std::filesystem::remove(temporary);
            return fail(error, "cannot commit trace file: " + ec.message());
        }
        return true;
    }

    bool load(
        const std::string& session_id,
        SessionTrace* trace,
        std::string* error = nullptr) const {

        if (!trace) return fail(error, "trace output is null");
        *trace = {};

        std::ifstream input(path_for(session_id), std::ios::binary);
        if (!input) return fail(error, "trace file not found");

        std::string line;
        if (!std::getline(input, line)) return fail(error, "trace header missing");

        std::size_t schema = 0;
        std::size_t header_packet_count = 0;
        if (!size_field(line, "schemaVersion", &schema) ||
            schema != static_cast<std::size_t>(SessionTrace::kSchemaVersion) ||
            !string_field(line, "sessionId", &trace->session_id) ||
            !string_field(line, "deviceId", &trace->device_id) ||
            !size_field(line, "packetCount", &header_packet_count)) {
            return fail(error, "invalid trace header");
        }

        trace->schema_version = static_cast<int>(schema);
        trace->truncated = line.find("\"truncated\":true") != std::string::npos;

        while (std::getline(input, line)) {
            if (line.empty()) continue;

            TracePacket packet;
            if (!double_field(line, "timestampMs", &packet.timestamp_ms) ||
                !uint_field(line, "classes", &packet.classes) ||
                !int_field(line, "dx", &packet.dx) ||
                !int_field(line, "dy", &packet.dy) ||
                !uint_field(line, "buttons", &packet.buttons) ||
                !int_field(line, "wheel", &packet.wheel)) {
                return fail(error, "invalid trace packet");
            }
            trace->packets.push_back(packet);
        }

        if (input.bad()) return fail(error, "cannot read trace file");
        if (trace->session_id != session_id) return fail(error, "trace session id mismatch");
        if (trace->packets.size() != header_packet_count) {
            return fail(error, "trace packet count mismatch");
        }
        return true;
    }

private:
    static bool string_field(
        const std::string& json,
        const char* key,
        std::string* value) {

        const std::regex pattern(
            std::string("\"") + key + "\":\"([^\"]*)\"");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) return false;
        *value = match[1].str();
        return true;
    }

    static bool size_field(
        const std::string& json,
        const char* key,
        std::size_t* value) {

        const std::regex pattern(
            std::string("\"") + key + "\":([0-9]+)");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) return false;

        try {
            *value = static_cast<std::size_t>(std::stoull(match[1].str()));
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool uint_field(
        const std::string& json,
        const char* key,
        unsigned int* value) {

        std::size_t parsed = 0;
        if (!size_field(json, key, &parsed)) return false;
        if (parsed > static_cast<std::size_t>((std::numeric_limits<unsigned int>::max)())) {
            return false;
        }
        *value = static_cast<unsigned int>(parsed);
        return true;
    }

    static bool int_field(
        const std::string& json,
        const char* key,
        std::int32_t* value) {

        const std::regex pattern(
            std::string("\"") + key + "\":([-+]?[0-9]+)");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) return false;

        try {
            const auto parsed = std::stoll(match[1].str());
            if (parsed < static_cast<long long>((std::numeric_limits<std::int32_t>::min)()) ||
                parsed > static_cast<long long>((std::numeric_limits<std::int32_t>::max)())) {
                return false;
            }
            *value = static_cast<std::int32_t>(parsed);
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool double_field(
        const std::string& json,
        const char* key,
        double* value) {

        const std::regex pattern(
            std::string("\"") + key +
            "\":([-+]?[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?)");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) return false;

        try {
            *value = std::stod(match[1].str());
            return true;
        } catch (...) {
            return false;
        }
    }

    static std::string json_escape(const std::string& value) {
        std::string out;
        out.reserve(value.size() + 8);
        for (const char ch : value) {
            switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += ch; break;
            }
        }
        return out;
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
