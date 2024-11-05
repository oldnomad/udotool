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

## Why your own script language?

For reasons explained above, I cannot simply reimplement `xdotool` script
language. So I'll need a new set of commands anyway.

Why cannot I use some existing script language? Well, I can, of course.
But I want to preserve several feaures:

- I want to minimize grammar requirements for widely-used scenarios. In
  the simplest case of emulating some button presses and mouse clicks I
  don't want to force user to learn the script language. Simple command
  invocation should mean just writing the command and its arguments,
  separated by whitespace.
- I want to minimize dependencies. User shouldn't pay for features that
  they use once in a blue moon.
- At the same time, I want to be able to emulate some complex scenarios
  that may depend on external commands. These scenarios may include
  conditional blocks (so I need `if/else`) and non-trivial repetitions
  (so we need some sort of loop statement).

If someone points me to an embeddable script language that satisfies these
constraints, I'll be more than happy to consider it.

## Why LIBINPUT quirk?

On one hand, I want to make a universal tool that can emulate any sort
of complex input. On the other hand, `udev`/`libinput` is the most common
environment in which this tool will be used, so I'll need to adapt to
its behavior.

`LIBINPUT` quirk seems like a good compromise: we sacrifice some flexibility
by refusing to implement 16 digitizer tools, and for that we get correct
behavior of `libinput` without writing a custom `udev` rule.
