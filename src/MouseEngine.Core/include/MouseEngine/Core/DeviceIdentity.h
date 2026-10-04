#pragma once
#include "DeviceFingerprint.h"
#include "Evidence.h"
#include <cstdint>
#include <string>
#include <vector>

namespace mouse_engine::core {

using PhysicalDeviceId = std::uint64_t;
using DeviceSessionId = std::uint64_t;

enum class IdentityConfidence { Unknown, Low, Medium, High, Verified };

struct IdentityEvidence {
    std::string source;
    EvidenceLevel level{EvidenceLevel::Unsupported};
    std::string value;
};

struct DeviceIdentity {
    PhysicalDeviceId physical_id{};
    DeviceSessionId session_id{};
    DeviceFingerprint fingerprint{};
    IdentityConfidence confidence{IdentityConfidence::Unknown};
    std::vector<IdentityEvidence> evidence;

    bool is_write_safe() const noexcept {
        return confidence == IdentityConfidence::Verified ||
               confidence == IdentityConfidence::High;
    }
};

} // namespace mouse_engine::core
