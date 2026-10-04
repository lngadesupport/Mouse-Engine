#pragma once

#include <cstdint>
#include <string>
#include <optional>

namespace mouse_engine::core {

enum class Transport {
    Unknown,
    Usb,
    Wireless24GHz,
    Bluetooth
};

struct DeviceFingerprint {
    std::uint16_t vendor_id{};
    std::uint16_t product_id{};

    std::string manufacturer;
    std::string product;
    std::string serial_number;

    std::string hid_report_descriptor_hash;
    std::string protocol_id;

    std::optional<std::string> firmware_version;
    std::optional<std::string> hardware_revision;
    std::optional<std::string> receiver_revision;

    Transport transport{Transport::Unknown};

    // VID/PID alone is intentionally insufficient for exact model identity.
    bool has_identity_evidence() const noexcept {
        return vendor_id != 0 &&
               product_id != 0 &&
               !hid_report_descriptor_hash.empty();
    }
};

} // namespace mouse_engine::core
