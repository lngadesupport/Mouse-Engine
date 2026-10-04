# MouseEngine.Core.Tests

Unit tests for the core contracts and safety invariants.

Initial invariants:
- incomplete fingerprints cannot be treated as exact model identity
- unsupported capabilities are never exposed as supported
- experimental mutations require snapshots
- measurements preserve units and evidence
- rollback remains available after a failed mutation
