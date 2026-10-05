#include <windows.h>
#include "InputTiming.h"
#include "RawInputClassification.h"
#include "MouseEngine/Workspace.h"
#include "MouseEngine/WorkspaceStore.h"
#include "MouseEngine/ObservationSession.h"
#include "MouseEngine/SessionCapture.h"
#include "MouseEngine/SessionStore.h"
#include "MouseEngine/SessionTraceStore.h"
#include "MouseEngine/SessionTimeline.h"
#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <limits>
#include <optional>
#include <setupapi.h>
#include <devpkey.h>
#include <initguid.h>
#include <hidclass.h>
#include <usbiodef.h>
#include <usbioctl.h>
#include <cfgmgr32.h>
#include <shlobj.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <windowsx.h>

#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
#include <wrl.h>
#include <WebView2.h>
#endif

namespace {

constexpr UINT_PTR kSnapshotTimerId = 1;

std::filesystem::path executable_directory() { wchar_t buffer[MAX_PATH]{}; DWORD n=GetModuleFileNameW(nullptr,buffer,MAX_PATH); return n ? std::filesystem::path(buffer).parent_path() : std::filesystem::path{}; }
std::filesystem::path ui_entrypoint() { return executable_directory() / L"ui" / L"index.html"; }
void show_native_fallback(HWND hwnd, const wchar_t* reason) { SetWindowTextW(hwnd,L"Mouse Engine — UI fallback"); MessageBoxW(hwnd,reason,L"Mouse Engine",MB_OK|MB_ICONINFORMATION); }
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
using Microsoft::WRL::ComPtr;
#endif

std::filesystem::path local_app_data() {
    PWSTR raw = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &raw))) {
        std::filesystem::path result(raw);
        CoTaskMemFree(raw);
        return result;
    }
    return std::filesystem::temp_directory_path();
}

std::filesystem::path documents_root() {
    PWSTR raw = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, &raw))) {
        std::filesystem::path result(raw);
        CoTaskMemFree(raw);
        return result;
    }
    return local_app_data();
}

std::filesystem::path workspace_root() {
    return documents_root() / mouse_engine::workspace::kWorkspaceDirectoryName;
}

std::filesystem::path cache_root() {
    return local_app_data() / L"Mouse Engine";
}

std::filesystem::path storage_root() {
    return workspace_root();
}

std::filesystem::path install_root() {
    return local_app_data() / L"Programs" / L"Mouse Engine";
}

std::string wide_to_utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string out(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), out.data(), size, nullptr, nullptr);
    return out;
}

std::string json_escape(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string bool_json(bool value) {
    return value ? "true" : "false";
}

std::string environment_json() {
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
    constexpr bool sdk = true;
#else
    constexpr bool sdk = false;
#endif
    const bool mutation_allowed = false;
    const bool firmware_flashing = false;
    std::ostringstream out;
    out << "{\n"
        << "  \"platform\": \"windows\",\n"
        << "  \"architecture\": \"x64\",\n"
        << "  \"webview2SdkEnabled\": " << bool_json(sdk) << ",\n"
        << "  \"mutationDefaultAllowed\": " << bool_json(mutation_allowed) << ",\n"
        << "  \"firmwareFlashingEnabled\": " << bool_json(firmware_flashing) << "\n"
        << "}";
    return out.str();
}

enum class DirectTransport {
    Unknown,
    Usb,
    Bluetooth
};

const char* direct_transport_json(DirectTransport transport) {
    switch (transport) {
    case DirectTransport::Usb: return "Usb";
    case DirectTransport::Bluetooth: return "Bluetooth";
    default: return "Unknown";
    }
}

struct BusTopologyEvidence {
    DirectTransport transport{DirectTransport::Unknown};
    std::vector<std::wstring> ancestors;
    std::vector<DEVINST> ancestor_devinsts;
    std::wstring location_info;
    std::wstring device_location_info;
};

bool starts_with_ci(const std::wstring& value, const wchar_t* prefix) {
    const std::wstring wanted(prefix);
    return value.size() >= wanted.size() &&
        CompareStringOrdinal(value.data(), static_cast<int>(wanted.size()),
                             wanted.data(), static_cast<int>(wanted.size()), TRUE) == CSTR_EQUAL;
}

BusTopologyEvidence inspect_bus_topology(DEVINST devinst) {
    BusTopologyEvidence evidence;
    DEVINST current = devinst;

    wchar_t device_location[MAX_DEVICE_ID_LEN]{};
    ULONG device_location_size = ARRAYSIZE(device_location);
    ULONG device_location_type = 0;
    if (CM_Get_DevNode_Registry_PropertyW(
            devinst, CM_DRP_LOCATION_INFORMATION, &device_location_type,
            device_location, &device_location_size, 0) == CR_SUCCESS &&
        device_location[0] != L'\0') {
        evidence.device_location_info = device_location;
    }

    for (unsigned depth = 0; depth < 32; ++depth) {
        DEVINST parent = 0;
        if (CM_Get_Parent(&parent, current, 0) != CR_SUCCESS) break;

        evidence.ancestor_devinsts.push_back(parent);

        wchar_t id_buffer[MAX_DEVICE_ID_LEN]{};
        if (CM_Get_Device_IDW(parent, id_buffer, ARRAYSIZE(id_buffer), 0) == CR_SUCCESS) {
            const std::wstring id(id_buffer);
            evidence.ancestors.push_back(id);
            if (starts_with_ci(id, LR"(USB\)") && evidence.transport == DirectTransport::Unknown) {
                evidence.transport = DirectTransport::Usb;
            }
            if ((starts_with_ci(id, LR"(BTH\)") ||
                 starts_with_ci(id, LR"(BTHENUM\)") ||
                 starts_with_ci(id, LR"(BTHLEDEVICE\)")) &&
                evidence.transport == DirectTransport::Unknown) {
                evidence.transport = DirectTransport::Bluetooth;
            }
        }

        wchar_t location[MAX_DEVICE_ID_LEN]{};
        ULONG location_size = ARRAYSIZE(location);
        ULONG property_type = 0;
        if (CM_Get_DevNode_Registry_PropertyW(
                parent, CM_DRP_LOCATION_INFORMATION, &property_type,
                location, &location_size, 0) == CR_SUCCESS &&
            location[0] != L'\0' &&
            evidence.location_info.empty()) {
            evidence.location_info = location;
        }

        current = parent;
    }
    return evidence;
}

std::uint64_t topology_hash(const BusTopologyEvidence& evidence) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto add = [&hash](wchar_t value) {
        hash ^= static_cast<std::uint64_t>(value);
        hash *= 1099511628211ULL;
    };
    for (const auto& ancestor : evidence.ancestors) {
        for (const wchar_t c : ancestor) add(c);
        add(L'|');
    }
    for (const wchar_t c : evidence.location_info) add(c);
    add(static_cast<wchar_t>(evidence.transport));
    return hash;
}

struct UsbEndpointEvidence {
    bool available{false};
    ULONG connection_index{0};
    UCHAR speed{0};
    USHORT device_address{0};
    ULONG interrupt_in_endpoint_count{0};
    bool descriptor_interval_available{false};
    UCHAR descriptor_interval{0};
    UCHAR descriptor_endpoint_address{0};
};

std::optional<ULONG> parse_usb_port_index(const std::wstring& location_info) {
    constexpr std::wstring_view marker = L"Port_#";
    const auto position = location_info.find(marker);
    if (position == std::wstring::npos) return std::nullopt;

    std::size_t cursor = position + marker.size();
    if (cursor == location_info.size() || !std::iswdigit(location_info[cursor])) return std::nullopt;

    ULONG value = 0;
    for (; cursor < location_info.size() && std::iswdigit(location_info[cursor]); ++cursor) {
        const ULONG digit = static_cast<ULONG>(location_info[cursor] - L'0');
        if (value > (std::numeric_limits<ULONG>::max() - digit) / 10) return std::nullopt;
        value = value * 10 + digit;
    }
    if (value == 0) return std::nullopt;
    return value;
}

bool is_ancestor_devinst(const std::vector<DEVINST>& ancestors, DEVINST candidate) {
    return std::find(ancestors.begin(), ancestors.end(), candidate) != ancestors.end();
}

UsbEndpointEvidence inspect_usb_endpoint_evidence(const BusTopologyEvidence& topology) {
    UsbEndpointEvidence evidence;
    const auto port = parse_usb_port_index(topology.device_location_info);
    if (!port.has_value() || topology.ancestor_devinsts.empty()) return evidence;

    HDEVINFO set = SetupDiGetClassDevsW(
        &GUID_DEVINTERFACE_USB_HUB,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE) return evidence;

    SP_DEVICE_INTERFACE_DATA interface_data{};
    interface_data.cbSize = sizeof(interface_data);

    constexpr ULONG kPipeCapacity = 64;
    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(
             set, nullptr, &GUID_DEVINTERFACE_USB_HUB, index, &interface_data); ++index) {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(
            set, &interface_data, nullptr, 0, &required, nullptr);
        if (required == 0) continue;

        std::vector<BYTE> detail_buffer(required + sizeof(wchar_t));
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detail_buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        SP_DEVINFO_DATA hub_data{};
        hub_data.cbSize = sizeof(hub_data);
        if (!SetupDiGetDeviceInterfaceDetailW(
                set, &interface_data, detail,
                static_cast<DWORD>(detail_buffer.size()),
                nullptr, &hub_data)) {
            continue;
        }

        if (!is_ancestor_devinst(topology.ancestor_devinsts, hub_data.DevInst)) continue;

        HANDLE hub = CreateFileW(
            detail->DevicePath,
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (hub == INVALID_HANDLE_VALUE) continue;

        std::vector<BYTE> buffer(
            sizeof(USB_NODE_CONNECTION_INFORMATION_EX) +
            static_cast<std::size_t>(kPipeCapacity) * sizeof(USB_PIPE_INFO));
        auto* connection = reinterpret_cast<USB_NODE_CONNECTION_INFORMATION_EX*>(buffer.data());
        connection->ConnectionIndex = *port;

        DWORD returned = 0;
        const BOOL ok = DeviceIoControl(
            hub,
            IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
            connection,
            sizeof(USB_NODE_CONNECTION_INFORMATION_EX),
            connection,
            static_cast<DWORD>(buffer.size()),
            &returned,
            nullptr);
        CloseHandle(hub);

        if (!ok) continue;

        evidence.available = true;
        evidence.connection_index = connection->ConnectionIndex;
        evidence.speed = connection->Speed;
        evidence.device_address = connection->DeviceAddress;

        const ULONG pipe_count = (std::min)(connection->NumberOfOpenPipes, kPipeCapacity);
        for (ULONG pipe_index = 0; pipe_index < pipe_count; ++pipe_index) {
            const auto& endpoint = connection->PipeList[pipe_index].EndpointDescriptor;
            const bool interrupt = (endpoint.bmAttributes & 0x03u) == 0x03u;
            const bool input = (endpoint.bEndpointAddress & 0x80u) != 0;
            if (!interrupt || !input) continue;

            ++evidence.interrupt_in_endpoint_count;
            if (!evidence.descriptor_interval_available) {
                evidence.descriptor_interval_available = true;
                evidence.descriptor_interval = endpoint.bInterval;
                evidence.descriptor_endpoint_address = endpoint.bEndpointAddress;
            }
        }
        break;
    }

    SetupDiDestroyDeviceInfoList(set);
    return evidence;
}

struct ObservedInputStream {
    static constexpr double kIdleGapThresholdMs = mouse_engine::model::kDefaultIdleGapThresholdMs;

    std::size_t packet_count{0};
    mouse_engine::windows::InputTimingSummary timing{};
    std::size_t active_run_count{0};
    std::size_t current_run_packets{0};
    std::size_t longest_active_run_packets{0};
    double current_run_start_ms{0.0};
    double last_timestamp_ms{0.0};
    double longest_active_run_ms{0.0};
    std::uint64_t last_timestamp_ticks{0};

    void record(std::uint64_t timestamp_ticks, std::uint64_t frequency_ticks) {
        if (frequency_ticks == 0) return;
        const double timestamp_ms =
            static_cast<double>(timestamp_ticks) * 1000.0 /
            static_cast<double>(frequency_ticks);

        if (packet_count == 0) {
            ++active_run_count;
            current_run_packets = 1;
            current_run_start_ms = timestamp_ms;
            last_timestamp_ms = timestamp_ms;
            last_timestamp_ticks = timestamp_ticks;
            ++packet_count;
            longest_active_run_packets = 1;
            longest_active_run_ms = 0.0;
            return;
        }

        if (timestamp_ticks <= last_timestamp_ticks) return;

        const double gap_ms = timestamp_ms - last_timestamp_ms;
        if (gap_ms >= kIdleGapThresholdMs) {
            ++active_run_count;
            current_run_packets = 1;
            current_run_start_ms = timestamp_ms;
        } else {
            ++current_run_packets;
        }

        last_timestamp_ms = timestamp_ms;
        last_timestamp_ticks = timestamp_ticks;
        ++packet_count;
        longest_active_run_packets =
            (std::max)(longest_active_run_packets, current_run_packets);
        longest_active_run_ms =
            (std::max)(longest_active_run_ms, timestamp_ms - current_run_start_ms);
    }

    double current_run_duration_ms() const {
        if (current_run_packets == 0) return 0.0;
        return (std::max)(0.0, last_timestamp_ms - current_run_start_ms);
    }
};

struct ObservedInputStreams {
    ObservedInputStream all{};
    ObservedInputStream movement{};
    ObservedInputStream button{};
    ObservedInputStream wheel{};
};

struct ActiveSessionEvidence {
    bool active{false};
    std::string id;
    std::string device_id;
    std::string started_at_utc;
    std::size_t packet_count{0};
    double duration_ms{0.0};
    bool trace_available{false};
    std::size_t trace_packet_count{0};
    mouse_engine::model::TimingMeasurement timing{};
    mouse_engine::model::TimingDistribution distribution{};
};

struct ObservedInputEvidence {
    bool available{false};
    ObservedInputStreams streams{};
    ActiveSessionEvidence session{};
};

struct WindowsMouseIdentity {
    bool resolved{false};
    std::wstring interface_path;
    std::wstring instance_id;
    std::wstring container_id;
    std::wstring manufacturer;
    std::wstring product;
    std::wstring vid;
    std::wstring pid;
    DirectTransport transport{DirectTransport::Unknown};
    std::uint64_t topology_hash{0};
    UsbEndpointEvidence usb_endpoint{};
    ObservedInputEvidence observed_input{};
};

std::wstring first_multi_sz(const std::vector<wchar_t>& buffer) {
    if (buffer.empty() || buffer[0] == L'\0') return {};
    return std::wstring(buffer.data());
}

std::wstring get_registry_property(HDEVINFO set, SP_DEVINFO_DATA& data, DWORD property) {
    DWORD required = 0;
    SetupDiGetDeviceRegistryPropertyW(set, &data, property, nullptr, nullptr, 0, &required);
    if (required == 0) return {};
    std::vector<wchar_t> buffer(required / sizeof(wchar_t) + 2);
    if (!SetupDiGetDeviceRegistryPropertyW(
            set, &data, property, nullptr,
            reinterpret_cast<PBYTE>(buffer.data()),
            static_cast<DWORD>(buffer.size() * sizeof(wchar_t)),
            nullptr)) {
        return {};
    }
    return first_multi_sz(buffer);
}

std::wstring get_container_id(HDEVINFO set, SP_DEVINFO_DATA& data) {
    DEVPROPTYPE type = 0;
    DWORD required = 0;
    SetupDiGetDevicePropertyW(
        set, &data, &DEVPKEY_Device_ContainerId, &type,
        nullptr, 0, &required, 0);
    if (required == 0) return {};

    std::vector<BYTE> buffer(required);
    if (!SetupDiGetDevicePropertyW(
            set, &data, &DEVPKEY_Device_ContainerId, &type,
            buffer.data(), static_cast<DWORD>(buffer.size()),
            nullptr, 0) ||
        type != DEVPROP_TYPE_GUID ||
        buffer.size() < sizeof(GUID)) {
        return {};
    }

    wchar_t text[64]{};
    if (StringFromGUID2(*reinterpret_cast<const GUID*>(buffer.data()), text, ARRAYSIZE(text)) == 0) {
        return {};
    }
    return text;
}

std::wstring get_instance_id(HDEVINFO set, SP_DEVINFO_DATA& data) {
    DWORD required = 0;
    SetupDiGetDeviceInstanceIdW(set, &data, nullptr, 0, &required);
    if (required == 0) return {};
    std::vector<wchar_t> buffer(required + 1);
    if (!SetupDiGetDeviceInstanceIdW(set, &data, buffer.data(), static_cast<DWORD>(buffer.size()), nullptr)) {
        return {};
    }
    return buffer.data();
}

std::wstring extract_hardware_token(const std::wstring& hardware_id, const wchar_t* prefix) {
    const auto position = hardware_id.find(prefix);
    if (position == std::wstring::npos) return {};
    const auto start = position + 4;
    if (start + 4 > hardware_id.size()) return {};
    return hardware_id.substr(start, 4);
}

std::wstring raw_input_device_name(HANDLE device) {
    UINT required = 0;
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, nullptr, &required) == static_cast<UINT>(-1) || required == 0) {
        return {};
    }
    std::vector<wchar_t> buffer(required + 1);
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, buffer.data(), &required) == static_cast<UINT>(-1)) {
        return {};
    }
    return buffer.data();
}

bool path_equal_ci(const std::wstring& left, const std::wstring& right) {
    return CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

WindowsMouseIdentity resolve_setupapi_identity(const std::wstring& raw_device_path) {
    WindowsMouseIdentity identity;
    identity.interface_path = raw_device_path;

    HDEVINFO set = SetupDiGetClassDevsW(
        &GUID_DEVINTERFACE_HID,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE) return identity;

    SP_DEVICE_INTERFACE_DATA interface_data{};
    interface_data.cbSize = sizeof(interface_data);

    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(set, nullptr, &GUID_DEVINTERFACE_HID, index, &interface_data); ++index) {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(
            set, &interface_data, nullptr, 0, &required, nullptr);
        if (required == 0) continue;

        std::vector<BYTE> detail_buffer(required + sizeof(wchar_t));
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detail_buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        SP_DEVINFO_DATA device_data{};
        device_data.cbSize = sizeof(device_data);
        if (!SetupDiGetDeviceInterfaceDetailW(
                set, &interface_data, detail,
                static_cast<DWORD>(detail_buffer.size()),
                nullptr, &device_data)) {
            continue;
        }

        if (!path_equal_ci(detail->DevicePath, raw_device_path)) continue;

        identity.resolved = true;
        identity.instance_id = get_instance_id(set, device_data);
        identity.container_id = get_container_id(set, device_data);
        identity.manufacturer = get_registry_property(set, device_data, SPDRP_MFG);
        identity.product = get_registry_property(set, device_data, SPDRP_DEVICEDESC);
        const std::wstring hardware_id = get_registry_property(set, device_data, SPDRP_HARDWAREID);
        identity.vid = extract_hardware_token(hardware_id, L"VID_");
        identity.pid = extract_hardware_token(hardware_id, L"PID_");
        const BusTopologyEvidence topology = inspect_bus_topology(device_data.DevInst);
        identity.transport = topology.transport;
        identity.topology_hash = topology_hash(topology);
        if (identity.transport == DirectTransport::Usb) {
            identity.usb_endpoint = inspect_usb_endpoint_evidence(topology);
        }
        break;
    }

    SetupDiDestroyDeviceInfoList(set);
    return identity;
}

class RawInputTimingRegistry {
public:
    bool record(HRAWINPUT raw_input) {
        UINT size = 0;
        if (GetRawInputData(raw_input, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1) ||
            size < sizeof(RAWINPUTHEADER)) return false;

        std::vector<BYTE> buffer(size);
        if (GetRawInputData(raw_input, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) {
            return false;
        }

        const auto* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
        if (raw->header.dwType != RIM_TYPEMOUSE || raw->header.hDevice == nullptr) return false;

        const std::wstring path = raw_input_device_name(raw->header.hDevice);
        if (path.empty()) return false;

        LARGE_INTEGER now{};
        if (!QueryPerformanceCounter(&now)) return false;
        if (frequency_ticks_ == 0) {
            LARGE_INTEGER frequency{};
            if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) return false;
            frequency_ticks_ = static_cast<std::uint64_t>(frequency.QuadPart);
        }

        auto [it, inserted] = per_device_.try_emplace(path);
        (void)inserted;
        device_paths_[raw->header.hDevice] = path;

        auto& timing = it->second;
        const auto timestamp = static_cast<std::uint64_t>(now.QuadPart);
        ensure_session(path, timing, timestamp);

        const auto& mouse = raw->data.mouse;
        timing.all.accumulator.record(timestamp, frequency_ticks_);
        timing.all.record(timestamp, frequency_ticks_);

        unsigned int classes = 0u;
        if (raw_mouse_has_movement(mouse.lLastX, mouse.lLastY)) {
            timing.movement.accumulator.record(timestamp, frequency_ticks_);
            timing.movement.record(timestamp, frequency_ticks_);
            classes |= mouse_engine::observation::Movement;
        }
        if (raw_mouse_has_button_event(mouse.usButtonFlags)) {
            timing.button.accumulator.record(timestamp, frequency_ticks_);
            timing.button.record(timestamp, frequency_ticks_);
            classes |= mouse_engine::observation::Button;
        }
        if (raw_mouse_has_wheel_event(mouse.usButtonFlags)) {
            timing.wheel.accumulator.record(timestamp, frequency_ticks_);
            timing.wheel.record(timestamp, frequency_ticks_);
            classes |= mouse_engine::observation::Wheel;
        }

        if (timing.session && timing.session->is_recording() && timing.session_start_ticks != 0) {
            const double timestamp_ms =
                static_cast<double>(timestamp - timing.session_start_ticks) * 1000.0 /
                static_cast<double>(frequency_ticks_);
            const auto wheel_delta =
                raw_mouse_has_wheel_event(mouse.usButtonFlags)
                    ? static_cast<std::int16_t>(mouse.usButtonData)
                    : 0;
            const mouse_engine::session::TracePacket trace_packet{
                timestamp_ms,
                classes,
                static_cast<std::int32_t>(mouse.lLastX),
                static_cast<std::int32_t>(mouse.lLastY),
                static_cast<std::uint32_t>(mouse.usButtonFlags),
                static_cast<std::int32_t>(wheel_delta)
            };
            if (!timing.session->record({timestamp_ms, classes}, trace_packet)) {
                finalize_session(timing);
                ensure_session(path, timing, timestamp);
                if (timing.session && timing.session->is_recording() && timing.session_start_ticks != 0) {
                    timing.session->record(
                        {0.0, classes},
                        {0.0, classes,
                         static_cast<std::int32_t>(mouse.lLastX),
                         static_cast<std::int32_t>(mouse.lLastY),
                         static_cast<std::uint32_t>(mouse.usButtonFlags),
                         static_cast<std::int32_t>(wheel_delta)});
                }
            }
        }
        return true;
    }

    void remove_device(HRAWINPUT device) {
        if (!device) return;
        const auto it = device_paths_.find(device);
        if (it == device_paths_.end()) return;
        const auto timing = per_device_.find(it->second);
        if (timing != per_device_.end()) {
            finalize_session(timing->second);
            per_device_.erase(timing);
        }
        device_paths_.erase(it);
    }

    bool snapshot_for(const std::wstring& raw_path, ObservedInputEvidence& out) const {
        const auto it = per_device_.find(raw_path);
        if (it == per_device_.end()) return false;

        out = {};
        out.streams.all.packet_count = it->second.all.packet_count;
        out.streams.movement.packet_count = it->second.movement.packet_count;
        out.streams.button.packet_count = it->second.button.packet_count;
        out.streams.wheel.packet_count = it->second.wheel.packet_count;
        it->second.all.accumulator.snapshot(out.streams.all.timing);
        out.available = out.streams.all.packet_count > 0;
        it->second.movement.accumulator.snapshot(out.streams.movement.timing);
        it->second.button.accumulator.snapshot(out.streams.button.timing);
        it->second.wheel.accumulator.snapshot(out.streams.wheel.timing);

        if (it->second.session && it->second.session->is_recording()) {
            out.session.active = true;
            out.session.id = it->second.session->session_id();
            out.session.device_id = it->second.session->device_id();
            out.session.started_at_utc = it->second.session->started_at_utc();
            out.session.packet_count = it->second.session->packet_count();
            out.session.trace_available = it->second.session->trace_enabled();
            out.session.trace_packet_count = it->second.session->trace_packet_count();
            const auto live_session = it->second.session->snapshot({});
            out.session.timing = live_session.all.timing;
            out.session.distribution = live_session.all.distribution;

            LARGE_INTEGER now{};
            if (QueryPerformanceCounter(&now) && frequency_ticks_ > 0 && it->second.session_start_ticks != 0) {
                out.session.duration_ms = (std::max)(0.0,
                    static_cast<double>(static_cast<std::uint64_t>(now.QuadPart) - it->second.session_start_ticks) *
                    1000.0 / static_cast<double>(frequency_ticks_));
            }
        }
        return out.available;
    }

    bool any_available() const {
        ObservedInputEvidence evidence{};
        for (const auto& item : per_device_) {
            if (snapshot_for(item.first, evidence)) return true;
        }
        return false;
    }

    void clear() {
        for (auto& item : per_device_) finalize_session(item.second);
        for (auto it = pending_finalization_.begin(); it != pending_finalization_.end();) {
            std::string error;
            if ((*it)->stop(utc_now_iso8601(), nullptr, &error)) {
                it = pending_finalization_.erase(it);
            } else {
                std::cerr << "session: retry still pending: " << error << "\n";
                ++it;
            }
        }
        per_device_.clear();
        device_paths_.clear();
        frequency_ticks_ = 0;
    }

private:
    struct DeviceTiming {
        struct StreamTiming {
            std::size_t packet_count{0};
            mouse_engine::windows::InputTimingAccumulator accumulator;
        };

        StreamTiming all;
        StreamTiming movement;
        StreamTiming button;
        StreamTiming wheel;
        std::unique_ptr<mouse_engine::session::SessionCapture> session;
        std::uint64_t session_start_ticks{0};
    };

    static std::string utc_now_iso8601() {
        FILETIME file_time{};
        GetSystemTimePreciseAsFileTime(&file_time);
        SYSTEMTIME system_time{};
        if (!FileTimeToSystemTime(&file_time, &system_time)) return {};

        std::ostringstream out;
        out << std::setfill('0')
            << std::setw(4) << system_time.wYear << '-'
            << std::setw(2) << system_time.wMonth << '-'
            << std::setw(2) << system_time.wDay << 'T'
            << std::setw(2) << system_time.wHour << ':'
            << std::setw(2) << system_time.wMinute << ':'
            << std::setw(2) << system_time.wSecond << '.'
            << std::setw(3) << system_time.wMilliseconds << 'Z';
        return out.str();
    }

    static std::string session_device_id(const std::wstring& raw_path) {
        const WindowsMouseIdentity identity = resolve_setupapi_identity(raw_path);
        if (identity.resolved && !identity.instance_id.empty()) return wide_to_utf8(identity.instance_id);
        return wide_to_utf8(raw_path);
    }

    void ensure_session(const std::wstring& raw_path, DeviceTiming& timing, std::uint64_t timestamp_ticks) {
        if (timing.session && timing.session->is_recording()) return;

        const auto paths = mouse_engine::workspace::WorkspacePaths::from_root(workspace_root());
        timing.session = std::make_unique<mouse_engine::session::SessionCapture>(
            mouse_engine::session::SessionStore(paths), true);

        if (!timing.session->start(session_device_id(raw_path), utc_now_iso8601())) {
            timing.session.reset();
            timing.session_start_ticks = 0;
            return;
        }
        timing.session_start_ticks = timestamp_ticks;
    }

    void finalize_session(DeviceTiming& timing) {
        if (!timing.session || !timing.session->is_recording()) {
            timing.session.reset();
            timing.session_start_ticks = 0;
            return;
        }

        std::string error;
        if (timing.session->stop(utc_now_iso8601(), nullptr, &error)) {
            timing.session.reset();
            timing.session_start_ticks = 0;
        } else {
            std::cerr << "session: unable to persist observation session: " << error << "\n";
            pending_finalization_.push_back(std::move(timing.session));
            timing.session_start_ticks = 0;
        }
    }

    std::uint64_t frequency_ticks_{0};
    std::unordered_map<std::wstring, DeviceTiming> per_device_;
    std::unordered_map<HANDLE, std::wstring> device_paths_;
    std::vector<std::unique_ptr<mouse_engine::session::SessionCapture>> pending_finalization_;
};

RawInputTimingRegistry& raw_input_timing() {
    static RawInputTimingRegistry registry;
    return registry;
}

struct RawMouseObservation {
    bool available{false};
    bool detail_available{false};
    UINT mouse_count{0};
    DWORD max_buttons{0};
    DWORD max_sample_rate{0};
    std::vector<WindowsMouseIdentity> identities;
};

RawMouseObservation observe_raw_mice() {
    RawMouseObservation observation;

    for (int attempt = 0; attempt < 3; ++attempt) {
        UINT device_count = 0;
        if (GetRawInputDeviceList(nullptr, &device_count, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1)) {
            return observation;
        }

        observation.available = true;
        if (device_count == 0) return observation;

        std::vector<RAWINPUTDEVICELIST> devices(device_count);
        UINT capacity = device_count;
        const UINT result = GetRawInputDeviceList(
            devices.data(),
            &capacity,
            sizeof(RAWINPUTDEVICELIST));
        if (result == static_cast<UINT>(-1)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) continue;
            observation.available = false;
            return observation;
        }

        for (UINT index = 0; index < result; ++index) {
            if (devices[index].dwType != RIM_TYPEMOUSE) continue;

            ++observation.mouse_count;
            const std::wstring raw_path = raw_input_device_name(devices[index].hDevice);
            if (!raw_path.empty()) {
                observation.identities.push_back(resolve_setupapi_identity(raw_path));
            }
            RID_DEVICE_INFO info{};
            info.cbSize = sizeof(info);
            UINT info_size = sizeof(info);
            if (GetRawInputDeviceInfoW(
                    devices[index].hDevice,
                    RIDI_DEVICEINFO,
                    &info,
                    &info_size) != static_cast<UINT>(-1) &&
                info.dwType == RIM_TYPEMOUSE) {
                observation.detail_available = true;
                observation.max_buttons = (std::max)(observation.max_buttons, info.mouse.dwNumberOfButtons);
                observation.max_sample_rate = (std::max)(observation.max_sample_rate, info.mouse.dwSampleRate);
            }
        }
        return observation;
    }

    observation.available = false;
    return observation;
}

std::string wide_json_escape(const std::wstring& value) {
    return json_escape(wide_to_utf8(value));
}

std::string timing_summary_json(
    const mouse_engine::windows::InputTimingSummary& summary) {
    std::ostringstream out;
    out << "{\"intervalCount\":" << summary.interval_count
        << ",\"minIntervalMs\":" << summary.min_interval_ms
        << ",\"medianIntervalMs\":" << summary.median_interval_ms
        << ",\"p95IntervalMs\":" << summary.p95_interval_ms
        << ",\"maxIntervalMs\":" << summary.max_interval_ms
        << ",\"jitterP95MinusMedianMs\":" << summary.jitter_p95_minus_median_ms
        << ",\"idleGapCount50ms\":" << summary.idle_gap_count_50ms
        << ",\"longestIdleGapMs\":" << summary.longest_idle_gap_ms
        << "}";
    return out.str();
}

std::string timing_stream_json(const ObservedInputStream& stream) {
    std::ostringstream out;
    out << "{\"packetCount\":" << stream.packet_count
        << ",\"timing\":" << timing_summary_json(stream.timing)
        << ",\"activity\":{\"activeRunCount\":" << stream.active_run_count
        << ",\"longestActiveRunPackets\":" << stream.longest_active_run_packets
        << ",\"longestActiveRunMs\":" << stream.longest_active_run_ms
        << ",\"currentRunPackets\":" << stream.current_run_packets
        << ",\"currentRunDurationMs\":" << stream.current_run_duration_ms()
        << ",\"idleGapThresholdMs\":" << ObservedInputStream::kIdleGapThresholdMs
        << "}}";
    return out.str();
}

std::string timing_distribution_json(const mouse_engine::model::TimingDistribution& distribution) {
    std::ostringstream out;
    out << "{\"sampleCount\":" << distribution.sample_count
        << ",\"meanIntervalMs\":" << distribution.mean_interval_ms
        << ",\"bucketWidthMs\":" << distribution.bucket_width_ms
        << ",\"buckets\":[";
    for (std::size_t i = 0; i < distribution.buckets.size(); ++i) {
        if (i != 0) out << ",";
        const auto& bucket = distribution.buckets[i];
        out << "{\"lowerBoundMs\":" << bucket.lower_bound_ms
            << ",\"upperBoundMs\":" << bucket.upper_bound_ms
            << ",\"count\":" << bucket.count
            << ",\"cumulativeFraction\":" << bucket.cumulative_fraction << "}";
    }
    out << "]}";
    return out.str();
}

std::string session_history_json() {
    const auto paths = mouse_engine::workspace::WorkspacePaths::from_root(workspace_root());
    mouse_engine::session::SessionStore store(paths);
    std::string error;
    const auto sessions = store.list(&error);

    std::ostringstream out;
    out << "{\"available\":" << bool_json(error.empty())
        << ",\"items\":[";
    for (std::size_t i = 0; i < sessions.size(); ++i) {
        if (i != 0) out << ",";
        const auto& session = sessions[i];
        out << "{\"id\":\"" << json_escape(session.id)
            << "\",\"deviceId\":\"" << json_escape(session.device_id)
            << "\",\"startedAtUtc\":\"" << json_escape(session.started_at_utc)
            << "\",\"endedAtUtc\":\"" << json_escape(session.ended_at_utc)
            << "\",\"packetCount\":" << session.packet_count
            << ",\"intervalCount\":" << session.interval_count
            << ",\"medianIntervalMs\":" << session.median_interval_ms
            << ",\"p95IntervalMs\":" << session.p95_interval_ms
            << ",\"jitterP95MinusMedianMs\":" << session.jitter_p95_minus_median_ms
            << ",\"idleGapCount50ms\":" << session.idle_gap_count_50ms
            << ",\"activeRunCount\":" << session.active_run_count
            << ",\"longestActiveRunPackets\":" << session.longest_active_run_packets
            << ",\"traceAvailable\":" << bool_json(session.trace_available)
            << ",\"tracePacketCount\":" << session.trace_packet_count
            << ",\"distribution\":{\"sampleCount\":" << session.distribution_sample_count
            << ",\"meanIntervalMs\":" << session.distribution_mean_interval_ms
            << ",\"bucketWidthMs\":" << session.distribution_bucket_width_ms
            << ",\"buckets\":[";
        for (std::size_t bucket_index = 0; bucket_index < session.distribution_buckets.size(); ++bucket_index) {
            if (bucket_index != 0) out << ",";
            const auto& bucket = session.distribution_buckets[bucket_index];
            out << "{\"lowerBoundMs\":" << bucket.lower_bound_ms
                << ",\"upperBoundMs\":" << bucket.upper_bound_ms
                << ",\"count\":" << bucket.count
                << ",\"cumulativeFraction\":" << bucket.cumulative_fraction << "}";
        }
        out << "]},\"anomalies\":[";
        for (std::size_t anomaly_index = 0; anomaly_index < session.anomalies.size(); ++anomaly_index) {
            if (anomaly_index != 0) out << ",";
            const auto& anomaly = session.anomalies[anomaly_index];
            out << "{\"id\":\"" << json_escape(anomaly.id)
                << "\",\"severity\":\"" << json_escape(anomaly.severity)
                << "\",\"type\":\"" << json_escape(anomaly.type)
                << "\",\"message\":\"" << json_escape(anomaly.message)
                << "\",\"stream\":\"" << json_escape(anomaly.stream)
                << "\",\"packetIndex\":" << anomaly.packet_index
                << ",\"timestampMs\":" << anomaly.timestamp_ms << "}";
        }
        out << "],\"complete\":" << bool_json(session.complete) << "}";
    }
    out << "]}";
    return out.str();
}

const char* timeline_event_kind_json(mouse_engine::timeline::TimelineEventKind kind) {
    switch (kind) {
    case mouse_engine::timeline::TimelineEventKind::SessionStart: return "sessionStart";
    case mouse_engine::timeline::TimelineEventKind::Packet: return "packet";
    case mouse_engine::timeline::TimelineEventKind::IdleGap: return "idleGap";
    case mouse_engine::timeline::TimelineEventKind::Anomaly: return "anomaly";
    case mouse_engine::timeline::TimelineEventKind::SessionEnd: return "sessionEnd";
    default: return "unknown";
    }
}

std::string session_trace_json(const std::string& session_id) {
    const auto paths = mouse_engine::workspace::WorkspacePaths::from_root(workspace_root());
    mouse_engine::session::SessionTraceStore trace_store(paths);
    mouse_engine::session::SessionStore session_store(paths);
    mouse_engine::session::SessionTrace trace;
    std::string error;
    const bool available = trace_store.load(session_id, &trace, &error);
    std::ostringstream out;
    out << "{\"type\":\"sessionTrace\",\"sessionId\":\"" << json_escape(session_id)
        << "\",\"available\":" << bool_json(available);
    if (!available) {
        out << ",\"error\":\"" << json_escape(error) << "\"}";
        return out.str();
    }

    std::vector<mouse_engine::model::ObservationAnomaly> anomalies;
    const auto summaries = session_store.list();
    const auto summary_it = std::find_if(summaries.begin(), summaries.end(),
        [&session_id](const auto& summary) { return summary.id == session_id; });
    if (summary_it != summaries.end()) {
        anomalies.reserve(summary_it->anomalies.size());
        for (const auto& persisted : summary_it->anomalies) {
            mouse_engine::model::ObservationAnomaly anomaly;
            anomaly.id = persisted.id;
            anomaly.severity = persisted.severity;
            anomaly.type = persisted.type;
            anomaly.message = persisted.message;
            anomaly.stream = persisted.stream;
            anomaly.packet_index = persisted.packet_index;
            anomaly.timestamp_ms = persisted.timestamp_ms;
            anomalies.push_back(std::move(anomaly));
        }
    }

    const auto timeline = mouse_engine::timeline::build_timeline(trace, anomalies);
    out << ",\"schemaVersion\":" << trace.schema_version
        << ",\"deviceId\":\"" << json_escape(trace.device_id)
        << "\",\"truncated\":" << bool_json(trace.truncated)
        << ",\"packets\":[";
    for (std::size_t i = 0; i < trace.packets.size(); ++i) {
        if (i != 0) out << ",";
        const auto& packet = trace.packets[i];
        out << "{\"timestampMs\":" << packet.timestamp_ms
            << ",\"classes\":" << packet.classes
            << ",\"dx\":" << packet.dx
            << ",\"dy\":" << packet.dy
            << ",\"buttons\":" << packet.buttons
            << ",\"wheel\":" << packet.wheel << "}";
    }
    out << "],\"timelineAvailable\":" << bool_json(timeline.available)
        << ",\"timeline\":[";
    for (std::size_t i = 0; i < timeline.events.size(); ++i) {
        if (i != 0) out << ",";
        const auto& event = timeline.events[i];
        out << "{\"offsetMs\":" << event.offset_ms
            << ",\"kind\":\"" << timeline_event_kind_json(event.kind)
            << "\",\"packetIndex\":" << event.packet_index
            << ",\"stream\":\"" << json_escape(event.stream)
            << "\",\"severity\":\"" << json_escape(event.severity)
            << "\",\"type\":\"" << json_escape(event.type)
            << "\",\"message\":\"" << json_escape(event.message) << "\"}";
    }
    out << "]}";
    return out.str();
}

std::string snapshot_json() {
    RawMouseObservation mouse = observe_raw_mice();
    for (auto& identity : mouse.identities) {
        identity.observed_input.available =
            raw_input_timing().snapshot_for(identity.interface_path, identity.observed_input);
    }
    std::ostringstream out;
    out << "{\n"
        << "  \"schemaVersion\": 2,\n"
        << "  \"host\": { \"online\": true, \"platform\": \"windows\", \"architecture\": \"x64\" },\n"
        << "  \"device\": { \"observationAvailable\": " << bool_json(mouse.available)
        << ", \"connected\": " << bool_json(mouse.available && mouse.mouse_count > 0)
        << ", \"mouseCount\": " << mouse.mouse_count
        << ", \"detailAvailable\": " << bool_json(mouse.detail_available)
        << ", \"maxObservedButtons\": " << mouse.max_buttons
        << ", \"maxObservedSampleRate\": " << mouse.max_sample_rate
        << ", \"identityResolvedCount\": " << std::count_if(mouse.identities.begin(), mouse.identities.end(), [](const auto& item) { return item.resolved; })
        << ", \"identities\": [";
    for (std::size_t i = 0; i < mouse.identities.size(); ++i) {
        const auto& identity = mouse.identities[i];
        if (i != 0) out << ",";
        out << "{\"resolved\":" << bool_json(identity.resolved)
            << ",\"instanceId\":\"" << wide_json_escape(identity.instance_id)
            << "\",\"containerId\":\"" << wide_json_escape(identity.container_id)
            << "\",\"manufacturer\":\"" << wide_json_escape(identity.manufacturer)
            << "\",\"product\":\"" << wide_json_escape(identity.product)
            << "\",\"vid\":\"" << wide_json_escape(identity.vid)
            << "\",\"pid\":\"" << wide_json_escape(identity.pid)
            << "\",\"transport\":\"" << direct_transport_json(identity.transport)
            << "\",\"topologyHash\":\"" << std::hex << identity.topology_hash << std::dec
            << "\",\"usbEndpoint\":{\"available\":" << bool_json(identity.usb_endpoint.available)
            << ",\"connectionIndex\":" << identity.usb_endpoint.connection_index
            << ",\"speedCode\":" << static_cast<unsigned>(identity.usb_endpoint.speed)
            << ",\"deviceAddress\":" << identity.usb_endpoint.device_address
            << ",\"interruptInEndpointCount\":" << identity.usb_endpoint.interrupt_in_endpoint_count
            << ",\"configuredInterval\":{\"available\":false}"
            << ",\"descriptorInterval\":{\"available\":" << bool_json(identity.usb_endpoint.descriptor_interval_available)
            << ",\"value\":" << static_cast<unsigned>(identity.usb_endpoint.descriptor_interval)
            << ",\"endpointAddress\":" << static_cast<unsigned>(identity.usb_endpoint.descriptor_endpoint_address)
            << "}"
            << ",\"observedInput\":{\"available\":"
            << bool_json(identity.observed_input.available)
            << ",\"scope\":\"WM_INPUT arrival inter-arrival\","
            << "\"streams\":{"
            << "\"all\":" << timing_stream_json(identity.observed_input.streams.all)
            << ",\"movement\":" << timing_stream_json(identity.observed_input.streams.movement)
            << ",\"button\":" << timing_stream_json(identity.observed_input.streams.button)
            << ",\"wheel\":" << timing_stream_json(identity.observed_input.streams.wheel)
            << "},\"session\":{\"active\":"
            << bool_json(identity.observed_input.session.active)
            << ",\"id\":\"" << json_escape(identity.observed_input.session.id)
            << "\",\"deviceId\":\"" << json_escape(identity.observed_input.session.device_id)
            << "\",\"startedAtUtc\":\"" << json_escape(identity.observed_input.session.started_at_utc)
            << "\",\"packetCount\":" << identity.observed_input.session.packet_count
            << ",\"durationMs\":" << identity.observed_input.session.duration_ms
            << ",\"traceAvailable\":" << bool_json(identity.observed_input.session.trace_available)
            << ",\"tracePacketCount\":" << identity.observed_input.session.trace_packet_count
            << ",\"timing\":{\"intervalCount\":" << identity.observed_input.session.timing.interval_count
            << ",\"minIntervalMs\":" << identity.observed_input.session.timing.min_interval_ms
            << ",\"medianIntervalMs\":" << identity.observed_input.session.timing.median_interval_ms
            << ",\"p95IntervalMs\":" << identity.observed_input.session.timing.p95_interval_ms
            << ",\"maxIntervalMs\":" << identity.observed_input.session.timing.max_interval_ms
            << ",\"jitterP95MinusMedianMs\":" << identity.observed_input.session.timing.jitter_p95_minus_median_ms
            << "},\"distribution\":" << timing_distribution_json(identity.observed_input.session.distribution)
            << "}},\"observedInterval\":{\"available\":"
            << bool_json(raw_input_timing().any_available())
            << "} },\n"
        << "  \"sessions\": " << session_history_json() << ",\n"
        << "  \"mutation\": { \"allowed\": false }\n"
        << "}";
    return out.str();
}

bool initialize_workspace(std::string* error = nullptr) {
    const auto paths = mouse_engine::workspace::WorkspacePaths::from_root(workspace_root());
    mouse_engine::workspace::WorkspaceStore store(paths);
    return store.initialize(error);
}

int self_test() {
    std::string workspace_error;
    if (!initialize_workspace(&workspace_error)) {
        std::cerr << "self-test: " << workspace_error << "\n";
        return 2;
    }

    std::error_code ec;
    std::filesystem::create_directories(cache_root(), ec);
    if (ec) {
        std::cerr << "self-test: cannot create cache root: " << ec.message() << "\n";
        return 3;
    }

    const auto probe = workspace_root() / ".self-test";
    {
        std::ofstream file(probe, std::ios::binary | std::ios::trunc);
        if (!file) {
            std::cerr << "self-test: cannot write storage probe\n";
            return 3;
        }
        file << "Mouse Engine self-test\n";
    }
    std::filesystem::remove(probe, ec);

    mouse_engine::session::SessionTrace trace;
    trace.session_id = "self-test-session";
    trace.device_id = "self-test-device";
    trace.packets = {
        {0.0, mouse_engine::observation::Movement, 1, 0, 0, 0},
        {10.0, mouse_engine::observation::Movement, 1, 0, 0, 0},
        {70.0, mouse_engine::observation::Button, 0, 0, 1, 0},
    };

    mouse_engine::model::ObservationAnomaly anomaly;
    anomaly.id = "self-test-anomaly";
    anomaly.severity = "info";
    anomaly.type = "interval-outlier";
    anomaly.message = "Self-test anomaly anchor.";
    anomaly.stream = "all";
    anomaly.packet_index = 2;
    anomaly.timestamp_ms = 9999.0;

    const auto timeline = mouse_engine::timeline::build_timeline(trace, {anomaly}, 50.0);
    if (!timeline.available || timeline.events.size() != 7) {
        std::cerr << "self-test: investigation timeline generation failed\n";
        return 4;
    }

    bool found_anomaly = false;
    bool found_idle_gap = false;
    for (const auto& event : timeline.events) {
        if (event.kind == mouse_engine::timeline::TimelineEventKind::Anomaly) {
            found_anomaly =
                event.packet_index == 2 &&
                std::abs(event.offset_ms - 70.0) < 1e-9;
        }
        if (event.kind == mouse_engine::timeline::TimelineEventKind::IdleGap) {
            found_idle_gap =
                event.packet_index == 2 &&
                std::abs(event.offset_ms - 70.0) < 1e-9;
        }
    }
    if (!found_anomaly || !found_idle_gap) {
        std::cerr << "self-test: timeline evidence anchoring failed\n";
        return 4;
    }

    std::cout << "status=PASS\n";
    return 0;
}

void print_storage() {
    std::cout << "{\n"
              << "  \"installRoot\": \"" << json_escape(install_root().string()) << "\",\n"
              << "  \"workspaceRoot\": \"" << json_escape(workspace_root().string()) << "\",\n"
              << "  \"cacheRoot\": \"" << json_escape(cache_root().string()) << "\"\n"
              << "}\n";
}

#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
class WebViewHost {
public:
    ~WebViewHost() {
        if (webview_ && navigation_completed_token_.value != 0) {
            webview_->remove_NavigationCompleted(navigation_completed_token_);
        }
        if (webview_ && web_message_received_token_.value != 0) {
            webview_->remove_WebMessageReceived(web_message_received_token_);
        }
    }

    bool initialize(HWND hwnd, const std::filesystem::path& data_dir) {
        hwnd_ = hwnd;
        data_dir_ = data_dir;
        std::error_code ec;
        std::filesystem::create_directories(data_dir_, ec);
        if (ec) return false;

        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
            nullptr,
            data_dir_.wstring().c_str(),
            nullptr,
            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                this, &WebViewHost::environment_ready).Get());
        return SUCCEEDED(hr);
    }

    bool ready() const noexcept { return controller_ != nullptr && webview_ != nullptr; }
    HRESULT publish_snapshot() { return ui_ready_ ? post_snapshot() : S_FALSE; }
    void resize(const RECT& bounds) { if (controller_) controller_->put_Bounds(bounds); }

private:
    HRESULT post_json(const std::string& json) {
        if (!webview_) return E_FAIL;
        const std::wstring wide(json.begin(), json.end());
        const HRESULT hr = webview_->PostWebMessageAsJson(wide.c_str());
        if (FAILED(hr)) {
            show_native_fallback(hwnd_, L"Mouse Engine could not publish its read-only host message to the UI.");
        }
        return hr;
    }

    HRESULT post_snapshot() {
        return post_json(snapshot_json());
    }

    HRESULT navigation_completed(
        ICoreWebView2*,
        ICoreWebView2NavigationCompletedEventArgs* args) {
        if (!args) return E_INVALIDARG;
        BOOL success = FALSE;
        HRESULT hr = args->get_IsSuccess(&success);
        if (FAILED(hr)) return hr;
        if (!success) {
            show_native_fallback(hwnd_, L"Mouse Engine's packaged UI navigation did not complete successfully.");
            return S_OK;
        }
        const HRESULT snapshot = post_snapshot();
        if (SUCCEEDED(snapshot)) ui_ready_ = true;
        return snapshot;
    }

    HRESULT web_message_received(
        ICoreWebView2*,
        ICoreWebView2WebMessageReceivedEventArgs* args) {
        if (!args) return E_INVALIDARG;

        LPWSTR raw_json = nullptr;
        HRESULT hr = args->get_WebMessageAsJson(&raw_json);
        if (FAILED(hr) || raw_json == nullptr) return FAILED(hr) ? hr : E_INVALIDARG;

        const std::wstring message(raw_json);
        CoTaskMemFree(raw_json);

        if (message.find(L"\"type\":\"sessionTraceRequest\"") == std::wstring::npos) return S_OK;

        const std::wstring marker = L"\"sessionId\":\"";
        const auto start = message.find(marker);
        if (start == std::wstring::npos) return S_OK;
        const auto value_start = start + marker.size();
        const auto value_end = message.find(L'"', value_start);
        if (value_end == std::wstring::npos) return S_OK;

        const std::string session_id =
            wide_to_utf8(message.substr(value_start, value_end - value_start));
        if (session_id.empty()) return S_OK;

        return post_json(session_trace_json(session_id));
    }

    HRESULT environment_ready(HRESULT result, ICoreWebView2Environment* environment) {
        if (FAILED(result) || environment == nullptr) {
            show_native_fallback(hwnd_, L"WebView2 Runtime could not be started. Install or repair the Microsoft Edge WebView2 Runtime, then restart Mouse Engine.");
            return FAILED(result) ? result : E_FAIL;
        }
        environment_ = environment;
        return environment_->CreateCoreWebView2Controller(
            hwnd_,
            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                this, &WebViewHost::controller_ready).Get());
    }

    HRESULT controller_ready(HRESULT result, ICoreWebView2Controller* controller) {
        if (FAILED(result) || controller == nullptr) {
            show_native_fallback(hwnd_, L"WebView2 could not create its browser controller. Install or repair the Microsoft Edge WebView2 Runtime, then restart Mouse Engine.");
            return FAILED(result) ? result : E_FAIL;
        }
        controller_ = controller;
        HRESULT hr = controller_->get_CoreWebView2(&webview_);
        if (FAILED(hr) || !webview_) {
            show_native_fallback(hwnd_, L"Mouse Engine created the WebView2 controller but could not access the browser instance.");
            return FAILED(hr) ? hr : E_FAIL;
        }

        ComPtr<ICoreWebView2Settings> settings;
        hr = webview_->get_Settings(&settings);
        if (FAILED(hr) || !settings) {
            show_native_fallback(hwnd_, L"Mouse Engine could not access WebView2 settings.");
            return FAILED(hr) ? hr : E_FAIL;
        }
        hr = settings->put_IsWebMessageEnabled(TRUE);
        if (FAILED(hr)) {
            show_native_fallback(hwnd_, L"Mouse Engine could not enable its read-only WebView2 message channel.");
            return hr;
        }

        hr = webview_->add_NavigationCompleted(
            Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
                this, &WebViewHost::navigation_completed).Get(),
            &navigation_completed_token_);
        if (FAILED(hr)) {
            show_native_fallback(hwnd_, L"Mouse Engine could not register its WebView2 navigation observer.");
            return hr;
        }

        hr = webview_->add_WebMessageReceived(
            Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                this, &WebViewHost::web_message_received).Get(),
            &web_message_received_token_);
        if (FAILED(hr)) {
            show_native_fallback(hwnd_, L"Mouse Engine could not register its read-only message observer.");
            return hr;
        }

        RECT bounds{};
        GetClientRect(hwnd_, &bounds);
        controller_->put_Bounds(bounds);
        const auto html = ui_entrypoint();
        if (!std::filesystem::exists(html)) {
            show_native_fallback(hwnd_, L"Mouse Engine could not find its packaged UI file (ui\\index.html).");
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
        std::wstring uri = L"file:///";
        for (wchar_t c : html.wstring()) uri += (c == L'\\' ? L'/' : c);
        const HRESULT navigation = webview_->Navigate(uri.c_str());
        if (FAILED(navigation)) {
            show_native_fallback(hwnd_, L"Mouse Engine could not navigate to its packaged UI.");
        }
        return navigation;
    }

    HWND hwnd_{};
    std::filesystem::path data_dir_;
    ComPtr<ICoreWebView2Environment> environment_;
    ComPtr<ICoreWebView2Controller> controller_;
    ComPtr<ICoreWebView2> webview_;
    EventRegistrationToken navigation_completed_token_{};
    EventRegistrationToken web_message_received_token_{};
    bool ui_ready_{false};
};
#endif

bool register_mouse_device_notifications(HWND hwnd) {
    RAWINPUTDEVICE mouse{};
    mouse.usUsagePage = 0x01;
    mouse.usUsage = 0x02;
    mouse.dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
    mouse.hwndTarget = hwnd;
    return RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) == TRUE;
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
    static WebViewHost* webview = nullptr;
#endif
    switch (message) {
    case WM_CREATE:
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
        webview = new WebViewHost();
        register_mouse_device_notifications(hwnd);
        SetTimer(hwnd, kSnapshotTimerId, 250, nullptr);
        if (!webview->initialize(hwnd, cache_root() / L"WebView2")) {
            delete webview; webview = nullptr;
            show_native_fallback(hwnd,L"WebView2 could not be initialized. Install the Microsoft Edge WebView2 Runtime, then restart Mouse Engine.");
        }
#else
        (void)lparam;
        show_native_fallback(hwnd,L"This Mouse Engine build was compiled without WebView2 support.");
#endif
        return 0;

    case WM_TIMER:
        if (wparam == kSnapshotTimerId) {
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
            if (webview && webview->ready()) webview->publish_snapshot();
#endif
        }
        return 0;

    case WM_SIZE:
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
        if (webview && webview->ready()) { RECT bounds{}; GetClientRect(hwnd,&bounds); webview->resize(bounds); }
#endif
        return 0;

    case WM_INPUT:
        raw_input_timing().record(reinterpret_cast<HRAWINPUT>(lparam));
        return DefWindowProcW(hwnd, message, wparam, lparam);

    case WM_INPUT_DEVICE_CHANGE:
        if (wparam == GIDC_REMOVAL) {
            raw_input_timing().remove_device(reinterpret_cast<HRAWINPUT>(lparam));
        }
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
        if ((wparam == GIDC_ARRIVAL || wparam == GIDC_REMOVAL) && webview && webview->ready()) {
            webview->publish_snapshot();
        }
#endif
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, kSnapshotTimerId);
        raw_input_timing().clear();
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
        delete webview;
        webview = nullptr;
#endif
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
}

int run_gui(HINSTANCE instance) {
    const wchar_t* class_name = L"MouseEngineHostWindow";
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpfnWndProc = window_proc;
    wc.lpszClassName = class_name;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) return 10;

    HWND hwnd = CreateWindowExW(
        0, class_name, L"Mouse Engine",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 760,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 11;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR command_line, int) {
    HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const int result = [&]() -> int {
        std::string workspace_error;
        if (!initialize_workspace(&workspace_error)) {
            std::cerr << "workspace: " << workspace_error << "\n";
            return 1;
        }

        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        std::wstring command;
        if (argc > 1 && argv) command = argv[1];
        if (argv) LocalFree(argv);

        if (command == L"--self-test") return self_test();
        if (command == L"--print-environment") {
            std::cout << environment_json() << "\n";
            return 0;
        }
        if (command == L"--print-storage") {
            print_storage();
            return 0;
        }
        if (command == L"--print-snapshot") {
            std::cout << snapshot_json() << "\n";
            return 0;
        }
        return run_gui(instance);
    }();
    if (SUCCEEDED(com)) CoUninitialize();
    return result;
}
