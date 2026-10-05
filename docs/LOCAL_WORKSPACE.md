# Mouse Engine Local Workspace

Mouse Engine is local-first. A user account is optional and is never required to use the application, inspect a mouse, run diagnostics, or keep presets locally.

## Storage contract

User-owned workspace:

`%USERPROFILE%\\Documents\\Mouse Engine`

The workspace contains:

- `workspace.json` — workspace manifest and schema version.
- `Devices/` — device passports and device metadata.
- `Presets/` — user presets.
- `Profiles/` — application/device profiles.
- `Sessions/` — observation sessions.
- `Diagnostics/` — diagnostic data.
- `Reports/` — generated reports.
- `Experiments/` — controlled experiments.
- `Backups/` — workspace backups.

Infrastructure/cache:

`%LOCALAPPDATA%\\Mouse Engine`

The cache may contain WebView2 data and other disposable application state. It is not the authoritative home of user-owned presets, sessions, diagnostics, or reports.

## Cloud

When the user signs in, cloud synchronization is an additional copy/sync layer over the local workspace. Local data remains available.

The product guarantees:

1. An account is optional.
2. Cloud synchronization is optional.
3. Local workspace data remains usable offline.
4. Signing out does not delete the local workspace.
5. User-owned files can be copied or moved independently.
6. Cloud data is never the only copy required for normal local operation.

## Workspace schema

The initial manifest uses schema version `1` and type `mouse-engine-workspace`.

Future schema changes must use explicit migrations rather than silently changing file semantics.

## Observation sessions

Observation sessions use an explicit 50 ms idle-gap threshold for activity segmentation. An active run is a sequence of observed packets whose inter-arrival gaps remain below that threshold. A gap at or above the threshold starts a new run; it is not by itself a hardware-failure diagnosis.

Session analysis records per-stream packet counts, timing summaries, active-run counts, longest active run and current-run duration. Timing irregularities are evidence records with an explicit method and scope rather than generic fault claims.
