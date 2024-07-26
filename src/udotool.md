% udotool(1) | User Commands
%
% "July 18 2024"

# NAME

udotool - emulate input events

# SYNOPSIS

**udotool** [_options_] _cmd_ [_arg_...]

**udotool** [_options_] {**-i** | **\-\-input**} _file_

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

**-i** _file_, **\-\-input** _file_
:   Read commands from a file instead of command line. File name **-**
 (single minus sign) can be used for standard input.

**-n**, **\-\-dry-run**
:   Do not execute input emulation commands. Generic commands will be executed anyway.

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
- Delays are specified in seconds and parts of seconds.
- All other numeric parameters are integer.

**NOTE**: Mouse wheel axes (**REL_WHEEL** and **REL_HWHEEL**) are
handled as high-resolution, so their values should be specified in
1/120 of a wheel notch.

## Generic commands

**sleep** _seconds_
:   Wait for specified time. Time is given in seconds, and may be
 fractional. Minimum delay is **0.001** (1 millisecond), maximum
 is **86400** (1 day).

**exec** _command_ [_arg_...]
:   Execute specified command. If command is does not include slashes,
 it will be searched in PATH. Environment for the command will
 include environment variable **UDOTOOL_SYSNAME**, which includes
 virtual device name under **/sys/devices/virtual/input/**.

**script** [{_file_ | **-**}]
:   Execute commands from specified file. If no file name is given or
 if the file name is a single minus sign (**-**), commands will be
 read from the standard input.

**help** [{_command_ | **-axis** | **-key**}...]
:   Print help on commands. If no arguments are specified, **help**
 will print description of all supported commands. Arguments
 **-axis** and **-key** will make **help** print lists of known
 axes and keys, correspondingly.

**loop** _num_
:   Execute commands between **loop** and corresponding **endloop**
 _num_ times. Loops may be embedded, but maximum stack depth is
 limited.

**endloop**
:   End corresponding **loop** block.

## Input emulation commands

**key** [**-repeat** _num_] [**-delay** _seconds_] _key_...
:   Emulate keys/buttons being pressed down (in order) and then released
 (in reverse order). If option **-repeat** is specified, the whole
 sequence will be repeated _num_ times. If **-delay** option is specified,
 its value will be used as a delay between each repetition (default is
 **0.05**, that is, 50 milliseconds). See also section **KEY NAMES** below.

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
:   Ensure that input device is open and initialized.
 If this command was not specified, initialization happens
 before the first emulation command.

**input** {*axis***=***value* | **KEYDOWN=***key* | **KEYUP=***key*}...
:   Emulate a complex input message. This command allows
 you emulate a complex message that includes data for
 several axes and keys/buttons. See also sections **VALUE UNITS**,
 **AXIS NAMES** and **KEY NAMES** below.

# SCRIPTS

Commands for **udotool** can be given in a script file, using command
**script** or command-line option **-i** (**\-\-input**). Scripts
can be also made executable with a shebang syntax:

```
#!/usr/bin/udotool -i
exec echo "This script emulates 10 left mouse button clicks"
key -repeat 10 BTN_LEFT
```

In scripts, each command should be in a separate line. There is no
syntax for breaking a command into several lines.

Empty lines are ignored. Leading and trailing whitespace is ignored.

Lines starting with hash (**#**) or semicolon (**;**), probably with
some whitespace before them, are treated as comments and ignored.

Command arguments may be enclosed in apostrophes (**\'...\'**) or
quotes (**\"...\"**). Inside quoted arguments, backslash character
escapes following character, so, for example, **\"A\\\"B\\\\\"** is
a 4-character string containing: character **A**, quote character,
character **B**, backslash character.

# VALUE UNITS

Values for various axes are specified in abstract "units":

- Movement in major relative axes (**REL_X**, **REL_Y**, **REL_Z**)
  is usually interpreted in pixels.
- Wheel movement is specified in **1/120** of a wheel notch.
  That is, value **+120** means scrolling the wheel by one notch.
  Note that, depending on your software, values below one notch
  might be not recognized.
- Position in absolute axes is specified in **1/1000000** (one
  millionth) of maximum range. For example, if your screen has
  size **1920x1080** pixels (FHD), then position **250000 333333**
  is at pixel **(480,360)**.

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

If you have the device that you want to emulate, you can use **evtest**(1)
to determine which axes it uses.

# KEY NAMES

Full list of supported key names can be retrieved using command
**help -key**. **WARNING**: The list is huge!

If the key you want to emulate is not in the list, you can specify
it as a number in decimal or (with prefix **0x**) hexadecimal.

If you have the device that you want to emulate, you can use **evtest**(1)
to determine which keys it uses.

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