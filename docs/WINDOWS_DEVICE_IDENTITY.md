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

Static contract checks currently cover the SetupAPI symbols, snapshot schema, UI validation, and mutation boundary.

A Windows runner with the current branch must still perform the authoritative compile/link/runtime verification. Physical hardware verification remains separate from CI.
