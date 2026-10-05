#include "MouseEngine/DevicePassport.h"
#include "MouseEngine/Evidence.h"
#include "MouseEngine/ObservationSession.h"
#include "MouseEngine/Workspace.h"

#include <cassert>
#include <filesystem>

int main() {
    using namespace mouse_engine::model;
    using namespace mouse_engine::workspace;

    const auto paths = WorkspacePaths::from_root(std::filesystem::path("C:/Users/Test/Documents/Mouse Engine"));
    assert(paths.root.filename() == "Mouse Engine");
    assert(paths.manifest.filename() == "workspace.json");
    assert(paths.sessions.filename() == "Sessions");
    assert(paths.backups.filename() == "Backups");
    assert(is_workspace_path_safe(paths.root));
    assert(!is_workspace_path_safe(std::filesystem::path("C:/Users/Test/AppData/Mouse Engine")));

    DevicePassport passport;
    passport.id = "device-001";
    passport.identity.resolved = true;
    passport.identity.transport = DeviceTransport::Usb;
    passport.identity.evidence.source = EvidenceSource::DeviceReported;
    passport.capabilities.push_back(DeviceCapability{
        "dpi", CapabilityState::ReadOnly, true, true, false, "generic-hid",
        Evidence{EvidenceSource::DeviceReported, EvidenceConfidence::High, "SetupAPI", "device", "2026-10-05T00:00:00Z"}
    });
    assert(passport.identity.resolved);
    assert(passport.capabilities.front().state == CapabilityState::ReadOnly);

    ObservationSession session;
    session.id = "session-001";
    session.device_id = passport.id;
    session.all.packet_count = 10;
    session.all.timing.interval_count = 9;
    session.all.timing.median_interval_ms = 1.0;
    session.all.timing.p95_interval_ms = 1.2;
    session.activity.idle_gap_threshold_ms = 50.0;
    assert(observation_session_is_valid(session));

    session.all.timing.median_interval_ms = -1.0;
    assert(!observation_session_is_valid(session));

    return 0;
}
