#pragma once

#include "SessionSerializer.h"
#include "Workspace.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <system_error>

namespace mouse_engine::session {

class SessionStore {
public:
    explicit SessionStore(workspace::WorkspacePaths paths)
        : paths_(std::move(paths)) {}

    std::filesystem::path path_for(const std::string& session_id) const {
        return paths_.sessions / (sanitize_id(session_id) + ".json");
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
