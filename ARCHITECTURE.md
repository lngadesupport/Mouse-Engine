# Mouse Engine Architecture

## Mission

Mouse Engine is a Windows-native platform for universal mouse control, diagnostics, measurement, and evidence-based latency optimization.

Core principle:

> Identify → Expose capabilities → Measure → Snapshot → Change one variable → Benchmark → Keep or Rollback

## Architectural pillars

1. Device Fingerprinting
2. Capability Engine
3. Protocol Adapter Framework
4. Latency Measurement Engine
5. Snapshot + Rollback

A cross-cutting Evidence & Confidence layer prevents unsupported claims.

## Dependency direction

UI → Application/Core → Domain contracts → Platform/Adapters

The domain must not depend on WinUI, vendor SDKs, or a specific device.

## Initial repository layout

- `src/MouseEngine.Core/` — platform-independent domain contracts and orchestration
- `src/MouseEngine.Platform.Windows/` — Windows HID, Raw Input, SetupAPI, registry/power integrations
- `src/MouseEngine.Adapters/` — vendor/OEM protocol adapters
- `src/MouseEngine.Database/` — versioned device fingerprints and capability knowledge
- `src/MouseEngine.App/` — future WinUI 3 application
- `tests/` — unit and integration tests

## Safety boundaries

- Read-only discovery is the default.
- Device writes require an adapter capability explicitly declaring support.
- Firmware flashing is not part of v1.
- Experimental system changes require a snapshot.
- No optimization is considered successful without before/after measurement.
- Unsupported capabilities are hidden rather than presented as fake disabled controls.
