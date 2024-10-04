# Mappings for various types of controllers

All mappings below are extracted from Linux kernel documentation.

## Mouse

- Mouse movement is reported on relative axes `REL_X` and `REL_Y`.
- There exists a case of "absolute axes mouse" (VMware), when mouse
  position is reported on absolute axes `ABS_X` and `ABS_Y`.
- Standard buttons are `BTN_LEFT` (left), `BTN_RIGHT` (right),
  and `BTN_MIDDLE` (middle or wheel click).
- There may be additional buttons, such as `BTN_SIDE`, `BTN_EXTRA`,
  `BTN_FORWARD`, `BTN_TASK`, and so on.
- Wheels are reported as axes `REL_WHEEL` (main/vertical wheel) and
  `REL_HWHEEL` (horizontal wheel), with values indicating number of
  notches moved. Note that this value can be non-integer, since we
  support high-resolution scrolling (with steps of 1/120 of a notch).

## Gamepad

Some controls can be digital (reported as buttons), or analog
(reported as axes).

- Action-Pad:
  - For 2-button pad: `BTN_SOUTH` (upper or left) and `BTN_EAST`
    (lower or right).
  - For 3-button pad: `BTN_WEST`, `BTN_SOUTH`, `BTN_EAST` (left to
    right or top to bottom).
  - For 4-button pad: `BTN_NORTH`, `BTN_WEST`, `BTN_SOUTH`, `BTN_EAST`
    (if rectangular, north is top left).
- D-Pad:
  - If digital: `BTN_DPAD_UP`, `BTN_DPAD_DOWN`, `BTN_DPAD_LEFT`,
    `BTN_DPAD_RIGHT`.
  - If analog: axes `ABS_HAT0X`, `ABS_HAT0Y`.
- Analog sticks:
  - Left stick: axes `ABS_X`, `ABS_Y`, button `BTN_THUMBL`.
  - Right stick: axes `ABS_RX`, `ABS_RY`, button `BTN_THUMBR`.
- Triggers:
  - Upper: `BTN_TR` or axis `ABS_HAT1X` (right), `BTN_TL` or axis
    `ABS_HAT1Y` (left).
  - Lower: `BTN_TR2` or axis `ABS_HAT2X` (right/ZR), `BTN_TL2` or axis
    `ABS_HAT2Y` (left/ZL).
  - If only one pair is present, it's reported as right.
  - If analog, pressure is reported as a positive integer.
- Menu-Pad:
  - `BTN_START` (right or only button) and `BTN_SELECT` (left, if
    present).
  - If there is a third (special: "HOME"/"X"/"PS") button, it's reported
    as `BTN_MODE`.
- Profile selection for adaptive controllers is reported as axis
  `ABS_PROFILE`.

## Joystick

There's no standard for joysticks.
