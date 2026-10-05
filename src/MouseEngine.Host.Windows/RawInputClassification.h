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
    std::uint16_t button_flags,
    std::int32_t last_x,
    std::int32_t last_y,
    std::int32_t wheel_delta,
    std::int32_t horizontal_wheel_delta);

bool raw_mouse_has_button_event(std::uint16_t button_flags);
bool raw_mouse_has_wheel_event(std::uint16_t button_flags);
bool raw_mouse_has_movement(std::int32_t last_x, std::int32_t last_y);

} // namespace mouse_engine::windows
