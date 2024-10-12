# UDOTOOL commands

This document contains a description of `udotool` commands more detailed
than the man page.

## Common elements

Following rules apply throughout all commands:

- Lines starting with `#` or `;` are treated as comments and skipped.
- Each line is subject to POSIX-compatible word expansion (tilde expansion,
  parameter expansion, command substitution, arithmetic expansion, and so on),
  field splitting, pathname expansion, and quote removal. For details see
  [POSIX.1 standard](https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/),
  section **System Interfaces** [wordexp()](https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/functions/wordexp.html)
  and section **Shell & Utilities**
  [2 Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/utilities/V3_chap02.html#tag_19_06),
  subsections **2.6 Word Expansion** and **2.2 Quoting**.
  See also caveats below.
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

When using word expansion in scripts, you should remember some caveats:

- In `glibc` version 2.40 (and earlier), `wordexp(3)` doesn't yet support
  "dollar-single-quotes" quoting style, introduced in POSIX.1-2024.
- In `glibc` version 2.40 (and earlier), `wordexp(3)` arithmetic expansion
  supports only 4 basic arithmetic operations (`+`, `-`, `*`, and `/`).
- `wordexp(3)` follows shell expansion rules. That means that characters
  in set `"|&;<>(){}"`  should be properly escaped or quoted.
- If word expansion produces newline characters, they will _not_ be treated
  as line separators, but as regular whitespace.
- If word expansion produces new flow control commands (`loop`, `if`, `else`,
  or `end`), result is _undefined_. In particular, this may result in syntax
  error.

Environment available to `udotool` script and to all commands invoked
from it contains all environment variables available to `udotool` itself,
plus following variables:

- `UDOTOOL_SYSNAME` contains virtual device directory name under
  `/sys/devices/virtual/input/`. This additional environment variable
  becomes available only when the `uinput` device is created, so if you
  need it before any input emulation command, you should use `udotool`
  command `open` (see below).

### Condition expressions

Command `if` supports condition expressions which may include parenthesized
expressions and following operators (in order of decreasing precedence):

- `-not` for unary logical negation.
- `-lt`, `-gt`, `-le`, and `-ge` for numeric comparisons.
- `-eq` and `-ne` for equality comparisons.
- `-and` for logical conjunction.
- `-or` for logical disjunction.

Parameters of all operators should be numeric (floating-point or integer).
Arithmetic operators are not supported, use arithmetic expansion if you
need them.

When numbers occur in logical context, zero is false, and any non-zero
value is true.

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

The command can be a file path (containing slashes), or a plain
command. If a plain command is specified, it will be searched
in `$PATH`.

If option `-detach` is specified, the command will be executed
in background in a separate session.

### ECHO

Syntax: `echo <ARG>...`

Print all specified arguments to standard output.

### SET

Syntax: `set <NAME> [<VALUE>]`

Set environment variable `<NAME>` to value `<VALUE>`, or unset it
if `<VALUE>` is omitted.

Changes in the environment become available to all commands executed after
this.

### SCRIPT

Syntax: `script <FILE>`

Run commands from specified file.

Special file name consisting of one minus sign (`"-"`) can be
used for reading commands from standard input.

When used on command line, `script` is equivalent to option
`--input` (`-i`). That is, `/usr/bin/udotool script my-file`
is the same as `/usr/bin/udotool -i my-file`.

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

## Flow control commands

### LOOP

Syntax: `loop [-time <TIME>] [<NUMBER>] <NL> ... <NL> end`

This command creates a loop of command lines.

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
end
```

This will emulate left mouse button clicks for 3 seconds,
but no more than 70 clicks. Since the default delay between
clicks is 50 milliseconds, actual number of clicks will
be at most 60.

### IF/ELSE

Syntax: `if <CONDITION> <NL> ... <NL> [else <NL> ... <NL>] end`

These commands create conditional blocks of commands.

The commands in the "if" block will be executed only if
`<CONDITION>` expression is true. See above for syntax of
condition expression.

The commands in the "else" block, if present, will be executed if
"if" block is not executed.

For example:

```
if "$XCURSOR_SIZE" -lt 24
    move "+$XCURSOR_SIZE" 0
else
    move +24 0
endif
```

This will emulate moving the mouse `$XCURSOR_SIZE` pixels to the
right if environment variable `XCURSOR_SIZE` is less than 24,
otherwise the mouse will move 24 pixels to the right.

### EXIT

Syntax: `exit`

This command terminates execution of current script.

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
end
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
  input commands, and you want to use environment variable `UDOTOOL_SYSNAME`.
- If you want to give your software some time to detect new device
  being attached before emulating input.

```
#!/usr/bin/udotool -i
# Following will not print anything, the device isn't created yet
exec echo $UDOTOOL_SYSNAME
# You need either an input emulation command, or `open`
open
# Now we know the device name:
exec echo $UDOTOOL_SYSNAME
```

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

This emulates a report from a gamepad saying that its left analog stick
is tilted up, and its left D-Pad button is pressed.
