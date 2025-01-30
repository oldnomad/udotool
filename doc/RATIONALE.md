# Design choices rationale

## Why UINPUT?

My inspiration, `xdotool`, used X11-specific libraries. This was OK at
the time, but since Wayland has almost completely replaced X11, `xdotool`
doesn't work on modern systems any more.

Meanwhile, modern Linux kernels since version 4.5 implement a new feature,
virtual input device (`/dev/uinput`). This allows me to implement a
universal solution that works not only in X11 and Wayland, but even in
console.

On the other side, this solution has certain drawbacks:

- It's impossible to implement `xdotool` features related to window
  manager functions, such as sending keystrokes to specific window.
- It's impossible to implement `xdotool` features that depend on
  keyboard layout, such as command `type`.
- It's difficult to implement `xdotool` features that require getting
  system information, such as command `getmouselocation`.

## Why LIBINPUT quirk?

On one hand, I want to make a universal tool that can emulate any sort
of complex input. On the other hand, `udev`/`libinput` is the most common
environment in which this tool will be used, so I'll need to adapt to
its behavior.

`LIBINPUT` quirk seems like a good compromise: we sacrifice some flexibility
by refusing to implement 16 digitizer tools, and for that we get correct
behavior of `libinput` without writing a custom `udev` rule.

## Why Jim Tcl?

When I started initial version of `udotool`, I didn't want to implement
any scripts. The idea was to use external shell for all control, and my
tool will only emit input events, in a true Unix way.

Soon enough, it occurred to me that some control flow instructions needed
for input emitaion scenarios, such as "repeat this block for certain time",
are non-trivial in most shells. So, I started implementing my own script
language in `udotool`.

My requirements were:

- User shouldn't learn a new syntax just to write a trivial script.
  The simplest form of a script should be a sequence of commands, each
  in its separate lines, with no other separators, with arguments separated
  by whitespace.
- However, for complex scripts we need not only an ability to call external
  commands, but also some control flow commands (conditions, loops, and
  so on) and variable substitution.

My own script language was becoming more and more cumbersome, especially
since I didn't have a clear plan in my head. It became clear, that it would
be better to throw it all away, and use some existing embeddable script
language instead.

My requirements, listed above, excluded the most popular choice (Lua), so
I looked at other embeddable languages, and that's how I focused on Tcl.
It's not very well known these days, but it's an aged language, and aging
makes good things (like cheese and wine and programming languages) even
better.

Of course, I didn't want to pull in the whole Tcl/Tk infrastructure,
that's too much for a simple tool like `udotool`. So I chose
[Jim's Tcl](https://jim.tcl.tk/) since it's (a) small; and (b) included
in standard Debian repositories.
