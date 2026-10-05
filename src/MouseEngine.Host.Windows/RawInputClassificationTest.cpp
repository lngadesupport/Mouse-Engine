#include "RawInputClassification.h"

#include <iostream>

namespace {

int fail(const char* message) {
    std::cerr << "RawInputClassification test FAIL: " << message << "\n";
    return 1;
}

int check(mouse_engine::windows::RawInputPacketClass actual,
          mouse_engine::windows::RawInputPacketClass expected,
          const char* message) {
    if (actual != expected) return fail(message);
    return 0;
}

} // namespace

int main() {
    using mouse_engine::windows::RawInputPacketClass;

    if (check(classify_raw_mouse_packet(0, 0, 0, 0, 0),
              RawInputPacketClass::Other,
              "empty packet must be other")) return 1;
    if (check(classify_raw_mouse_packet(0, 5, -2, 0, 0),
              RawInputPacketClass::Movement,
              "relative motion must be movement")) return 1;
    if (check(classify_raw_mouse_packet(0x01, 0, 0, 0, 0),
              RawInputPacketClass::Button,
              "button down must be button")) return 1;
    if (check(classify_raw_mouse_packet(0x02, 0, 0, 0, 0),
              RawInputPacketClass::Button,
              "button up must be button")) return 1;
    if (check(classify_raw_mouse_packet(0x0400, 0, 0, 120, 0),
              RawInputPacketClass::Wheel,
              "vertical wheel must be wheel")) return 1;
    if (check(classify_raw_mouse_packet(0x0800, 0, 0, 0, 120),
              RawInputPacketClass::Wheel,
              "horizontal wheel must be wheel")) return 1;
    if (raw_mouse_has_button_event(0x01) != true) return fail("button predicate");
    if (raw_mouse_has_wheel_event(0x0400) != true) return fail("wheel predicate");
    if (raw_mouse_has_movement(1, 0) != true) return fail("movement predicate");

    std::cout << "RawInputClassification test PASS\n";
    return 0;
}
