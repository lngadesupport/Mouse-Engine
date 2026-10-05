#include <windows.h>
#include <algorithm>
#include <shlobj.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <windowsx.h>

#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
#include <wrl.h>
#include <WebView2.h>
#endif

namespace {

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

std::filesystem::path storage_root() {
    return local_app_data() / L"Mouse Engine";
}

std::filesystem::path install_root() {
    return local_app_data() / L"Programs" / L"Mouse Engine";
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

struct RawMouseObservation {
    bool available{false};
    bool detail_available{false};
    UINT mouse_count{0};
    DWORD max_buttons{0};
    DWORD max_sample_rate{0};
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

std::string snapshot_json() {
    const RawMouseObservation mouse = observe_raw_mice();
    std::ostringstream out;
    out << "{\n"
        << "  \"schemaVersion\": 1,\n"
        << "  \"host\": { \"online\": true, \"platform\": \"windows\", \"architecture\": \"x64\" },\n"
        << "  \"device\": { \"observationAvailable\": " << bool_json(mouse.available)
        << ", \"connected\": " << bool_json(mouse.available && mouse.mouse_count > 0)
        << ", \"mouseCount\": " << mouse.mouse_count
        << ", \"detailAvailable\": " << bool_json(mouse.detail_available)
        << ", \"maxObservedButtons\": " << mouse.max_buttons
        << ", \"maxObservedSampleRate\": " << mouse.max_sample_rate << " },\n"
        << "  \"latency\": { \"available\": false },\n"
        << "  \"mutation\": { \"allowed\": false }\n"
        << "}";
    return out.str();
}

int self_test() {
    std::error_code ec;
    std::filesystem::create_directories(storage_root(), ec);
    if (ec) {
        std::cerr << "self-test: cannot create storage root: " << ec.message() << "\n";
        return 2;
    }

    const auto probe = storage_root() / ".self-test";
    {
        std::ofstream file(probe, std::ios::binary | std::ios::trunc);
        if (!file) {
            std::cerr << "self-test: cannot write storage probe\n";
            return 3;
        }
        file << "Mouse Engine self-test\n";
    }
    std::filesystem::remove(probe, ec);
    std::cout << "status=PASS\n";
    return 0;
}

void print_storage() {
    std::cout << "{\n"
              << "  \"installRoot\": \"" << json_escape(install_root().string()) << "\",\n"
              << "  \"userData\": \"" << json_escape(storage_root().string()) << "\"\n"
              << "}\n";
}

#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
class WebViewHost {
public:
    ~WebViewHost() {
        if (webview_ && navigation_completed_token_.value != 0) {
            webview_->remove_NavigationCompleted(navigation_completed_token_);
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
    HRESULT post_snapshot() {
        if (!webview_) return E_FAIL;
        const std::string json = snapshot_json();
        const std::wstring wide(json.begin(), json.end());
        const HRESULT hr = webview_->PostWebMessageAsJson(wide.c_str());
        if (FAILED(hr)) {
            show_native_fallback(hwnd_, L"Mouse Engine could not publish its read-only host snapshot to the UI.");
        }
        return hr;
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
    bool ui_ready_{false};
};
#endif

bool register_mouse_device_notifications(HWND hwnd) {
    RAWINPUTDEVICE mouse{};
    mouse.usUsagePage = 0x01;
    mouse.usUsage = 0x02;
    mouse.dwFlags = RIDEV_DEVNOTIFY;
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
        if (!webview->initialize(hwnd, storage_root() / L"WebView2")) {
            delete webview; webview = nullptr;
            show_native_fallback(hwnd,L"WebView2 could not be initialized. Install the Microsoft Edge WebView2 Runtime, then restart Mouse Engine.");
        }
#else
        (void)lparam;
        show_native_fallback(hwnd,L"This Mouse Engine build was compiled without WebView2 support.");
#endif
        return 0;

    case WM_SIZE:
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
        if (webview && webview->ready()) { RECT bounds{}; GetClientRect(hwnd,&bounds); webview->resize(bounds); }
#endif
        return 0;

    case WM_INPUT_DEVICE_CHANGE:
#ifdef MOUSE_ENGINE_WEBVIEW2_SDK
        if ((wparam == GIDC_ARRIVAL || wparam == GIDC_REMOVAL) && webview && webview->ready()) {
            webview->publish_snapshot();
        }
#endif
        return 0;

    case WM_DESTROY:
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
