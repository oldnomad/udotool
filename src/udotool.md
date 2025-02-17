% udotool(1) | User Commands
%
% "July 18 2024"

# NAME

udotool - emulate input events

# SYNOPSIS

**udotool** [_options_] _cmd_ [_arg_...]

**udotool** [_options_] {**-i** | **\-\-input**} [_file_]

**udotool** [{**-h** | *\-\-help**} | {**-V** | **\-\-version**}]

# DESCRIPTION

**udotool** lets you simulate input events (keyboard input, mouse movement
and clicks, and more complex input). It does this using Linux **uinput**
driver.

Since **uinput** is a kernel feature, **udotool** should work in X11, in
Wayland, and even in console.

# OPTIONS

The program follows the usual GNU command line syntax, with long options
starting with two dashes ('-'). A summary of options is included below.

**-i** _file_, **\-\-input** [_file_]
:   Read commands from a file or from standard input, instead of using
 the command line. File name **-** (single minus sign) can be used for
 standard input. If file name is omitted (for long option only), the default
 is to use standard input.

**-n**, **\-\-dry-run**
:   Do not execute input emulation commands. Generic commands will be executed anyway.

**\-\-settle-time** _time_
:   Use specified settle time (default is 0.5 seconds).

**\-\-dev** _dev-path_
:   Use specified UINPUT device. Default is **/dev/uinput**.

**\-\-dev-name** _name_
:   Use specified emulated name. Default is **udotool**.

**\-\-dev-id** _vendor-id_[**:**_product-id_[**:**_version_]]
:   Use specified emulated device ID. Default is **0x0000:0x0000:0**.

**\-\-quirk** [+|-]_name_,...
:   Set on or off emulation quirk flags. Default is **+libinput**. See section
 **QUIRK FLAGS** below for a list of known quirk flags.

**-v**, **\-\-verbose**
:   Print more debug messages. Adding multiple **-v** will increase the verbosity.

**-h**, **\-\-help**
:   Show summary of options and exit.

**-V**, **\-\-version**
:   Show version of program and exit.

# COMMANDS

`udotool` uses Jim Tcl, a small-footprint dialect of Tcl.
Documentation for Jim Tcl is available at <https://jim.tcl.tk/>.
Documentation for Tcl language is available at <https://www.tcl-lang.org/>.

`udotool` extends the language with commands listed below.

## Generic commands

**names** _topic_
:   Return a list of all known axes (for topic "axis") or keys
 (for topic "key"). Each element of the list is a pair of a name
 and a code.

**timedloop** _seconds_ [_num_] [_vartime_] [_varnum_] _body_
:   Execute _body_ for at least _seconds_ time, but no more than
 _num_ times (if specified). If _vartime_ is specified and not
 an empty string, variable with this name will contain time
 elapsed since start of loop. If _varnum_ is specified and not
 an empty string, variable with this name will contain number
 of already executed iterations.

## Input emulation commands

**key** [**-repeat** _num_] [**-time** _seconds_] [**-delay** _seconds_] _key_...
:   Emulate keys/buttons being pressed down (in order) and then released
 (in reverse order). If option **-repeat** is specified, the whole
 sequence will be repeated no more than _num_ times. If option **-time**
 is specified, the whole option will be repeated for no more than
 specified time. If **-delay** option is specified, its value will be used
 as a delay between each repetition (default is **0.05**, that is,
 50 milliseconds). See also section **KEY NAMES** below.

**keydown** _key_...
:   Emulate key/button being pressed down. If several keys are specified,
 events will be emulated in this sequence. See also section **KEY NAMES** below.

**keyup** _key_...
:   Emulate key/button being released. If several keys are specified,
 events will be emulated in this sequence. See also section **KEY NAMES** below.

**move** [**-r**] _delta-x_ [_delta-y_ [_delta-z_]]
:   Emulate moving pointer by specified delta. This command usually
 uses axes **REL_X**, **REL_Y**, and **REL_Z**, but if option **-r**
 is specified, axes **REL_RX**, **REL_RY**, and **REL_RZ** are used
 instead. See also section **VALUE UNITS** below.

**wheel** [**-h**] _delta_
:   Emulate turning mouse wheel (or horizontal wheel if option **-h**
 is specified) by specified delta. See also section **VALUE UNITS** below.

**position** [**-r**] _abs-x_ [_abs-y_ [_abs-z_]]
:   Emulate moving pointer to specified absolute position. This command
 usually uses axes **ABS_X**, **ABS_Y**, and **ABS_Z**, but if option
 **-r** is specified, axes **ABS_RX**, **ABS_RY**, and **ABS_RZ** are
 used instead. See also section **VALUE UNITS** below.

## Low-level input emulation commands

**open**
:   Ensure that the input device is open and initialized.
 If this command was not specified, initialization happens
 before the first emulation command. Note that initialization
 takes some time (settle time).

**input** {*axis***=***value* | **KEYDOWN=***key* | **KEYUP=***key* | **SYNC**}...
:   Emulate a complex input message. This command allows
 you emulate a complex message that includes data for
 several axes and keys/buttons. This may be needed if you want to
 emulate, for example, some complex gamepad combo. See also sections
 **VALUE UNITS**, **AXIS NAMES** and **KEY NAMES** below.

**input** **{** *axis* *value* **}**...
:   This is an alternative syntax for **input** command. It interprets
 arguments not as strings, but as Tcl lists, each containing an axis name
 and a value. This syntax can be intermixed with the string-argument syntax,
 i.e. you can mix Tcl lists and "="-separated strings. However, this syntax
 can be used only in scripts.

## Variables and environment

`udotool` sets several global Tcl variables. Unless stated otherwise,
modifying these variables in the script has no effect on execution.

- **::udotool::debug** contains debug verbosity level. Modifying this
  variable may affect tracing Tcl commands, but has no effect on other
  debug messages.
- **::udotool::dry_run** is non-zero on dry run.
- **::udotool::device** contains UINPUT device path.
- **::udotool::dev_name** contains emulated device name.
- **::udotool::dev_id** contains emulated device ID.
- **::udotool::settle_time** contains device settle time (in seconds).
- **::udotool::default_delay** contains default delay between key/button
  events in command **key**. Modifying this variable affects all following
  commands.
- **::udotool::sys_name** contains virtual device directory name under
  **/sys/devices/virtual/input/**. It becomes available when
  emulation device is initialized.

# SCRIPTS

Commands for **udotool** can be given in a script file, using command
command-line option **-i** (**\-\-input**). Scripts can be also made
executable with a shebang syntax:

```
#!/usr/bin/udotool -i
puts "This script emulates 10 left mouse button clicks"
key -repeat 10 BTN_LEFT
```

**NOTE**: Since some systems don't properly split options in shebang
line, to combine several options you can use only short options:
**#!/usr/bin/udotool -vi** will work, but **#!/usr/bin/udotool -v -i**
probably won't.

# VALUE UNITS

Values for various axes are specified in abstract "units":

- Relative axes are usually integer:
  - Movement in major relative axes (**REL_X**, **REL_Y**, **REL_Z**)
    is usually interpreted in pixels.
  - Wheel movement (**REL_WHEEL** and **REL_HWHEEL**) is in notches.
    It can be fractional, with maximum resolution of **1/120** of a notch.
- Position in absolute axes is specified percents (**0.0** to **100.0**)
  of a maximum range. For example, if your screen has size **1920x1080**
  pixels (FHD), then position **25 33.3333** is at pixel **(480,360)**.
  - For axes that have no natural range (for example, **ABS_PROFILE**),
    maximum range is **1000000** (one million), so to get axis value
    **42** you have to specify it as **0.0042**.

# AXIS NAMES

Full list of supported axis names can be retrieved using command
**names axis**.

Following axis names are supported at the moment:

- Relative axes (mice, touchpads, gamepads, and similar):
  - **REL_X**, **REL_Y**, **REL_Z**: main axes used by most devices.
  - **REL_RX**, **REL_RY**, **REL_RZ**: additional axes (for example, rotation)
    used by some devices.
  - **REL_DIAL**: jog wheel on some devices.
  - **REL_MISC**: additional axis on some devices.
  - **REL_WHEEL**, **REL_HWHEEL**: mouse wheel and horizontal wheel axes.
- Absolute axes (touchscreens, tablets/digitizers, gamepads, joysticks, and similar):
  - **ABS_X**, **ABS_Y**, **ABS_Z**: main axes used by most devices.
  - **ABS_RX**, **ABS_RY**, **ABS_RZ**: additional axes (for example, rotation)
    used by some devices.
  - **ABS_THROTTLE**, **ABS_RUDDER**, **ABS_WHEEL**, **ABS_GAS**, **ABS_BRAKE**:
    additional axes used by some devices.
  - **ABS_HAT*n*X**, **ABS_HAT*n*Y** for *n* from 0 to 3: additional axes for
    secondary controls.
  - **ABS_PRESSURE**, **ABS_DISTANCE**, **ABS_TILT_X**, **ABS_TILT_Y**, **ABS_TOOL_WIDTH**:
    additional axes used by some digitizers.
  - **ABS_VOLUME**, **ABS_PROFILE**, **ABS_MISC**: special axes.

Additionally, command **input** accepts pseudo-axis names:

- **KEYDOWN**: axis value is a key or button to emulate a key/button press.
- **KEYUP**: axis value is a key or button to emulate a key/button release.
- **SYNC**: axis value is ignored, and a sync report (event border) is emulated.

If you have the device that you want to emulate, you can use **evtest**(1)
to determine which axes it uses.

# KEY NAMES

Full list of supported key names can be retrieved using command
**names key**. **WARNING**: The list is huge!

If the key you want to emulate is not in the list, you can specify
it as a number.

If you have the device that you want to emulate, you can use **evtest**(1)
to determine which keys it uses.

# QUIRK FLAGS

Following quirks are defined at the moment:

- `libinput` (on by default): for reasons related to how `libinput` guesses
  input device type, buttons with values `0x140` to `0x14f` (`BTN_TOOL_PEN`
  to `BTN_TOOL_QUADTAP`), which are used by tablets (digitizers) and
  touchscreens, are disabled.

# ENVIRONMENT

**UDOTOOL_SETTLE_TIME**
:   If set, this environment variable overrides default device settle time
 (in seconds). This value can be overridden by a command-line option.

**UDOTOOL_DEVICE_PATH**
:   If set, this environment variable overrides default UINPUT device path.
 This value can be overridden by a command-line option.

**UDOTOOL_DEVICE_NAME**
:   If set, this environment variable overrides default emulated device name.
 This value can be overridden by a command-line option.

**UDOTOOL_DEVICE_ID**
:   If set, this environment variable overrides default emulated device ID.
 This value can be overridden by a command-line option.

**UDOTOOL_QUIRKS**
:   If set, this environment variable contains quirk flags to set on or off.
 This value can be overridden by a command-line option.

# SEE ALSO

**evtest**(1)

# AUTHOR

Alec Kojaev <alec.kojaev@gmail.com>

# COPYRIGHT

Copyright © 2024 Alec Kojaev

Permission is granted to copy, distribute and/or modify this document under
the terms of the GNU General Public License, Version 3 or (at your option)
any later version published by the Free Software Foundation.

On Debian systems, the complete text of the GNU General Public License
can be found in /usr/share/common-licenses/GPL.
