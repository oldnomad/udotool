# UDOTOOL: Simulate input events in Linux

**udotool** is a tool to emulate input events in Linux. It uses
kernel `uinput` driver, which allows userspace program to emulate
an input device.

Since the program uses a kernel feature, it works in any environment:
X11, Wayland, even console. It can emulate keyboard input,
mouse/trackball/touchpad movement and clicks, various gamepads, etc.

**udotool** also supports reading commands from files or standard
input, so it can be used to create input-emulating scripts:

```
#!/usr/bin/udotool -i
puts "Move mouse around and double-click"
sleep 5
position 10 12.5
loop i 20 {
    move +16 +16
    key -repeat 2 BTN_LEFT
}
```

Script language used in **udotool** is [Jim Tcl](http://jim.tcl.tk/),
with some additional commands.

## Building and installing

Build-time dependencies are:

- `gcc(1)`, `glibc(7)`, `make(1)`, `install(1)`, and some common POSIX
  commands are used in the build process.
- `xxd(1)` is needed for building.
- `pandoc(1)` is needed for building the manpage.
- Package `debhelper` is needed to build a Debian package.
- `git(1)` and `envsubst(1)` (package `gettext-base` in Debian) are used to
  determine program version and generate file `src/config.h`.
- Linux kernel headers (package `linux-libc-dev` in Debian) are needed for
  `uinput` constants.
- [Jim Tcl](http://jim.tcl.tk/) headers and library (packages `libjim-dev`
  and `libjim0.81` in Debian) are needed for building. The library is also
  used at run time.

Building just the binary and the manpage, and installing them:

```sh
make all
sudo make prefix=/usr/local install
```

Building Debian package:

```sh
make package
```

## Emulation quirks

Some details of input emulation are controlled by flags called "quirks".
They can be switched on or off by specifying command line option `--quirk`,
or by setting environment variable `UDOTOOL_QUIRKS`.

Following quirks are defined at the moment:

- `libinput` (on by default): for reasons related to how `libinput` guesses
  input device type, buttons with values `0x140` to `0x14f` (`BTN_TOOL_PEN`
  to `BTN_TOOL_QUADTAP`), which are used by tablets (digitizers) and
  touchscreens, are disabled. See [separate document](doc/QUIRK-LIBINPUT.md).

## Compatibility notes

- This program uses `/dev/uinput` device, available only in Linux. It's
  not intended to be useful on other OSes.
- While it's declared that this program is compatible with Linux kernels
  from 4.5 onwards, it never was actually tested on old kernels. If it
  breaks on your kernel, please file a bug.
- The program is built with Jim Tcl version 0.81, however, it should
  probably work with later versions too.

## Acknowledgements and thanks

- This tool was inspired by [xdotool](https://github.com/jordansissel/xdotool)
  by [Jordan Sissel](https://github.com/jordansissel).
- This tool uses [Jim Tcl](http://jim.tcl.tk/), an opensource small-footprint
  implementation of the Tcl programming language, originally written by
  Salvatore "antirez" Sanfilippo and currently supported by Steve Bennett.
