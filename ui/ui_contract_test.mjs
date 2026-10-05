import fs from 'node:fs';
import assert from 'node:assert/strict';

const html = fs.readFileSync(new URL('./index.html', import.meta.url), 'utf8');
const required = [
  'Overview', 'Devices', 'Performance', 'Controls', 'Profiles', 'Analyzer',
  'Latency Lab', 'Diagnostics', 'Firmware', 'Settings', 'Evidence Rail',
  'Mutation policy', 'DENIED BY DEFAULT', 'No device telemetry available',
  'Host snapshot: awaiting'
];
for (const label of required) assert.ok(html.includes(label), `missing UI contract: ${label}`);
assert.match(html, /id="app"/);
assert.match(html, /aria-live="polite"/);
assert.match(html, /window\.chrome\.webview/);
assert.match(html, /addEventListener\('message'/);
assert.doesNotMatch(html, /window\.MouseEngineHost\.set|window\.chrome\.webview\.postMessage|fetch\(|XMLHttpRequest/);
console.log('UI contract PASS');
