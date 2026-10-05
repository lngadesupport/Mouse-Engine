# Session Investigation Timeline

The Session Investigation Timeline is a read-only analytical view derived from persisted evidence. Core `SessionTimeline` is the semantic authority for event construction; the Windows host serializes that result to the WebView2 UI, which only renders it.

## Evidence model

The timeline combines:
- persisted SessionTrace packets;
- packet-relative timestamps from the trace;
- the existing 50 ms idle-gap segmentation threshold;
- persisted anomaly records anchored to a packet index;
- the same observed timing scope already used by the Timing Distribution Lab.

SessionRecorder generates the Core timing-irregularity records when a captured session is snapshotted, preserving the original packet index and timestamp. The host loads the persisted session summary to recover anomaly anchors, converts those records into the Core anomaly model, and calls `build_timeline` against the persisted trace. The UI does not independently infer packet ordering, idle gaps, or anomaly placement.

It does not reconstruct packets from histograms, CDFs, medians, P95, or any other aggregate statistic.

## Event types

Session start
Packet
Idle gap
Anomaly
Session end

All offsets are relative to the first persisted packet timestamp.

## Anomaly anchoring

ObservationAnomaly now carries packet_index and timestamp_ms.
The timing irregularity detector records the packet that closes the anomalous interval. Serialization persists both values so the host/UI can place the finding on the same temporal axis as the packet trace.

Anomaly placement remains observational. It does not establish a hardware fault, polling frequency, causality, or user-perceptible latency.

## UI behavior

The Session Explorer exposes:
- persisted distribution comparison;
- deterministic packet replay;
- Investigation Timeline;
- Anomaly Ledger.

The timeline display is intentionally capped for large traces; the replay scrubber remains the packet-level inspection surface.

## Integrity boundaries

A timeline is unavailable when the persisted trace cannot produce a valid monotonic replay.
Negative, non-finite, or regressing timestamps are rejected by the replay layer.
Mutation is not exposed by this feature.
