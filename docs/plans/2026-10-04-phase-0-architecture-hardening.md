# Phase 0 Architecture Hardening Implementation Plan

> **For agentic workers:** Use the host's available task-by-task implementation workflow. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish the complete platform-independent architecture needed for safe device identity, compatibility, adapters, measurement, experiments, evidence, recovery, profiles, diagnostics, privacy, and future Windows discovery.

**Architecture:** The Core remains platform-independent and owns domain contracts, state machines, evidence, statistics, experiments, safety, and audit semantics. Windows integrations and vendor/OEM adapters depend on Core contracts but Core never depends on them. UI consumes application-facing schemas rather than hardcoding device pages.

**Tech Stack:** C++20, CMake, Windows APIs in the future Platform.Windows layer, WinUI 3 in the future App layer, GitHub Actions for CI.

## Global Constraints

- Read-only discovery is the default.
- VID/PID alone is never sufficient for model-level write compatibility.
- Unsupported capabilities are hidden rather than represented as fake disabled controls.
- Firmware flashing is excluded from v1.
- Experimental mutations require a valid snapshot and transaction.
- Optimization decisions require measured before/after evidence.
- Directly unmeasurable stages must be represented as `NotMeasurable`.
- Every externally visible claim retains evidence level and provenance.
- Adapter operations are permission-scoped and isolated from Core.
- The same physical mouse must retain a stable identity across USB, 2.4 GHz, and Bluetooth sessions when evidence permits.
- Vendor software coexistence is a first-class constraint.
- Privacy-sensitive telemetry is opt-in and minimized.

---

### Task 1: Core identity, compatibility, and evidence model

**Files:**
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/DeviceIdentity.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Compatibility.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/EvidenceGraph.h`
- Modify: `src/MouseEngine.Core/include/MouseEngine/Core/DeviceFingerprint.h`
- Test: `tests/MouseEngine.Core.Tests/identity_contracts.cpp`

**Interfaces:**
- Consumes: `DeviceFingerprint`, `EvidenceLevel`
- Produces: `PhysicalDeviceId`, `DeviceSessionId`, `DeviceIdentity`, `CompatibilityAssessment`, `EvidenceNode`, `EvidenceEdge`

- [ ] **Step 1: Add focused failing tests**

Assert that identity separates physical identity from connection session; descriptor/serial/protocol evidence contributes to confidence; conflicting evidence lowers confidence; and compatibility cannot authorize writes from VID/PID alone.

- [ ] **Step 2: Verify the relevant failure**

Run: `cmake -S . -B build && cmake --build build`
Expected: compilation fails because the new identity/compatibility contracts do not yet exist.

- [ ] **Step 3: Implement the minimum behavior**

Add stable opaque IDs, evidence-backed identity resolution, compatibility states (`Unknown`, `Compatible`, `ConditionallyCompatible`, `Incompatible`), and explicit write authorization requiring model/revision/protocol evidence.

- [ ] **Step 4: Verify the focused pass**

Run: `cmake -S . -B build && cmake --build build`
Expected: the Core target compiles with the new contracts.

- [ ] **Step 5: Run the affected integration check**

Run: `cmake --build build`
Expected: all configured Core targets compile without warnings promoted to errors.

- [ ] **Step 6: Commit the passing deliverable**

```bash
git add src/MouseEngine.Core/include/MouseEngine/Core/DeviceIdentity.h src/MouseEngine.Core/include/MouseEngine/Core/Compatibility.h src/MouseEngine.Core/include/MouseEngine/Core/EvidenceGraph.h src/MouseEngine.Core/include/MouseEngine/Core/DeviceFingerprint.h tests/MouseEngine.Core.Tests/identity_contracts.cpp
git commit -m "feat: add identity compatibility and evidence contracts"
```

### Task 2: Adapter permissions, profiles, and safety transactions

**Files:**
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Adapter.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Profile.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Snapshot.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Transaction.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Audit.h`
- Test: `tests/MouseEngine.Core.Tests/safety_contracts.cpp`

**Interfaces:**
- Consumes: `DeviceIdentity`, `Capability`, `EvidenceLevel`
- Produces: `AdapterManifest`, `AdapterPermission`, `Profile`, `Snapshot`, `Transaction`, `AuditEvent`

- [ ] **Step 1: Add focused failing tests**

Assert read-only adapters can inspect but cannot mutate; experimental mutation without a snapshot is rejected; interrupted transactions are recoverable; and firmware flashing is not a v1 permission.

- [ ] **Step 2: Verify the relevant failure**

Run: `cmake -S . -B build && cmake --build build`
Expected: compilation fails because safety contracts are absent.

- [ ] **Step 3: Implement the minimum behavior**

Define explicit adapter permissions, mutation risk classes, snapshot validity, transaction states (`Prepared`, `Applied`, `Benchmarking`, `Committed`, `RolledBack`, `Interrupted`, `RecoveryRequired`), profile scopes, and append-only audit events.

- [ ] **Step 4: Verify the focused pass**

Run: `cmake -S . -B build && cmake --build build`
Expected: Core safety contracts compile.

- [ ] **Step 5: Run the affected integration check**

Run: `cmake --build build`
Expected: all Core targets compile.

- [ ] **Step 6: Commit the passing deliverable**

```bash
git add src/MouseEngine.Core/include/MouseEngine/Core/Adapter.h src/MouseEngine.Core/include/MouseEngine/Core/Profile.h src/MouseEngine.Core/include/MouseEngine/Core/Snapshot.h src/MouseEngine.Core/include/MouseEngine/Core/Transaction.h src/MouseEngine.Core/include/MouseEngine/Core/Audit.h tests/MouseEngine.Core.Tests/safety_contracts.cpp
git commit -m "feat: add adapter and safety transaction contracts"
```

### Task 3: Measurement, statistics, and experiment framework

**Files:**
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Measurement.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Statistics.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Experiment.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Diagnostic.h`
- Test: `tests/MouseEngine.Core.Tests/measurement_contracts.cpp`

**Interfaces:**
- Consumes: transaction/snapshot semantics and evidence contracts
- Produces: `MeasurementSeries`, `StatisticalSummary`, `ExperimentDefinition`, `ExperimentResult`, `DiagnosticFinding`

- [ ] **Step 1: Add focused failing tests**

Assert percentile ordering, sample-count preservation, missing instrumentation represented as `NotMeasurable`, single-variable experiment definitions, baseline/treatment separation, and repeatability metadata.

- [ ] **Step 2: Verify the relevant failure**

Run: `cmake -S . -B build && cmake --build build`
Expected: compilation fails because measurement and experiment contracts are absent.

- [ ] **Step 3: Implement the minimum behavior**

Define units, sample series, percentiles, MAD/stddev, outlier metadata, effect size, repeatability, experiment hypotheses, baseline/treatment phases, decision states, and diagnostic severity/confidence.

- [ ] **Step 4: Verify the focused pass**

Run: `cmake -S . -B build && cmake --build build`
Expected: Core measurement contracts compile.

- [ ] **Step 5: Run the affected integration check**

Run: `cmake --build build`
Expected: all Core targets compile.

- [ ] **Step 6: Commit the passing deliverable**

```bash
git add src/MouseEngine.Core/include/MouseEngine/Core/Measurement.h src/MouseEngine.Core/include/MouseEngine/Core/Statistics.h src/MouseEngine.Core/include/MouseEngine/Core/Experiment.h src/MouseEngine.Core/include/MouseEngine/Core/Diagnostic.h tests/MouseEngine.Core.Tests/measurement_contracts.cpp
git commit -m "feat: add measurement statistics and experiment contracts"
```

### Task 4: Platform boundaries, simulation, privacy, and compatibility schema

**Files:**
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/PlatformBoundary.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Simulation.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/Privacy.h`
- Create: `src/MouseEngine.Core/include/MouseEngine/Core/CompatibilitySchema.h`
- Create: `docs/CORE_ARCHITECTURE.md`
- Modify: `ARCHITECTURE.md`
- Modify: `docs/CORE_CONTRACTS.md`
- Test: `tests/MouseEngine.Core.Tests/boundary_contracts.cpp`

**Interfaces:**
- Consumes: all Core contracts
- Produces: platform capability boundaries, virtual devices, privacy policies, versioned knowledge records, and UI-safe capability schemas.

- [ ] **Step 1: Add focused failing tests**

Assert Core contracts contain no Windows/vendor UI dependency; simulated devices can reproduce fingerprints and reports; telemetry defaults to local-only; and compatibility records are versioned.

- [ ] **Step 2: Verify the relevant failure**

Run: `cmake -S . -B build && cmake --build build`
Expected: compilation fails because boundary contracts are absent.

- [ ] **Step 3: Implement the minimum behavior**

Define explicit platform-provider interfaces, virtual device fixtures, privacy modes (`LocalOnly`, `OptInDiagnostics`), schema/version metadata, and capability presentation records.

- [ ] **Step 4: Verify the focused pass**

Run: `cmake -S . -B build && cmake --build build`
Expected: Core boundary contracts compile.

- [ ] **Step 5: Run the affected integration check**

Run: `cmake --build build`
Expected: all Core targets compile.

- [ ] **Step 6: Commit the passing deliverable**

```bash
git add src/MouseEngine.Core/include/MouseEngine/Core/PlatformBoundary.h src/MouseEngine.Core/include/MouseEngine/Core/Simulation.h src/MouseEngine.Core/include/MouseEngine/Core/Privacy.h src/MouseEngine.Core/include/MouseEngine/Core/CompatibilitySchema.h docs/CORE_ARCHITECTURE.md ARCHITECTURE.md docs/CORE_CONTRACTS.md tests/MouseEngine.Core.Tests/boundary_contracts.cpp
git commit -m "docs: finalize phase zero architecture boundaries"
```

## Unresolved Product Decisions

- Whether cloud knowledge synchronization is enabled by default remains a product decision; the architecture supports local-only operation and optional synchronization.
- The final Windows UI framework remains WinUI 3 as the planned target, but the Core contracts deliberately do not depend on it.
- Firmware flashing remains excluded from v1; future support requires a separate validated security model.
