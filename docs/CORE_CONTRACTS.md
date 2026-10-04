# Core Contracts

## DeviceFingerprint

A fingerprint uniquely identifies a physical mouse configuration as far as the available evidence permits.

Required evidence fields include VID, PID, USB/HID identity, transport, report descriptor identity, firmware/hardware revision when available, and protocol identity.

VID/PID alone is never sufficient for model-level write compatibility.

## Capability

A capability describes an operation or measurable property supported by a specific device/revision/protocol.

Every capability carries:
- identifier
- category
- support level
- confidence/evidence
- read/write safety
- source/provenance

## ProtocolAdapter

Adapters are isolated from the core and expose only explicitly declared operations.

Required conceptual operations:
- identify
- query capabilities
- read state
- write configuration
- profile operations
- diagnostics
- firmware information

Firmware flashing is excluded from the v1 adapter contract.

## Latency Measurement

Measurements must preserve:
- metric
- unit
- samples
- timestamp
- test conditions
- percentiles
- variability
- dropped/burst reports where applicable
- confidence

A stage is `NotMeasurable` when direct instrumentation is unavailable.

## Snapshot

A snapshot captures the state necessary to restore a change.

Experimental mutation without a valid snapshot is rejected.

## Evidence

Evidence levels:
- Verified
- Measured
- Estimated
- Experimental
- Unsupported

Claims shown in the UI must retain their evidence level.
