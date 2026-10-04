#pragma once
#include "Capability.h"
#include "DeviceIdentity.h"
#include <string>
#include <vector>

namespace mouse_engine::core {

enum class AdapterPermission {
    Identify,
    QueryCapabilities,
    ReadState,
    WriteConfiguration,
    ProfileRead,
    ProfileWrite,
    Diagnostics,
    FirmwareInfo
};

struct AdapterManifest {
    std::string id;
    std::string version;
    std::vector<AdapterPermission> permissions;
    bool firmware_flashing{false}; // prohibited by the v1 contract
};

struct AdapterContext {
    DeviceIdentity identity;
    std::vector<Capability> capabilities;
};

class ProtocolAdapter {
public:
    virtual ~ProtocolAdapter() = default;
    virtual const AdapterManifest& manifest() const noexcept = 0;
    virtual bool can(AdapterPermission permission) const noexcept = 0;
};

} // namespace mouse_engine::core
