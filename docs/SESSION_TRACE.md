# Session Trace

Mouse Engine persists observation summaries separately from an optional packet trace.

## Storage

For a persisted session:

- summary: `Documents/Mouse Engine/Sessions/<session-id>.json`
- trace sidecar: `Documents/Mouse Engine/Sessions/<session-id>.trace.jsonl`

The summary remains small and suitable for Session Explorer, comparison and CDF analysis. The trace is loaded only when the user requests replay.

## Trace schema

Trace schema version: **1**.

The first JSONL line is metadata:

```json
{
  "schemaVersion": 1,
  "sessionId": "session-...",
  "deviceId": "...",
  "packetCount": 123,
  "truncated": false
}
```

Each following line is one observed Raw Input mouse event:

```json
{
  "timestampMs": 12.500,
  "classes": 1,
  "dx": 4,
  "dy": -2,
  "buttons": 0,
  "wheel": 0
}
```

`timestampMs` is the session-relative host observation timestamp. It is not a bus timestamp and does not establish a physical polling rate.

## Bounds

The native trace recorder is bounded to 65,536 packets by default. When the bound is reached:

1. the trace is marked `truncated=true`;
2. the current observation session is finalized;
3. a new session is started;
4. capture continues.

This prevents an unbounded telemetry buffer.

## Replay semantics

Replay is deterministic and read-only.

Mouse Engine reconstructs:

- event order;
- session-relative event timing;
- movement trajectory from captured `dx/dy`;
- button-event classes;
- wheel events;
- event-level details.

Mouse Engine does **not** reconstruct packets from:

- histograms;
- percentiles;
- CDFs;
- median/P95;
- inferred polling rates;
- configuration claims.

If a trace is absent or corrupt, replay remains unavailable rather than synthesizing data.

## Privacy

The trace contains device/session identifiers already used by the local session model and observed mouse event fields. It does not intentionally collect account credentials, browser history, or user identity.

Trace capture is an explicit Core policy. The Windows host currently enables it for local session replay. No trace data is sent to a cloud service by this local host path.

## Integrity

Trace loading validates:

- schema version;
- session ID;
- packet count declared by the header;
- numeric packet fields;
- monotonic timestamps during replay validation.

Corrupt traces are rejected.
