#pragma once

#include "Workspace.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <utility>
#include <string>
#include <system_error>

namespace mouse_engine::workspace {

struct WorkspaceManifest {
    int schema_version{kWorkspaceSchemaVersion};
    std::string type{workspace_schema_name()};
    bool cloud_sync{false};
};

class WorkspaceStore {
public:
    explicit WorkspaceStore(WorkspacePaths paths) : paths_(std::move(paths)) {}

    const WorkspacePaths& paths() const noexcept {
        return paths_;
    }

    bool initialize(std::string* error = nullptr) const {
        if (!is_workspace_root_shape_valid(paths_.root)) {
            return fail(error, "workspace root has an invalid shape");
        }

        std::error_code ec;
        const std::filesystem::path directories[] = {
            paths_.root, paths_.devices, paths_.presets, paths_.profiles,
            paths_.sessions, paths_.diagnostics, paths_.reports,
            paths_.experiments, paths_.backups
        };

        for (const auto& directory : directories) {
            std::filesystem::create_directories(directory, ec);
            if (ec) {
                return fail(error, "cannot create workspace directory: " +
                    directory.string() + ": " + ec.message());
            }
        }

        if (!std::filesystem::exists(paths_.manifest)) {
            if (!write_manifest({}, error)) return false;
        }

        return verify(error);
    }

    bool verify(std::string* error = nullptr) const {
        if (!is_workspace_root_shape_valid(paths_.root)) {
            return fail(error, "workspace root has an invalid shape");
        }
        if (!std::filesystem::is_directory(paths_.root)) {
            return fail(error, "workspace root does not exist");
        }
        if (!std::filesystem::is_regular_file(paths_.manifest)) {
            return fail(error, "workspace manifest does not exist");
        }

        std::ifstream manifest(paths_.manifest, std::ios::binary);
        if (!manifest) return fail(error, "workspace manifest cannot be opened");

        const std::string contents(
            (std::istreambuf_iterator<char>(manifest)),
            std::istreambuf_iterator<char>());

        if (contents.find(""schemaVersion": 1") == std::string::npos ||
            contents.find(""type": "mouse-engine-workspace"") == std::string::npos ||
            contents.find(""cloudSync": false") == std::string::npos) {
            return fail(error, "workspace manifest does not match schema v1");
        }

        return true;
    }

    bool write_manifest(
        const WorkspaceManifest& manifest,
        std::string* error = nullptr) const {

        if (manifest.schema_version != kWorkspaceSchemaVersion) {
            return fail(error, "unsupported workspace schema version");
        }
        if (manifest.type != workspace_schema_name()) {
            return fail(error, "unsupported workspace manifest type");
        }

        std::ofstream output(paths_.manifest, std::ios::binary | std::ios::trunc);
        if (!output) return fail(error, "workspace manifest cannot be written");

        output
            << "{
"
            << "  "schemaVersion": " << manifest.schema_version << ",
"
            << "  "type": "" << manifest.type << "",
"
            << "  "cloudSync": " << (manifest.cloud_sync ? "true" : "false") << ",
"
            << "  "createdBy": "Mouse Engine"
"
            << "}
";

        if (!output) return fail(error, "workspace manifest write failed");
        return true;
    }

private:
    static bool fail(std::string* error, const std::string& message) {
        if (error) *error = message;
        return false;
    }

    WorkspacePaths paths_;
};

} // namespace mouse_engine::workspace
