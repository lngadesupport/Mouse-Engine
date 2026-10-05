#pragma once

#include <filesystem>
#include <string>
#include <utility>

namespace mouse_engine::workspace {

inline constexpr int kWorkspaceSchemaVersion = 1;
inline constexpr const char* kWorkspaceDirectoryName = "Mouse Engine";

struct WorkspacePaths {
    std::filesystem::path root;
    std::filesystem::path manifest;
    std::filesystem::path devices;
    std::filesystem::path presets;
    std::filesystem::path profiles;
    std::filesystem::path sessions;
    std::filesystem::path diagnostics;
    std::filesystem::path reports;
    std::filesystem::path experiments;
    std::filesystem::path backups;

    static WorkspacePaths from_root(std::filesystem::path root_path) {
        WorkspacePaths paths;
        paths.root = std::move(root_path);
        paths.manifest = paths.root / "workspace.json";
        paths.devices = paths.root / "Devices";
        paths.presets = paths.root / "Presets";
        paths.profiles = paths.root / "Profiles";
        paths.sessions = paths.root / "Sessions";
        paths.diagnostics = paths.root / "Diagnostics";
        paths.reports = paths.root / "Reports";
        paths.experiments = paths.root / "Experiments";
        paths.backups = paths.root / "Backups";
        return paths;
    }
};

inline bool is_workspace_root_shape_valid(const std::filesystem::path& root) {
    return !root.empty();
}

inline std::string workspace_schema_name() {
    return "mouse-engine-workspace";
}

} // namespace mouse_engine::workspace
