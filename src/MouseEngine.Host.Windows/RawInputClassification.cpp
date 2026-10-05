#include "RawInputClassification.h"

namespace mouse_engine::windows {

namespace {
constexpr std::uint16_t kMouseWheel = 0x0400;
constexpr std::uint16_t kMouseHWheel = 0x0800;
constexpr std::uint16_t kButtonMask =
    0x0001 | 0x0002 | 0x0004 | 0x0008 | 0x0010 | 0x0020 | 0x0040 | 0x0080;
}

bool raw_mouse_has_button_event(std::uint16_t button_flags) {
    return (button_flags & kButtonMask) != 0;
}

bool raw_mouse_has_wheel_event(std::uint16_t button_flags) {
    return (button_flags & (kMouseWheel | kMouseHWheel)) != 0;
}

bool raw_mouse_has_movement(std::int32_t last_x, std::int32_t last_y) {
    return last_x != 0 || last_y != 0;
}

RawInputPacketClass classify_raw_mouse_packet(
    std::uint16_t button_flags,
    std::int32_t last_x,
    std::int32_t last_y,
    std::int32_t wheel_delta,
    std::int32_t horizontal_wheel_delta) {
    if (raw_mouse_has_wheel_event(button_flags) ||
        wheel_delta != 0 ||
        horizontal_wheel_delta != 0) {
        return RawInputPacketClass::Wheel;
    }
    if (raw_mouse_has_button_event(button_flags)) {
        return RawInputPacketClass::Button;
    }
    if (raw_mouse_has_movement(last_x, last_y)) {
        return RawInputPacketClass::Movement;
    }
    return RawInputPacketClass::Other;
}

} // namespace mouse_engine::windows
