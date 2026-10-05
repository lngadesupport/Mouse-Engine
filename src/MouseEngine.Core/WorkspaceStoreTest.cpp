#include "MouseEngine/WorkspaceStore.h"

#include <cassert>
#include <filesystem>
#include <string>

int main() {
    namespace fs = std::filesystem;
    using mouse_engine::workspace::WorkspacePaths;
    using mouse_engine::workspace::WorkspaceStore;

    const fs::path root = fs::temp_directory_path() / "Mouse Engine Workspace Store Test";
    std::error_code ec;
    fs::remove_all(root, ec);

    WorkspaceStore store(WorkspacePaths::from_root(root));
    std::string error;
    assert(store.initialize(&error));
    assert(error.empty());
    assert(fs::is_directory(store.paths().presets));
    assert(fs::is_directory(store.paths().sessions));
    assert(fs::is_directory(store.paths().diagnostics));
    assert(fs::is_regular_file(store.paths().manifest));
    assert(store.verify(&error));

    const auto original = fs::file_size(store.paths().manifest);
    assert(original > 0);

    assert(store.write_manifest({}, &error));
    assert(fs::file_size(store.paths().manifest) == original);

    fs::remove(store.paths().manifest, ec);
    assert(!store.verify(&error));
    assert(!error.empty());

    fs::remove_all(root, ec);
    return 0;
}
