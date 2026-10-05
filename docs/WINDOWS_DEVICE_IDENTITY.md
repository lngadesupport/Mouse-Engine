# Windows Device Identity

Mouse Engine separates device observation from identity resolution.

## Current read-only path

```text
Raw Input
  |
  +-- GetRawInputDeviceList
  +-- RIM_TYPEMOUSE
  +-- RIDI_DEVICENAME
  |
  v
Windows HID interface path
  |
  v
SetupAPI
  +-- SetupDiGetClassDevsW
  +-- SetupDiEnumDeviceInterfaces
  +-- SetupDiGetDeviceInterfaceDetailW
  +-- SetupDiGetDeviceInstanceIdW
  +-- DEVPKEY_Device_ContainerId
  +-- SPDRP_HARDWAREID
  +-- SPDRP_MFG
  +-- SPDRP_DEVICEDESC
  |
  v
read-only identity observation
  |
  v
WebView2 snapshot
```

## Bus topology evidence

For each resolved HID interface, the host walks the Windows Configuration Manager parent chain.

Direct transport classification is deliberately conservative:

- a proven `USB\\` ancestor produces `Usb`;
- a proven `BTH\\`, `BTHENUM\\` or `BTHLEDEVICE\\` ancestor produces `Bluetooth`;
- otherwise the result is `Unknown`.

The host does **not** classify a device as 2.4 GHz merely because it is attached to a USB receiver. Receiver ownership requires separate correlation evidence.

Each resolved identity also receives a deterministic topology evidence hash so changes in ancestry can invalidate downstream identity assumptions.

## Identity fields

The host currently exposes, when available:

- Windows instance ID;
- Container ID;
- manufacturer;
- product/device description;
- VID;
- PID;
- whether the identity was resolved for the observed Raw Input interface.

The Raw Input interface path is used internally to correlate the active interface with the corresponding SetupAPI device interface. It is not exposed as a UI write target.

## USB endpoint evidence

For a USB-resolved mouse, the host also correlates the resolved device with an ancestor USB hub and the port reported by Windows location information. It queries `IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX` to obtain read-only connection data and the associated open pipes.

When an interrupt IN pipe exposes an endpoint descriptor, Mouse Engine records:

- connection index;
- USB device address;
- Windows-reported speed code;
- interrupt IN endpoint count;
- raw endpoint `bInterval` value and endpoint address.

`bInterval` is retained as **descriptor evidence**. Microsoft documents that it reflects the device configuration and is relative to bus speed; it is not itself a fixed time duration and does not prove an observed polling frequency.

The Latency Lab therefore keeps three independent states:

- **Configured** — unavailable until a distinct host scheduling/configuration evidence source exists;
- **Device reported** — raw USB endpoint descriptor evidence when available;
- **Observed** — unavailable until a real capture/measurement session produces timing samples.

No rate is synthesized from `bInterval`.

## Observed Raw Input timing

The Windows host registers the mouse usage page for background Raw Input delivery and records `WM_INPUT` arrival timestamps using `QueryPerformanceCounter`. Timing is accumulated independently for each Raw Input device path.

The native accumulator retains a bounded interval window and exposes:

- interval count;
- minimum inter-arrival interval;
- median inter-arrival interval;
- P95 inter-arrival interval;
- maximum inter-arrival interval;
- P95-minus-median spread as a simple jitter indicator.

The measurement scope is explicitly **WM_INPUT arrival inter-arrival**. It is therefore evidence about the host-observed input stream, not a direct electrical/USB bus polling measurement.

No conversion from the observed interval distribution into a claimed device polling rate is performed.

Snapshot schema is now version 2 because observed timing is a new externally visible evidence field.

## Evidence rules

VID/PID alone is not treated as physical identity.

The UI therefore does not infer:

- 2.4 GHz from USB receiver attachment;
- receiver ownership;
- vendor protocol;
- DPI support;
- polling-rate configuration;
- firmware capabilities.

Transport remains `undetermined` until topology or protocol evidence is available.

## Safety boundary

This phase is read-only.

It does not:

- open a HID handle for writes;
- send HID output reports;
- change device configuration;
- flash firmware;
- reset devices;
- claim exclusive access.

## Hotplug

`RIDEV_DEVNOTIFY` registers device-arrival/removal notifications. A notification triggers a fresh observation snapshot; the notification itself is not treated as proof of a specific device identity.

## Verification status

Static contract checks cover the SetupAPI symbols, USB hub/endpoint evidence path, snapshot schema, UI validation, timing-state separation, and mutation boundary. A Windows runner with the current branch must still perform the authoritative compile/link/runtime verification. Physical hardware verification remains separate from CI.

A Windows runner with the current branch must still perform the authoritative compile/link/runtime verification. Physical hardware verification remains separate from CI.
