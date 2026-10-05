import fs from 'node:fs';
import assert from 'node:assert/strict';

const html = fs.readFileSync(new URL('./index.html', import.meta.url), 'utf8');
const required = [
  'Overview', 'Devices', 'Performance', 'Controls', 'Profiles', 'Analyzer',
  'Latency Lab', 'Diagnostics', 'Firmware', 'Settings', 'Evidence Rail',
  'Mutation policy', 'DENIED BY DEFAULT', 'No device telemetry available'
];
for (const label of required) assert.ok(html.includes(label), `missing UI contract: ${label}`);
assert.match(html, /id="app"/);
assert.match(html, /aria-live="polite"/);
assert.match(html, /window\.MouseEngineHost/);
assert.doesNotMatch(html, /window\.MouseEngineHost\.set|fetch\(|XMLHttpRequest/);
console.log('UI contract PASS');
