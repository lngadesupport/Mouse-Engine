#include "MouseEngine/SessionStore.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    namespace fs = std::filesystem;

    const fs::path root = fs::temp_directory_path() / "Mouse Engine Session Store Test";
    std::error_code ec;
    fs::remove_all(root, ec);

    mouse_engine::session::SessionStore store(
        mouse_engine::workspace::WorkspacePaths::from_root(root));

    mouse_engine::model::ObservationSession session;
    session.id = "session/unsafe";
    session.device_id = "device-1";
    session.started_at_utc = "2026-10-05T15:00:00Z";
    session.ended_at_utc = "2026-10-05T15:00:01Z";

    std::string error;
    assert(store.save(session, &error));
    assert(error.empty());

    const auto path = store.path_for(session.id);
    assert(path.filename() == "session_unsafe.json");
    assert(fs::is_regular_file(path));

    std::ifstream input(path, std::ios::binary);
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    assert(json.find(""schemaVersion":3") != std::string::npos);
    assert(json.find(""id":"session/unsafe"") != std::string::npos);

    fs::remove_all(root, ec);
    return 0;
}
