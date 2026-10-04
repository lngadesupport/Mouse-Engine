#pragma once
#include "DeviceIdentity.h"
#include <string>
#include <vector>

namespace mouse_engine::core {

enum class CompatibilityState {
    Unknown,
    Compatible,
    ConditionallyCompatible,
    Incompatible
};

struct CompatibilityAssessment {
    CompatibilityState state{CompatibilityState::Unknown};
    bool write_authorized{false};
    std::string adapter_id;
    std::vector<std::string> reasons;

    bool can_write() const noexcept {
        return write_authorized && state == CompatibilityState::Compatible;
    }
};

} // namespace mouse_engine::core
