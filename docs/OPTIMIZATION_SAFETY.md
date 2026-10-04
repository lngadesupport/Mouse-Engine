# Optimization Safety Policy

Mouse Engine uses controlled experimentation rather than blind tweak packs.

## Experiment lifecycle

1. Capture baseline.
2. Create snapshot.
3. Apply exactly one variable where practical.
4. Run repeated benchmark samples.
5. Compare distributions, not only averages.
6. Check CPU, frame-time, stability, and side effects.
7. Keep only if improvement is repeatable and materially justified.
8. Otherwise rollback automatically.

## Experimental variables

Potentially risky variables such as timer behavior, scheduler configuration, affinity, power policy, Dynamic Tick, HPET-related settings, or process priority are classified as experimental.

They are never bundled into a universal "gaming tweak" preset.

## Firmware

v1 only detects and reports firmware information and can direct users to the manufacturer's official support/update path. Mouse Engine does not flash firmware in v1.

## Security

Mouse Engine must not disable antivirus, security controls, integrity protections, or other system security features as an optimization shortcut.

## Coexistence

Official vendor software should continue to work unless the user explicitly chooses a controlled exclusive-access workflow supported by the adapter.
