# UDOTOOL commands

This document contains a description of `udotool` commands more detailed
than the man page.

## Common elements

Following rules apply throughout all commands:

- Any parameter can be enclosed in double quotes (`"..."`)
  or apostrophes (`'...'`). Backslash (`\`) can be used to
  escape any character within a quoted string, but special
  escape sequences (like `"\n"`) are not supported.
- All delays and times are specified in seconds. Minimum
  time is **0.001** (1 millisecond), maximum is **86400** (1 day).
- Values for relative axes are specified in units, usually
  corresponding to pixels. However, there are some exceptions:
  - Wheel axes (`REL_WHEEL` and `REL_HWHEEL`) are in notches. Since
    high-resolution wheel reporting is supported, wheel values may
    include fractional part, but its interpretation depends on the
    software.
  - Dial axis (`REL_DIAL`), used for jog wheel on some devices,
    is specified in dial notches.
  - Units for axis `REL_MISC` are unspecified.
- Values for absolute axes are specified in percents of full axis
  range. For axes where exact value is desired (such as `ABS_PROFILE`),
  full range is `1000000` (one million), so `1` corresponds to
  exact value `10000` (ten thousand), and `0.0042` corresponds to
  exact value `42`.

## Generic commands

Generic commands are not related to input emulation directly. They are
either informational (like `help`), or allow to write more complex
emulation scripts.

### SLEEP

Syntax: `sleep <DELAY>`

Stop script execution for specified number of seconds.

### EXEC

Syntax: `exec [-detach] <COMMAND> [<ARG>...]`

Execute specified command with specified arguments.

Note that all arguments will be passed as-is. In particular,
no shell expansion (variable expansion, word expansion, etc)
will be performed. If you need some sort of expansion, use
`sh -c`.

The command can be a file path (containing slashes), or a plain
command. If a plain command is specified, it will be searched
in `$PATH`.

If option `-detach` is specified, the command will be executed
in background in a separate session.

The command will receive the same environment variables as
`udotool` itself, with addition of `$UDOTOOL_SYSNAME` variable,
which contains virtual device directory name under
`/sys/devices/virtual/input/`. However, this additional
environment variable is only set when the `uinput` device is
created, so if you need it before any input emulation command,
use `open`:

```
#!/usr/bin/udotool -i
# Following will not print anything, the device isn't created yet
exec sh -c "echo $UDOTOOL_SYSNAME"
# You need either an input emulation command, or `open`
open
# Now we know the device name:
exec sh -c "echo $UDOTOOL_SYSNAME"
```

### SCRIPT

Syntax: `script <FILE>`

Run commands from specified file.

Special file name consisting of one minus sign (`"-"`) can be
used for reading commands from standard input.

When used on command line, `script` is equivalent to option
`--input` (`-i`). That is, `/usr/bin/udotool script my-file`
is the same as `/usr/bin/udotool -i <my-file`.

When a script is called from another script, any error in the
called script results in an error in the caller script.

### HELP

Syntax: `help [<TOPIC>]`

Prints help on a topic. The topic can be a command name
(e.g. `help exec`), or one of special topics starting with
a minus sign.

Special topics are:

- `-axis`: Prints a list of all known axes, both relative and
  absolute. Both axis name and axis code are printed.
- `-keys`: Prints a list of all known keys and buttons.
  Both key/button name and its code are printed.

If no topic is specified, `help` prints help on all known
commands.

### LOOP/ENDLOOP

Syntax: `loop [-time <TIME>] [<NUMBER>] ... endloop`

This pair of commands creates a loop of commands.

The loop may have a number of repetitions, a repetition time,
or both.

- If only number of repetitions is specified, the loop
  will be repeated specified number of times.
- If only repetition time is specified, the loop will be
  repeated until the time is exhausted. Note that time is
  checked at the end of an iteration, so real time of
  loop execution can be longer than specified.
- If both number of repetitions and repetition time are
  specified, both limits are checked. That is, the loop
  will be repeated no more than specified number of times,
  and it will be stopped if repetition time is exhausted.

In any case, the loop body will be executed at least once.

For example:

```
loop -t 3 70
    key BTN_LEFT
endloop
```

This will emulate left mouse button clicks for 3 seconds,
but no more than 70 clicks. Since the default delay between
clicks is 50 milliseconds, actual number of clicks will
be at most 60.

## High-level input commands

High-level input commands emulate input from common input
devices (such as keyboard or mouse).

### KEY

Syntax: `key [-repeat <NUMBER>] [-time <TIME>] [-delay <DELAY>] <KEY>...`

Emulate pressing and then releasing specified keys/buttons.

For example, `key -repeat 300 -time 10 -delay 0.03 KEY_LEFTCTRL KEY_Q`
is equivalent of:

```
loop -time 10 300
    keydown KEY_LEFTCTRL KEY_Q
    keyup   KEY_Q KEY_LEFTCTRL
    sleep   0.03
endloop
```

### KEYDOWN

Syntax: `keydown <KEY>...`

Emulate pressing specified keys/buttons.

### KEYUP

Syntax: `keyup <KEY>...`

Emulate releasing specified keys/buttons.

### MOVE

Syntax: `move [-r] <DELTA-X> [<DELTA-Y> [<DELTA-Z>]]`

Emulate moving a mouse or a 3D pointer with relative axes.

Usually this command uses relative axes `REL_X`, `REL_Y`, and `REL_Z`.
However, if option `-r` is used, it will use axes `REL_RX`, `REL_RY`,
and `REL_RZ`.

### POSITION

Syntax: `position [-r] <ABS-X> [<ABS-Y> [<ABS-Z>]]`

Emulate positioning on a touchscreen or a digitizer with absolute axes.

Usually this command uses absolute axes `ABS_X`, `ABS_Y`, and `ABS_Z`.
However, if option `-r` is used, it will use axes `ABS_RX`, `ABS_RY`,
and `ABS_RZ`.

### WHEEL

Syntax: `wheel [-h] <DELTA>`

Emulate turning mouse wheel.

Usually this command uses relative axis `REL_WHEEL`. However, if option
`-h` is used, it will use axis `REL_HWHEEL` (horizontal wheel).

## Low-level input commands

Low-level input commands are main building blocks from which
high-level input commands are built. You can use them directly if
you want to emulate some complex input devices.

### OPEN

Syntax: `open`

Create emulation device.

Typically, emulation device is created when the first input command
is executed. However, you might want to create it explicitly with this
command in following cases:

- If you want to execute an external command using `exec` before any
  input commands, and you want to use environment variable `$UDOTOOL_SYSNAME`.
- If you want to give your software some time to detect new device
  being attached before emulating input.

### INPUT

Syntax: `input <AXIS>=<VALUE>...`

Emulate input report.

This is the main building block for all other input emulation.
This command emulates an input event report that includes several axes
and/or keys/buttons. All values specified in this command are sent in
a single input event report, so each axis must be specified at most
once.

Axis can be any relative or absolute axis name, or one of special values:

- `KEYDOWN`: value is interpreted as a key/button to report being pressed.
- `KEYUP`: value is interpreted as a key/button to report being released.

For example:

```
input ABS_X=50 ABS_Y=25 KEYDOWN=BTN_DPAD_LEFT
```

This emulates a report from a gamepad with left analog stick being tilted
up and left button on D-pad being pressed.
