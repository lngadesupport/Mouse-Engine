import fs from 'node:fs';
import assert from 'node:assert/strict';

const html = fs.readFileSync(new URL('./index.html', import.meta.url), 'utf8');
const required = [
  'Overview', 'Devices', 'Performance', 'Controls', 'Profiles', 'Analyzer',
  'Latency Lab', 'Diagnostics', 'Firmware', 'Settings', 'Evidence Rail',
  'Mutation policy', 'DENIED BY DEFAULT', 'No device telemetry available',
  'Host snapshot: awaiting', 'schemaVersion===2', 'mouseCount', 'observationAvailable', 'identityResolvedCount', 'instanceId', 'containerId', 'manufacturer', 'product', 'transport', 'topologyHash', 'direct ancestry evidence', 'Mixed — per-device direct ancestry evidence',
  'configuredInterval', 'descriptorInterval', 'observedInput', 'medianIntervalMs', 'p95IntervalMs', 'jitterP95MinusMedianMs', 'WM_INPUT arrival inter-arrival', 'All input', 'Movement stream', 'Button events', 'Wheel events', 'streams', 'packetCount', 'timing pending', 'idleGapCount50ms', 'longestIdleGapMs',
  'Configured', 'Device reported', 'Observed', 'Not measured', 'Investigation Timeline', 'session-investigation-timeline', 'packetIndex', 'timestampMs', '50 ms segmentation threshold'
];
for (const label of required) assert.ok(html.includes(label), `missing UI contract: ${label}`);
assert.match(html, /id="app"/);
assert.match(html, /aria-live="polite"/);
assert.match(html, /window\.chrome\.webview/);
assert.match(html, /addEventListener\('message'/);
assert.doesNotMatch(html, /window\.MouseEngineHost\.set|fetch\(|XMLHttpRequest/);
const outboundTraceRequests = html.match(/window\.chrome\.webview\.postMessage\(/g) || [];
assert.equal(outboundTraceRequests.length, 1, 'only one outbound WebView2 operation is permitted');
assert.match(html, /window\.chrome\.webview\.postMessage\(\{\s*type:'sessionTraceRequest'/);
console.log('UI contract PASS');
