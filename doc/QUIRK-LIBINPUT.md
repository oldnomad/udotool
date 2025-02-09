# How `udev` and `libinput` interact for input devices

Modern Linuxes, when new virtual input (`uinput`) device appears,
perform following steps:

1. Kernel creates new directory under `/sys/devices/virtual/input`,
   reporting declared device capabilities.
2. `udev` uses a set of heuristics to determine input device properties
   (see `src/udev/udev-builtin-input_id.c`), and creates a device
   node under `/dev/input` with these properties.
3. `libinput` checks device properties (method `evdev_configure_device()`
   in `src/evdev.c`) to configure the device.

Device created by `udotool` reports following capabilities:

- All relative axes.
- All absolute axes (except multitouch).
- All buttons and keys (including unassigned) up to `KEY_MAX` (`0x2ff`).

As a result of its heuristics, `udev` assigns following properties:

- `ID_INPUT` (assigned to all input devices)
- `ID_INPUT_TABLET` (since `BTN_STYLUS` and `BTN_TOOL_PEN` are enabled).
- `ID_INPUT_KEY` (since all `KEY_*` are enabled).
- `ID_INPUT_KEYBOARD` (since first 31 keyboard keys are enabled).

Using these properties, `libinput` detects this device as a tablet, and
consequently ignores all other device capabilities (even though `udev`
suggests keyboard + tablet).

There are several possible solutions for this problem (including defining
an `udev` rule for `udotool` device), but probably the easiest is to
disable buttons `BTN_STYLUS` and `BTN_TOOL_PEN` (and, probably, all other
tablet/digitizer buttons). This prevents `udev` from setting property
`ID_INPUT_TABLET`, and makes it set `ID_INPUT_MOUSE` instead (comments in
the source say that this is for "VMware USB mouse", whatever that may be).

Nevertheless, I can envision a situation where a user might want to enable
these buttons, even if this means losing `libinput` compatibility. It can
be done by disabling quirk flag `libinput` (see option `--quirk` and
environment variable `UDOTOOL_QUIRKS`).
