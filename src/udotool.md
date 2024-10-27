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

**-i** [_file_], **\-\-input** [_file_]
:   Read commands from a file or from standard input, instead of using
 the command line. File name **-** (single minus sign) can be used for
 standard input. If file name is omitted, default is to use standard input.

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

**-v**, **\-\-verbose**
:   Print more debug messages. Adding multiple **-v** will increase the verbosity.

**-h**, **\-\-help**
:   Show summary of options and exit.

**-V**, **\-\-version**
:   Show version of program and exit.

# COMMANDS

Unless specified otherwise:

- Command and option names are case-sensitive.
- Axis and key names are case-insensitive.
- Time is specified in seconds and parts of seconds.
- Loop and repeat counters are integer.
- For other values see section **VALUE UNITS** below.

Environment available to scripts and to all commands invoked
from a script contains all environment variables available to
`udotool` itself, plus following variables:

- **$UDOTOOL_SYSNAME** contains virtual device directory name under
  **/sys/devices/virtual/input/**. It becomes available when
  emulation device is initialized.
- **$UDOTOOL_LOOP_COUNT** in a loop contains remaining number of
  iterations. If the loop is not limited by iteration count, initial
  number of iterations is **INT_MAX**.
- **$UDOTOOL_LOOP_RTIME** in a loop contains remaining time (in seconds
  and parts of seconds). If the loop is not limited by time, the
  variable contains single asterisk character.

## Generic commands

**sleep** _seconds_
:   Wait for specified time. Time is given in seconds, and may be
 fractional. Minimum delay is **0.001** (1 millisecond), maximum
 is **86400** (1 day).

**exec** [**-detach**] _command_ [_arg_...]
:   Execute specified command. If option **-detach** is specified,
 the command will be executed in a separate session. If _command_
 does not include slashes, it will be searched in PATH.

**echo** _arg_...
:   Print specified arguments to standard output.

**set** _name_ [_value_]
:   Set environment variable _name_ to value _value_, or unset it
 if _value_ is not specified.

**script** [{_file_ | **-**}]
:   Execute commands from specified file. If no file name is given or
 if the file name is a single minus sign (**-**), commands will be
 read from the standard input.

**help** [{_command_ | **-axis** | **-keys**}...]
:   Print help on commands. If no arguments are specified, **help**
 will print description of all supported commands. Arguments
 **-axis** and **-keys** will make **help** print lists of known
 axes and keys, correspondingly.

**loop** [**-time** _seconds_] [_num_] &lt;NL&gt; ... &lt;NL&gt; **end**
:   Execute lines between **loop** and corresponding **end**
 no more than _num_ times. If option **-time** is specified, the
 loop will be repeated for no more than specified time.

**if** _cond_ &lt;NL&gt; ... &lt;NL&gt; [**else** &lt;NL&gt; ... &lt;NL&gt;] **end**
:   Execute lines conditionally. If _cond_ is true, execute the
 block of lines after **if**, otherwise skip it, and execute the
 block of lines after **else** (if specified). See section
 **CONDITIONS** below for syntax of conditions.

**exit**
:   Terminate execution of current script.

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

**position** [**-r**] _delta-x_ [_delta-y_ [_delta-z_]]
:   Emulate moving pointer to specified absolute position. This command
 usually uses axes **ABS_X**, **ABS_Y**, and **ABS_Z**, but if option
 **-r** is specified, axes **ABS_RX**, **ABS_RY**, and **ABS_RZ** are
 used instead. See also section **VALUE UNITS** below.

## Low-level input simulation commands

**open**
:   Ensure that the input device is open and initialized.
 If this command was not specified, initialization happens
 before the first emulation command. Use this command if you have
 an **exec** before the first emulation command, and you want the
 device to be present when the external command is executed.

**input** {*axis***=***value* | **KEYDOWN=***key* | **KEYUP=***key*}...
:   Emulate a complex input message. This command allows
 you emulate a complex message that includes data for
 several axes and keys/buttons. This may be needed if you want to
 emulate, for example, some complex gamepad combo. See also sections
 **VALUE UNITS**, **AXIS NAMES** and **KEY NAMES** below.

# SCRIPTS

Commands for **udotool** can be given in a script file, using command
**script** or command-line option **-i** (**\-\-input**). Scripts
can be also made executable with a shebang syntax:

```
#!/usr/bin/udotool -i
echo "This script emulates 10 left mouse button clicks"
key -repeat 10 BTN_LEFT
```

**NOTE**: Since some systems don't properly split options in shebang
line, to combine several options you can use only short options:
**#!/usr/bin/udotool -vi** will work, but **#!/usr/bin/udotool -v -i**
probably won't.

In scripts, each command should be in a separate line. There is no
syntax for breaking a command into several lines.

Empty lines are ignored. Leading and trailing whitespace is ignored.

Lines starting with hash (**#**) or semicolon (**;**), optionally with
some whitespace before them, are treated as comments and ignored.

Command lines are subject to POSIX-compatible word expansion and
quote removal. That means that you can use all features (single quote,
double quote, command substitution, and so on) available in standard
POSIX-compatible shell.

# CONDITIONS

Command **if** supports condition expressions with following
syntax (EBNF):

```
<condition>  := <or-term> "-or" <condition> | <or-term>
<or-term>    := <and-term> "-and" <or-term> | <and-term>
<and-term>   := <comparison>
<comparison> := <value> CMP_OP <value> | <value>
<value>      := "-not" <value> | "(" <condition> ")" | NUMBER
```

Here _NUMBER_ is a floating-point or integer number, and _CMP_OP_ is
one of: "-eq", "-ne", "-lt", "-gt", "-le", or "-ge".

For example:

```
if -not '(' "$a" -eq 1 -and "$b" -gt 2 ')' -or "$b" -gt 3
  echo "Either $b is greater then 3, or it's not true that $a is 1 and $b is greater than 2"
end
```

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
**help -axis**.

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

If you have the device that you want to emulate, you can use **evtest**(1)
to determine which axes it uses.

# KEY NAMES

Full list of supported key names can be retrieved using command
**help -keys**. **WARNING**: The list is huge!

If the key you want to emulate is not in the list, you can specify
it as a number in decimal or (with prefix **0x**) hexadecimal.

If you have the device that you want to emulate, you can use **evtest**(1)
to determine which keys it uses.

# ENVIRONMENT

**UDOTOOL_SYSNAME**
:   This environment variable is set for commands called from a **udotool**
 script and contains virtual device directory name under
 **/sys/devices/virtual/input/**. It becomes available when emulation device
 is initialized.

**UDOTOOL_LOOP_COUNT**
:   This environment variable is set for commands called from a **udotool**
 script in a loop. It contains remaining number of iterations for the loop.
 If the loop is not limited by iteration count, initial number of iterations
 is **INT_MAX**.

**UDOTOOL_LOOP_RTIME**
:   This environment variable is set for commands called from a **udotool**
 script in a loop. It contains remaining iteration time for the loop
 (in seconds and parts of seconds). If the loop is not limited by time, the
 variable contains single asterisk character.

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
