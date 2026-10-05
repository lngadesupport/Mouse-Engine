import fs from 'node:fs';
import assert from 'node:assert/strict';

const host = fs.readFileSync(new URL('../src/MouseEngine.Host.Windows/main.cpp', import.meta.url), 'utf8');
const ui = fs.readFileSync(new URL('./index.html', import.meta.url), 'utf8');

const hostRequired = [
  'GetRawInputDeviceList',
  'RIM_TYPEMOUSE',
  'GetRawInputDeviceInfoW',
  'SetupDiGetClassDevsW',
  'SetupDiEnumDeviceInterfaces',
  'SetupDiGetDeviceInterfaceDetailW',
  'SetupDiGetDevicePropertyW',
  'DEVPKEY_Device_ContainerId',
  'CM_Get_Parent',
  'CM_Get_Device_IDW',
  'CM_Get_DevNode_Registry_PropertyW',
  'CM_DRP_LOCATION_INFORMATION',
  'USB\\',
  'BTH\\',
  'Unknown',
  'Usb',
  'Bluetooth',
  'topologyHash',
  'RegisterRawInputDevices',
  'RIDEV_DEVNOTIFY',
  'WM_INPUT_DEVICE_CHANGE',
  'GIDC_ARRIVAL',
  'GIDC_REMOVAL',
  'mouseCount',
  'WM_INPUT',
  'GetRawInputData',
  'QueryPerformanceCounter',
  'GetSystemTimePreciseAsFileTime', 'SessionCapture', 'SessionStore', 'pending_finalization_',
  'QueryPerformanceFrequency',
  'RIDEV_INPUTSINK',
  'InputTimingAccumulator',
  'medianIntervalMs',
  'p95IntervalMs',
  'jitterP95MinusMedianMs',
  'observedInput', 'SessionCapture', 'SessionStore', 'session', 'startedAtUtc', 'durationMs', 'RawInputClassification', 'raw_mouse_has_movement', 'raw_mouse_has_button_event', 'raw_mouse_has_wheel_event', 'streams', 'packetCount', 'timing', 'idleGapCount50ms', 'longestIdleGapMs', 'movement', 'button', 'wheel',
  'put_IsWebMessageEnabled(TRUE)',
  'PostWebMessageAsJson',
  'add_NavigationCompleted',
  'NavigationCompleted',
  'schemaVersion',
  'schemaVersion": 2',
  'usbioctl.h',
  'IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX',
  'USB_NODE_CONNECTION_INFORMATION_EX',
  'USB_PIPE_INFO',
  'GUID_DEVINTERFACE_USB_HUB',
  'CreateFileW',
  'DeviceIoControl',
  'ConnectionIndex',
  'EndpointDescriptor',
  'bInterval',
  'configuredInterval',
  'descriptorInterval',
  'observedInterval',
  'SessionTraceStore',
  'SessionTrace',
  'traceAvailable',
  'tracePacketCount',
  'session_trace_json',
  'get_WebMessageAsJson',
  'add_WebMessageReceived',
  'sessionTraceRequest'
];

for (const token of hostRequired) {
  assert.ok(host.includes(token), `missing Windows evidence contract: ${token}`);
}

assert.match(host, /IsWebMessageEnabled/);
assert.match(host, /PostWebMessageAsJson/);
assert.match(host, /NavigationCompleted/);
assert.doesNotMatch(host, /ExecuteScript\(/);
assert.doesNotMatch(host, /IOCTL_USB_HUB_CYCLE_PORT|IOCTL_USB_RESET_HUB_PORT|IOCTL_USB_RESET_PORT/);

for (const token of ['window.chrome.webview', "addEventListener('message'", 'schemaVersion']) {
  assert.ok(ui.includes(token), `missing WebView2 UI bridge contract: ${token}`);
}

assert.match(ui, /window\.chrome\.webview\.postMessage\(\{\s*type:'sessionTraceRequest'/);
console.log('WebView2 bridge contract PASS');

assert(ui.includes('timing-distribution-chart'), 'Timing distribution UI must be present');
assert(ui.includes('cumulativeFraction'), 'Timing distribution must expose CDF evidence');
assert(ui.includes('WM_INPUT inter-arrival'), 'Timing distribution scope must remain explicit');

assert(ui.includes('Session Explorer'), 'session explorer surface');
assert(ui.includes('session-history-list'), 'session history list');
assert(ui.includes('session-compare-a'), 'session comparison A');
assert(ui.includes('session-compare-b'), 'session comparison B');
assert(ui.includes('Paired session summary; descriptive deltas only; no winner inference.'), 'comparison methodology');
assert(ui.includes('snapshot.sessions.items'), 'session history snapshot contract');

assert(ui.includes('Persisted CDF comparison'), 'persisted CDF comparison surface');
assert(ui.includes('Anomaly Ledger'), 'anomaly ledger surface');
assert(ui.includes('Session Replay'), 'session replay surface');
assert(ui.includes('session-replay-load'), 'session replay load control');
assert(ui.includes('session-replay-scrubber'), 'session replay scrubber');
assert(ui.includes('tracePacketCount'), 'trace packet count contract');
assert(ui.includes('Packet trace available'), 'replay limitation disclosure');
assert(ui.includes('sequence-only') || ui.includes('Sequence'), 'anomaly sequence disclosure');
