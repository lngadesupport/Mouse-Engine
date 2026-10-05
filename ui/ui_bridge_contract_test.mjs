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
  'put_IsWebMessageEnabled(TRUE)',
  'PostWebMessageAsJson(snapshot_json().c_str())',
  'add_NavigationCompleted',
  'NavigationCompleted',
  'schemaVersion',
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
  'observedInterval'
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

assert.doesNotMatch(ui, /chrome\.webview\.postMessage|window\.chrome\.webview\.postMessage/);
console.log('WebView2 bridge contract PASS');
