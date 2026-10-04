#pragma once

#include <string_view>

namespace mouse_engine::core {

enum class EvidenceLevel {
    Verified,
    Measured,
    Estimated,
    Experimental,
    Unsupported
};

constexpr std::string_view to_string(EvidenceLevel level) noexcept {
    switch (level) {
    case EvidenceLevel::Verified: return "Verified";
    case EvidenceLevel::Measured: return "Measured";
    case EvidenceLevel::Estimated: return "Estimated";
    case EvidenceLevel::Experimental: return "Experimental";
    case EvidenceLevel::Unsupported: return "Unsupported";
    }
    return "Unknown";
}

} // namespace mouse_engine::core
