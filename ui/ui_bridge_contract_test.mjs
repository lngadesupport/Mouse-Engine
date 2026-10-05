import fs from 'node:fs';
import assert from 'node:assert/strict';

const host = fs.readFileSync(new URL('../src/MouseEngine.Host.Windows/main.cpp', import.meta.url), 'utf8');
const ui = fs.readFileSync(new URL('./index.html', import.meta.url), 'utf8');

const hostRequired = [
  'GetRawInputDeviceList',
  'RIM_TYPEMOUSE',
  'GetRawInputDeviceInfoW',
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
  'schemaVersion'
];

for (const token of hostRequired) {
  assert.ok(host.includes(token), `missing WebView2 host bridge contract: ${token}`);
}

assert.match(host, /IsWebMessageEnabled/);
assert.match(host, /PostWebMessageAsJson/);
assert.match(host, /NavigationCompleted/);
assert.doesNotMatch(host, /ExecuteScript\(/);

for (const token of ['window.chrome.webview', "addEventListener('message'", 'schemaVersion']) {
  assert.ok(ui.includes(token), `missing WebView2 UI bridge contract: ${token}`);
}

assert.doesNotMatch(ui, /chrome\.webview\.postMessage|window\.chrome\.webview\.postMessage/);
console.log('WebView2 bridge contract PASS');
