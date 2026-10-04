#pragma once

#include "Evidence.h"

#include <string>
#include <vector>

namespace mouse_engine::core {

enum class CapabilityCategory {
    Input,
    Performance,
    Sensor,
    Controls,
    Lighting,
    Profiles,
    Diagnostics,
    Firmware
};

enum class CapabilityAccess {
    ReadOnly,
    ReadWrite
};

struct Capability {
    std::string id;
    CapabilityCategory category{};
    CapabilityAccess access{CapabilityAccess::ReadOnly};
    EvidenceLevel evidence{EvidenceLevel::Unsupported};
    std::vector<std::string> sources;

    bool supported() const noexcept {
        return evidence != EvidenceLevel::Unsupported;
    }

    bool writable() const noexcept {
        return access == CapabilityAccess::ReadWrite && supported();
    }
};

} // namespace mouse_engine::core
