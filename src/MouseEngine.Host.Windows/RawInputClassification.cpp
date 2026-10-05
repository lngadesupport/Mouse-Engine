#include "RawInputClassification.h"

namespace mouse_engine::windows {

RawInputPacketClass classify_raw_mouse_packet(
    std::uint16_t flags,
    std::int32_t last_x,
    std::int32_t last_y,
    std::uint16_t button_flags,
    std::uint16_t button_data,
    std::int32_t wheel_delta,
    std::int32_t horizontal_wheel_delta) {
    constexpr std::uint16_t kMouseMoveAbsolute = 0x01;
    constexpr std::uint16_t kButtonFlagsMask = 0xFFu;

    (void)flags;
    (void)button_data;
    (void)kMouseMoveAbsolute;
    (void)kButtonFlagsMask;

    if (button_flags != 0) return RawInputPacketClass::Button;
    if (wheel_delta != 0 || horizontal_wheel_delta != 0) return RawInputPacketClass::Wheel;
    if (last_x != 0 || last_y != 0) return RawInputPacketClass::Movement;
    return RawInputPacketClass::Other;
}

} // namespace mouse_engine::windows
