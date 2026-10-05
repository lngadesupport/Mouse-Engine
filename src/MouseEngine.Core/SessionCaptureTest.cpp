#include "MouseEngine/SessionCapture.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    namespace fs = std::filesystem;

    const fs::path root = fs::temp_directory_path() / "Mouse Engine Session Capture Test";
    std::error_code ec;
    fs::remove_all(root, ec);

    mouse_engine::session::SessionCapture capture(
        mouse_engine::session::SessionStore(
            mouse_engine::workspace::WorkspacePaths::from_root(root)));

    assert(!capture.is_recording());
    assert(capture.start("device-instance-42", "2026-10-05T15:00:00Z"));
    assert(capture.is_recording());
    assert(capture.device_id() == "device-instance-42");
    assert(capture.packet_count() == 0);

    assert(capture.record({100.0, mouse_engine::observation::Movement}));
    assert(capture.record({110.0, mouse_engine::observation::Movement}));
    assert(capture.packet_count() == 2);

    std::string error;
    mouse_engine::model::ObservationSession saved;
    assert(capture.stop("2026-10-05T15:00:01Z", &saved, &error));
    assert(error.empty());
    assert(!capture.is_recording());
    assert(saved.device_id == "device-instance-42");
    assert(saved.all.packet_count == 2);
    assert(fs::is_regular_file(
        mouse_engine::session::SessionStore(
            mouse_engine::workspace::WorkspacePaths::from_root(root)).path_for(saved.id)));

    assert(!capture.stop("2026-10-05T15:00:02Z", nullptr, &error));

    const fs::path blocked = fs::temp_directory_path() / "Mouse Engine Session Capture Blocked";
    fs::remove_all(blocked, ec);
    {
        std::ofstream blocker(blocked, std::ios::binary | std::ios::trunc);
        blocker << "not a directory";
    }
    mouse_engine::session::SessionCapture failing(
        mouse_engine::session::SessionStore(
            mouse_engine::workspace::WorkspacePaths::from_root(blocked)));
    assert(failing.start("device-instance-43", "2026-10-05T15:01:00Z"));
    assert(failing.record({100.0, mouse_engine::observation::Movement}));
    error.clear();
    assert(!failing.stop("2026-10-05T15:01:01Z", nullptr, &error));
    assert(failing.is_recording());
    assert(failing.packet_count() == 1);

    fs::remove_all(blocked, ec);
    fs::remove_all(root, ec);
    return 0;
}
