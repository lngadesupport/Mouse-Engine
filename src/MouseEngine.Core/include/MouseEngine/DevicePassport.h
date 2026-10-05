#pragma once

#include "Evidence.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mouse_engine::model {

enum class DeviceTransport {
    Unknown,
    Usb,
    Bluetooth
};

enum class CapabilityState {
    Unknown,
    Unsupported,
    ReadOnly,
    Supported,
    Experimental
};

struct DeviceCapability {
    std::string id;
    CapabilityState state{CapabilityState::Unknown};
    bool hardware_verified{false};
    bool protocol_verified{false};
    bool write_available{false};
    std::string backend;
    Evidence evidence{};
};

struct DeviceIdentity {
    bool resolved{false};
    std::string interface_path;
    std::string instance_id;
    std::string container_id;
    std::string manufacturer;
    std::string product;
    std::string vid;
    std::string pid;
    DeviceTransport transport{DeviceTransport::Unknown};
    std::string topology_hash;
    Evidence evidence{};
};

struct DevicePassport {
    std::string id;
    std::string first_seen_utc;
    std::string last_seen_utc;
    DeviceIdentity identity{};
    std::vector<DeviceCapability> capabilities;
};

inline const char* device_transport_name(DeviceTransport transport) {
    switch (transport) {
    case DeviceTransport::Usb: return "Usb";
    case DeviceTransport::Bluetooth: return "Bluetooth";
    default: return "Unknown";
    }
}

inline const char* capability_state_name(CapabilityState state) {
    switch (state) {
    case CapabilityState::Unsupported: return "unsupported";
    case CapabilityState::ReadOnly: return "read_only";
    case CapabilityState::Supported: return "supported";
    case CapabilityState::Experimental: return "experimental";
    default: return "unknown";
    }
}

} // namespace mouse_engine::model
