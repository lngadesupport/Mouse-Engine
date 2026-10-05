#pragma once

#include <string>

namespace mouse_engine::model {

enum class EvidenceSource {
    DeviceReported,
    RawInput,
    Derived,
    UserConfigured,
    Inferred,
    Unknown
};

enum class EvidenceConfidence {
    High,
    Medium,
    Low,
    Unknown
};

struct Evidence {
    EvidenceSource source{EvidenceSource::Unknown};
    EvidenceConfidence confidence{EvidenceConfidence::Unknown};
    std::string method;
    std::string scope;
    std::string timestampUtc;
};

inline const char* evidence_source_name(EvidenceSource source) {
    switch (source) {
    case EvidenceSource::DeviceReported: return "device_reported";
    case EvidenceSource::RawInput: return "raw_input";
    case EvidenceSource::Derived: return "derived";
    case EvidenceSource::UserConfigured: return "user_configured";
    case EvidenceSource::Inferred: return "inferred";
    default: return "unknown";
    }
}

inline const char* evidence_confidence_name(EvidenceConfidence confidence) {
    switch (confidence) {
    case EvidenceConfidence::High: return "high";
    case EvidenceConfidence::Medium: return "medium";
    case EvidenceConfidence::Low: return "low";
    default: return "unknown";
    }
}

} // namespace mouse_engine::model
