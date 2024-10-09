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
echo "Move mouse around and double-click"
sleep 5
position 10 12.5
loop 20
    move +16 +16
    key -repeat 2 BTN_LEFT
endloop
```

**WARNING:** **udotool** is currently under active development, so
script language may change even between minor versions. Script language
is expected to become stable in version **2.0**.

## Building and installing

Build-time dependencies are:

- `gcc(1)`, `glibc(7)`, `make(1)`, `install(1)`, and some common POSIX
  commands are used in the build process.
- `pandoc(1)` is needed for building the manpage.
- Package `debhelper` is needed to build a Debian package.
- `git(1)` and `envsubst(1)` (package `gettext-base` in Debian) are used to
  determine program version and generate file `src/config.h`.
- Linux kernel headers (package `linux-libc-dev` in Debian) are needed for
  `uinput` constants.

Building just the binary and the manpage, and installing them:

```sh
make all
sudo make prefix=/usr/local install
```

Building Debian package:

```sh
make package
```

## Quirks and tweaks

Some sections of the code are guarded by conditional compilation controlled
by preprocessor defines. Such sections may be switched on or off by passing
to `make` a list of quirk names (in variable `QUIRKS`) and a list of tweak
names (in variable `TWEAKS`). The difference between quirks and tweaks is
that quirks change visible program behavior, while tweaks are just internal
changes.

Following quirks are defined at the moment:

- `LIBINPUT` (on by default): for reasons related to how `libinput` guesses
  input device type, buttons with values `0x140` to `0x14f` (`BTN_TOOL_PEN`
  to `BTN_TOOL_QUADTAP`), which are used by tablets (digitizers) and
  touchscreens, are disabled. See [separate document](doc/QUIRK-LIBINPUT.md).

Following tweaks are defined at the moment:

- `NOMMAN`: by default, we use `memfd_create(2)` to create a temporary file
  in memory. If this tweak is defined, we use `open(2)` with Linux-specific
  flag `O_TMPFILE` instead. Both variants are available on Linux only, and
  the second one has higher chances to fail, especially if path `/tmp` is
  mounted on some unsupported file system. However, it also has higher
  chances to work when the system is memory-constrained.

## Compatibility notes

- This program uses `/dev/uinput` device, available only in Linux. It's
  not intended to be useful on other OSes.
- While it's declared that this program is compatible with Linux kernels
  from 4.5 onwards, it never was actually tested on old kernels. If it
  breaks on your kernel, please file a bug.

## Acknowledgements and thanks

- This tool was inspired by [xdotool](https://github.com/jordansissel/xdotool)
  by [Jordan Sissel](https://github.com/jordansissel).
