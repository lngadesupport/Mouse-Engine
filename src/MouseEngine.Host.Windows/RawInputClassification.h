#pragma once

#include <cstdint>

namespace mouse_engine::windows {

enum class RawInputPacketClass {
    Other,
    Movement,
    Button,
    Wheel
};

RawInputPacketClass classify_raw_mouse_packet(
    std::uint16_t flags,
    std::int32_t last_x,
    std::int32_t last_y,
    std::uint16_t button_flags,
    std::uint16_t button_data,
    std::int32_t wheel_delta,
    std::int32_t horizontal_wheel_delta);

} // namespace mouse_engine::windows
