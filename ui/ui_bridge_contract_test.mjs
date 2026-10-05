import fs from 'node:fs';
import assert from 'node:assert/strict';

const host = fs.readFileSync(new URL('../src/MouseEngine.Host.Windows/main.cpp', import.meta.url), 'utf8');

const required = [
  'put_IsWebMessageEnabled(TRUE)',
  'PostWebMessageAsJson(snapshot_json().c_str())',
  'add_NavigationCompleted',
  'NavigationCompleted',
  'window.chrome.webview',
  'schemaVersion'
];

for (const token of required) {
  assert.ok(host.includes(token), `missing WebView2 bridge contract: ${token}`);
}

assert.match(host, /IsWebMessageEnabled/);
assert.match(host, /PostWebMessageAsJson/);
assert.match(host, /NavigationCompleted/);
assert.doesNotMatch(host, /ExecuteScript\(/);
console.log('WebView2 bridge contract PASS');
