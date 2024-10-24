// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * UINPUT identifier tables.
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <linux/uinput.h>

#include "udotool.h"
#include "uinput-func.h"

/**
 * Find an item value by name.
 *
 * @param ids   list of items.
 * @param name  name to look for.
 * @return      item value.
 */
static int uinput_find_id(const struct udotool_obj_id ids[], const char *name) {
    for (const struct udotool_obj_id *idptr = ids; idptr->name != NULL; idptr++)
        if (strcasecmp(name, idptr->name) == 0)
            return idptr->value;
    return -1;
}

/**
 * Convert axis name to axis code.
 *
 * Depending on `mask`, this function looks for absolute axes, relative
 * axes, or both.
 *
 * If `pflag` is not `NULL`, the buffer it points to will be set to `1`
 * if the axis is absolute, or `0` otherwise.
 *
 * @param prefix  prefix for error messages.
 * @param name    axis name to look for.
 * @param mask    flag bit mask for types of axes to look for.
 * @param pflag   if not `NULL`, pointer to buffer to write axis type to.
 * @return        axis code, or `-1` if not found.
 */
int uinput_find_axis(const char *prefix, const char *name, unsigned mask, int *pflag) {
    int id;
    if ((mask & UDOTOOL_AXIS_ABS) != 0) {
        if ((id = uinput_find_id(UINPUT_ABS_AXES, name)) >= 0) {
            if (pflag != NULL)
                *pflag = 1;
            return id;
        }
    }
    if ((mask & UDOTOOL_AXIS_REL) != 0) {
        if ((id = uinput_find_id(UINPUT_REL_AXES, name)) >= 0) {
            if (pflag != NULL)
                *pflag = 0;
            return id;
        }
    }
    log_message(-1, "%s: unrecognized axis '%s'", prefix, name);
    return -1;
}

/**
 * Convert key/button to its value.
 *
 * Key/button can be either a name from predefined list, or a numeric
 * (decimal, octal, or hexadecimal) value.
 *
 * @param prefix  prefix for error messages.
 * @param key     key/button.
 * @return        key/button value.
 */
int uinput_find_key(const char *prefix, const char *key) {
    if (key[0] >= '0' && key[0] <= '9') {
        const char *ep = key;
        unsigned long uval = strtoul(key, (char **)&ep, 0);
        if (ep == key || *ep != '\0' || uval > KEY_MAX)
            goto ON_UNKN_KEY;
        return (int)uval;
    }
    int id;
    if ((id = uinput_find_id(UINPUT_KEYS, key)) < 0) {
ON_UNKN_KEY:
        log_message(-1, "%s: unrecognized key '%s'", prefix, key);
        return -1;
    }
    return id;
}

/**
 * List of primary relative axes.
 *
 * There are two sets (main and alternative), three axes in each.
 */
const int UINPUT_MAIN_REL_AXES[2][3] = {
    { REL_X,  REL_Y,  REL_Z  },
    { REL_RX, REL_RY, REL_RZ }
};
/**
 * List of primary absolute axes.
 *
 * There are two sets (main and alternative), three axes in each.
 */
const int UINPUT_MAIN_ABS_AXES[2][3] = {
    { ABS_X,  ABS_Y,  ABS_Z  },
    { ABS_RX, ABS_RY, ABS_RZ }
};
/**
 * List of wheel axes.
 *
 * There are two axes (main and horizontal).
 */
const int UINPUT_MAIN_WHEEL_AXES[2]  = {
    REL_WHEEL,
    REL_HWHEEL
};
/**
 * Map of high-resolution wheel axes.
 *
 * Each element contains: low-resolution axis code, high-resolution
 * axis code, conversion factor.
 */
const struct udotool_hires_axis UINPUT_HIRES_AXIS[] = {
    { REL_WHEEL,  REL_WHEEL_HI_RES,  120 },
    { REL_HWHEEL, REL_HWHEEL_HI_RES, 120 },
    { -1, -1, 0 }
};

#define DEF_KEY(V) { #V, V }

/**
 * List of known relative axes.
 */
const struct udotool_obj_id UINPUT_REL_AXES[] = {
    // Regular axes: mouse, touchpad, gamepad (left stick)
    DEF_KEY(REL_X),
    DEF_KEY(REL_Y),
    DEF_KEY(REL_Z),
    // "Rotate" axes, gamepad (right stick)
    DEF_KEY(REL_RX),
    DEF_KEY(REL_RY),
    DEF_KEY(REL_RZ),
    // Various special axes
    DEF_KEY(REL_DIAL),
    DEF_KEY(REL_MISC),
    // Wheel axes
    DEF_KEY(REL_WHEEL),  // Needs special handling!
    DEF_KEY(REL_HWHEEL), // Needs special handling!
    { NULL }
};

/**
 * List of known absolute axes.
 */
const struct udotool_obj_id UINPUT_ABS_AXES[] = {
    // Regular axes
    DEF_KEY(ABS_X),
    DEF_KEY(ABS_Y),
    DEF_KEY(ABS_Z),
    // "Rotate" axes
    DEF_KEY(ABS_RX),
    DEF_KEY(ABS_RY),
    DEF_KEY(ABS_RZ),
    // Various special axes
    DEF_KEY(ABS_THROTTLE),
    DEF_KEY(ABS_RUDDER),
    DEF_KEY(ABS_WHEEL),
    DEF_KEY(ABS_GAS),
    DEF_KEY(ABS_BRAKE),
    // Analog gamepad controls
    DEF_KEY(ABS_HAT0X),
    DEF_KEY(ABS_HAT0Y),
    DEF_KEY(ABS_HAT1X),
    DEF_KEY(ABS_HAT1Y),
    DEF_KEY(ABS_HAT2X),
    DEF_KEY(ABS_HAT2Y),
    DEF_KEY(ABS_HAT3X),
    DEF_KEY(ABS_HAT3Y),
    // Digitizer axes
    DEF_KEY(ABS_PRESSURE),
    DEF_KEY(ABS_DISTANCE),
    DEF_KEY(ABS_TILT_X),
    DEF_KEY(ABS_TILT_Y),
    DEF_KEY(ABS_TOOL_WIDTH),
    DEF_KEY(ABS_VOLUME),
    // Special axes
    DEF_KEY(ABS_PROFILE),
    DEF_KEY(ABS_MISC),
#if 0 // TODO: Multitouch
    // Multitouch axes
    DEF_KEY(ABS_MT_SLOT),
    DEF_KEY(ABS_MT_TOUCH_MAJOR),
    DEF_KEY(ABS_MT_TOUCH_MINOR),
    DEF_KEY(ABS_MT_WIDTH_MAJOR),
    DEF_KEY(ABS_MT_WIDTH_MINOR),
    DEF_KEY(ABS_MT_ORIENTATION),
    DEF_KEY(ABS_MT_POSITION_X),
#endif
    { NULL }
};

/**
 * List of known key/button names.
 */
const struct udotool_obj_id UINPUT_KEYS[] = {
    // Main kbd, row 1 (esc - backspace)
    DEF_KEY(KEY_ESC),
    DEF_KEY(KEY_1),
    DEF_KEY(KEY_2),
    DEF_KEY(KEY_3),
    DEF_KEY(KEY_4),
    DEF_KEY(KEY_5),
    DEF_KEY(KEY_6),
    DEF_KEY(KEY_7),
    DEF_KEY(KEY_8),
    DEF_KEY(KEY_9),
    DEF_KEY(KEY_0),
    DEF_KEY(KEY_MINUS),
    DEF_KEY(KEY_EQUAL),
    DEF_KEY(KEY_BACKSPACE),
    // Main kbd, row 2 (tab - enter)
    DEF_KEY(KEY_TAB),
    DEF_KEY(KEY_Q),
    DEF_KEY(KEY_W),
    DEF_KEY(KEY_E),
    DEF_KEY(KEY_R),
    DEF_KEY(KEY_T),
    DEF_KEY(KEY_Y),
    DEF_KEY(KEY_U),
    DEF_KEY(KEY_I),
    DEF_KEY(KEY_O),
    DEF_KEY(KEY_P),
    DEF_KEY(KEY_LEFTBRACE),
    DEF_KEY(KEY_RIGHTBRACE),
    DEF_KEY(KEY_ENTER),
    // Main kbd, row 3 (left Ctrl - grave)
    DEF_KEY(KEY_LEFTCTRL),
    DEF_KEY(KEY_A),
    DEF_KEY(KEY_S),
    DEF_KEY(KEY_D),
    DEF_KEY(KEY_F),
    DEF_KEY(KEY_G),
    DEF_KEY(KEY_H),
    DEF_KEY(KEY_J),
    DEF_KEY(KEY_K),
    DEF_KEY(KEY_L),
    DEF_KEY(KEY_SEMICOLON),
    DEF_KEY(KEY_APOSTROPHE),
    DEF_KEY(KEY_GRAVE),
    // Main kbd, row 4 (left Shift - right Shift)
    DEF_KEY(KEY_LEFTSHIFT),
    DEF_KEY(KEY_BACKSLASH),
    DEF_KEY(KEY_Z),
    DEF_KEY(KEY_X),
    DEF_KEY(KEY_C),
    DEF_KEY(KEY_V),
    DEF_KEY(KEY_B),
    DEF_KEY(KEY_N),
    DEF_KEY(KEY_M),
    DEF_KEY(KEY_COMMA),
    DEF_KEY(KEY_DOT),
    DEF_KEY(KEY_SLASH),
    DEF_KEY(KEY_RIGHTSHIFT),
    // Main kbd, numpad and other keys
    DEF_KEY(KEY_KPASTERISK),
    DEF_KEY(KEY_LEFTALT),
    DEF_KEY(KEY_SPACE),
    DEF_KEY(KEY_CAPSLOCK),
    // Main kbd, F1-F10
    DEF_KEY(KEY_F1),
    DEF_KEY(KEY_F2),
    DEF_KEY(KEY_F3),
    DEF_KEY(KEY_F4),
    DEF_KEY(KEY_F5),
    DEF_KEY(KEY_F6),
    DEF_KEY(KEY_F7),
    DEF_KEY(KEY_F8),
    DEF_KEY(KEY_F9),
    DEF_KEY(KEY_F10),
    // Main kbd, locks and numpad
    DEF_KEY(KEY_NUMLOCK),
    DEF_KEY(KEY_SCROLLLOCK),
    DEF_KEY(KEY_KP7),
    DEF_KEY(KEY_KP8),
    DEF_KEY(KEY_KP9),
    DEF_KEY(KEY_KPMINUS),
    DEF_KEY(KEY_KP4),
    DEF_KEY(KEY_KP5),
    DEF_KEY(KEY_KP6),
    DEF_KEY(KEY_KPPLUS),
    DEF_KEY(KEY_KP1),
    DEF_KEY(KEY_KP2),
    DEF_KEY(KEY_KP3),
    DEF_KEY(KEY_KP0),
    DEF_KEY(KEY_KPDOT),
    // Main kbd, CJK and special keys
    DEF_KEY(KEY_ZENKAKUHANKAKU),
    DEF_KEY(KEY_102ND),
    DEF_KEY(KEY_F11),
    DEF_KEY(KEY_F12),
    DEF_KEY(KEY_RO),
    DEF_KEY(KEY_KATAKANA),
    DEF_KEY(KEY_HIRAGANA),
    DEF_KEY(KEY_HENKAN),
    DEF_KEY(KEY_KATAKANAHIRAGANA),
    DEF_KEY(KEY_MUHENKAN),
    DEF_KEY(KEY_KPJPCOMMA),
    // Main kbd, other keys
    DEF_KEY(KEY_KPENTER),
    DEF_KEY(KEY_RIGHTCTRL),
    DEF_KEY(KEY_KPSLASH),
    DEF_KEY(KEY_SYSRQ),
    DEF_KEY(KEY_RIGHTALT),
    DEF_KEY(KEY_LINEFEED),
    // Main kbd, arrows and page control block
    DEF_KEY(KEY_HOME),
    DEF_KEY(KEY_UP),
    DEF_KEY(KEY_PAGEUP),
    DEF_KEY(KEY_LEFT),
    DEF_KEY(KEY_RIGHT),
    DEF_KEY(KEY_END),
    DEF_KEY(KEY_DOWN),
    DEF_KEY(KEY_PAGEDOWN),
    DEF_KEY(KEY_INSERT),
    DEF_KEY(KEY_DELETE),
    // Main kbd, multimedia and special keys
    DEF_KEY(KEY_MACRO),
    DEF_KEY(KEY_MUTE),
    DEF_KEY(KEY_VOLUMEDOWN),
    DEF_KEY(KEY_VOLUMEUP),
    DEF_KEY(KEY_POWER),
    DEF_KEY(KEY_KPEQUAL),
    DEF_KEY(KEY_KPPLUSMINUS),
    DEF_KEY(KEY_PAUSE),
    DEF_KEY(KEY_SCALE),
    DEF_KEY(KEY_KPCOMMA),
    // Main kbd, CJK keys
    DEF_KEY(KEY_HANGEUL),
    DEF_KEY(KEY_HANJA),
    DEF_KEY(KEY_YEN),
    // Main kbd, controls
    DEF_KEY(KEY_LEFTMETA),
    DEF_KEY(KEY_RIGHTMETA),
    DEF_KEY(KEY_COMPOSE),
    // Main kbd, tools & multimedia keys
    DEF_KEY(KEY_STOP),
    DEF_KEY(KEY_AGAIN),
    DEF_KEY(KEY_PROPS),
    DEF_KEY(KEY_UNDO),
    DEF_KEY(KEY_FRONT),
    DEF_KEY(KEY_COPY),
    DEF_KEY(KEY_OPEN),
    DEF_KEY(KEY_PASTE),
    DEF_KEY(KEY_FIND),
    DEF_KEY(KEY_CUT),
    DEF_KEY(KEY_HELP),
    DEF_KEY(KEY_MENU),
    DEF_KEY(KEY_CALC),
    DEF_KEY(KEY_SETUP),
    DEF_KEY(KEY_SLEEP),
    DEF_KEY(KEY_WAKEUP),
    DEF_KEY(KEY_FILE),
    DEF_KEY(KEY_SENDFILE),
    DEF_KEY(KEY_DELETEFILE),
    DEF_KEY(KEY_XFER),
    DEF_KEY(KEY_PROG1),
    DEF_KEY(KEY_PROG2),
    DEF_KEY(KEY_WWW),
    DEF_KEY(KEY_MSDOS),
    DEF_KEY(KEY_SCREENLOCK),
    DEF_KEY(KEY_ROTATE_DISPLAY),
    DEF_KEY(KEY_CYCLEWINDOWS),
    DEF_KEY(KEY_MAIL),
    DEF_KEY(KEY_BOOKMARKS),
    DEF_KEY(KEY_COMPUTER),
    DEF_KEY(KEY_BACK),
    DEF_KEY(KEY_FORWARD),
    DEF_KEY(KEY_CLOSECD),
    DEF_KEY(KEY_EJECTCD),
    DEF_KEY(KEY_EJECTCLOSECD),
    DEF_KEY(KEY_NEXTSONG),
    DEF_KEY(KEY_PLAYPAUSE),
    DEF_KEY(KEY_PREVIOUSSONG),
    DEF_KEY(KEY_STOPCD),
    DEF_KEY(KEY_RECORD),
    DEF_KEY(KEY_REWIND),
    DEF_KEY(KEY_PHONE),
    DEF_KEY(KEY_ISO),
    DEF_KEY(KEY_CONFIG),
    DEF_KEY(KEY_HOMEPAGE),
    DEF_KEY(KEY_REFRESH),
    DEF_KEY(KEY_EXIT),
    DEF_KEY(KEY_MOVE),
    DEF_KEY(KEY_EDIT),
    DEF_KEY(KEY_SCROLLUP),
    DEF_KEY(KEY_SCROLLDOWN),
    DEF_KEY(KEY_KPLEFTPAREN),
    DEF_KEY(KEY_KPRIGHTPAREN),
    DEF_KEY(KEY_NEW),
    DEF_KEY(KEY_REDO),
    // Main kbd, F13-F24
    DEF_KEY(KEY_F13),
    DEF_KEY(KEY_F14),
    DEF_KEY(KEY_F15),
    DEF_KEY(KEY_F16),
    DEF_KEY(KEY_F17),
    DEF_KEY(KEY_F18),
    DEF_KEY(KEY_F19),
    DEF_KEY(KEY_F20),
    DEF_KEY(KEY_F21),
    DEF_KEY(KEY_F22),
    DEF_KEY(KEY_F23),
    DEF_KEY(KEY_F24),
    // Main kbd, more tools & multimedia keys
    DEF_KEY(KEY_PLAYCD),
    DEF_KEY(KEY_PAUSECD),
    DEF_KEY(KEY_PROG3),
    DEF_KEY(KEY_PROG4),
    DEF_KEY(KEY_DASHBOARD),
    DEF_KEY(KEY_SUSPEND),
    DEF_KEY(KEY_CLOSE),
    DEF_KEY(KEY_PLAY),
    DEF_KEY(KEY_FASTFORWARD),
    DEF_KEY(KEY_BASSBOOST),
    DEF_KEY(KEY_PRINT),
    DEF_KEY(KEY_HP),
    DEF_KEY(KEY_CAMERA),
    DEF_KEY(KEY_SOUND),
    DEF_KEY(KEY_QUESTION),
    DEF_KEY(KEY_EMAIL),
    DEF_KEY(KEY_CHAT),
    DEF_KEY(KEY_SEARCH),
    DEF_KEY(KEY_CONNECT),
    DEF_KEY(KEY_FINANCE),
    DEF_KEY(KEY_SPORT),
    DEF_KEY(KEY_SHOP),
    DEF_KEY(KEY_ALTERASE),
    DEF_KEY(KEY_CANCEL),
    DEF_KEY(KEY_BRIGHTNESSDOWN),
    DEF_KEY(KEY_BRIGHTNESSUP),
    DEF_KEY(KEY_MEDIA),
    DEF_KEY(KEY_SWITCHVIDEOMODE),
    DEF_KEY(KEY_KBDILLUMTOGGLE),
    DEF_KEY(KEY_KBDILLUMDOWN),
    DEF_KEY(KEY_KBDILLUMUP),
    DEF_KEY(KEY_SEND),
    DEF_KEY(KEY_REPLY),
    DEF_KEY(KEY_FORWARDMAIL),
    DEF_KEY(KEY_SAVE),
    DEF_KEY(KEY_DOCUMENTS),
    DEF_KEY(KEY_BATTERY),
    DEF_KEY(KEY_BLUETOOTH),
    DEF_KEY(KEY_WLAN),
    DEF_KEY(KEY_UWB),
    DEF_KEY(KEY_UNKNOWN),
    DEF_KEY(KEY_VIDEO_NEXT),
    DEF_KEY(KEY_VIDEO_PREV),
    DEF_KEY(KEY_BRIGHTNESS_CYCLE),
    DEF_KEY(KEY_BRIGHTNESS_AUTO),
    DEF_KEY(KEY_DISPLAY_OFF),
    DEF_KEY(KEY_WWAN),
    DEF_KEY(KEY_RFKILL),
    DEF_KEY(KEY_MICMUTE),
    // Generic buttons
    DEF_KEY(BTN_MISC),
    DEF_KEY(BTN_0),
    DEF_KEY(BTN_1),
    DEF_KEY(BTN_2),
    DEF_KEY(BTN_3),
    DEF_KEY(BTN_4),
    DEF_KEY(BTN_5),
    DEF_KEY(BTN_6),
    DEF_KEY(BTN_7),
    DEF_KEY(BTN_8),
    DEF_KEY(BTN_9),
    // Mouse buttons
    DEF_KEY(BTN_LEFT),
    DEF_KEY(BTN_RIGHT),
    DEF_KEY(BTN_MIDDLE),
    DEF_KEY(BTN_SIDE),
    DEF_KEY(BTN_EXTRA),
    DEF_KEY(BTN_FORWARD),
    DEF_KEY(BTN_BACK),
    DEF_KEY(BTN_TASK),
    // Joystick buttons
    DEF_KEY(BTN_TRIGGER),
    DEF_KEY(BTN_THUMB),
    DEF_KEY(BTN_THUMB2),
    DEF_KEY(BTN_TOP),
    DEF_KEY(BTN_TOP2),
    DEF_KEY(BTN_PINKIE),
    DEF_KEY(BTN_BASE),
    DEF_KEY(BTN_BASE2),
    DEF_KEY(BTN_BASE3),
    DEF_KEY(BTN_BASE4),
    DEF_KEY(BTN_BASE5),
    DEF_KEY(BTN_BASE6),
    DEF_KEY(BTN_DEAD),
    // Gamepad buttons
    DEF_KEY(BTN_SOUTH),
    DEF_KEY(BTN_A),
    DEF_KEY(BTN_EAST),
    DEF_KEY(BTN_B),
    DEF_KEY(BTN_C),
    DEF_KEY(BTN_NORTH),
    DEF_KEY(BTN_X),
    DEF_KEY(BTN_WEST),
    DEF_KEY(BTN_Y),
    DEF_KEY(BTN_Z),
    DEF_KEY(BTN_TL),
    DEF_KEY(BTN_TR),
    DEF_KEY(BTN_TL2),
    DEF_KEY(BTN_TR2),
    DEF_KEY(BTN_SELECT),
    DEF_KEY(BTN_START),
    DEF_KEY(BTN_MODE),
    DEF_KEY(BTN_THUMBL),
    DEF_KEY(BTN_THUMBR),
#ifndef UDOTOOL_LIBINPUT_QUIRK
    // Digitizer buttons
    DEF_KEY(BTN_TOOL_PEN),
    DEF_KEY(BTN_TOOL_RUBBER),
    DEF_KEY(BTN_TOOL_BRUSH),
    DEF_KEY(BTN_TOOL_PENCIL),
    DEF_KEY(BTN_TOOL_AIRBRUSH),
    DEF_KEY(BTN_TOOL_FINGER),
    DEF_KEY(BTN_TOOL_MOUSE),
    DEF_KEY(BTN_TOOL_LENS),
    DEF_KEY(BTN_TOOL_QUINTTAP),
    DEF_KEY(BTN_STYLUS3),
    DEF_KEY(BTN_TOUCH),
    DEF_KEY(BTN_STYLUS),
    DEF_KEY(BTN_STYLUS2),
    DEF_KEY(BTN_TOOL_DOUBLETAP),
    DEF_KEY(BTN_TOOL_TRIPLETAP),
    DEF_KEY(BTN_TOOL_QUADTAP),
#endif // !UDOTOOL_LIBINPUT_QUIRK
    // Wheel & gear buttons
    DEF_KEY(BTN_WHEEL),
    DEF_KEY(BTN_GEAR_DOWN),
    DEF_KEY(BTN_GEAR_UP),
    // Media keys
    DEF_KEY(KEY_OK),
    DEF_KEY(KEY_SELECT),
    DEF_KEY(KEY_GOTO),
    DEF_KEY(KEY_CLEAR),
    DEF_KEY(KEY_POWER2),
    DEF_KEY(KEY_OPTION),
    DEF_KEY(KEY_INFO),
    DEF_KEY(KEY_TIME),
    DEF_KEY(KEY_VENDOR),
    DEF_KEY(KEY_ARCHIVE),
    DEF_KEY(KEY_PROGRAM),
    DEF_KEY(KEY_CHANNEL),
    DEF_KEY(KEY_FAVORITES),
    DEF_KEY(KEY_EPG),
    DEF_KEY(KEY_PVR),
    DEF_KEY(KEY_MHP),
    DEF_KEY(KEY_LANGUAGE),
    DEF_KEY(KEY_TITLE),
    DEF_KEY(KEY_SUBTITLE),
    DEF_KEY(KEY_ANGLE),
    DEF_KEY(KEY_FULL_SCREEN),
    DEF_KEY(KEY_ZOOM),
    DEF_KEY(KEY_MODE),
    DEF_KEY(KEY_KEYBOARD),
    DEF_KEY(KEY_ASPECT_RATIO),
    DEF_KEY(KEY_SCREEN),
    DEF_KEY(KEY_PC),
    DEF_KEY(KEY_TV),
    DEF_KEY(KEY_TV2),
    DEF_KEY(KEY_VCR),
    DEF_KEY(KEY_VCR2),
    DEF_KEY(KEY_SAT),
    DEF_KEY(KEY_SAT2),
    DEF_KEY(KEY_CD),
    DEF_KEY(KEY_TAPE),
    DEF_KEY(KEY_RADIO),
    DEF_KEY(KEY_TUNER),
    DEF_KEY(KEY_PLAYER),
    DEF_KEY(KEY_TEXT),
    DEF_KEY(KEY_DVD),
    DEF_KEY(KEY_AUX),
    DEF_KEY(KEY_MP3),
    DEF_KEY(KEY_AUDIO),
    DEF_KEY(KEY_VIDEO),
    DEF_KEY(KEY_DIRECTORY),
    DEF_KEY(KEY_LIST),
    DEF_KEY(KEY_MEMO),
    DEF_KEY(KEY_CALENDAR),
    DEF_KEY(KEY_RED),
    DEF_KEY(KEY_GREEN),
    DEF_KEY(KEY_YELLOW),
    DEF_KEY(KEY_BLUE),
    DEF_KEY(KEY_CHANNELUP),
    DEF_KEY(KEY_CHANNELDOWN),
    DEF_KEY(KEY_FIRST),
    DEF_KEY(KEY_LAST),
    DEF_KEY(KEY_AB),
    DEF_KEY(KEY_NEXT),
    DEF_KEY(KEY_RESTART),
    DEF_KEY(KEY_SLOW),
    DEF_KEY(KEY_SHUFFLE),
    DEF_KEY(KEY_BREAK),
    DEF_KEY(KEY_PREVIOUS),
    DEF_KEY(KEY_DIGITS),
    DEF_KEY(KEY_TEEN),
    DEF_KEY(KEY_TWEN),
    DEF_KEY(KEY_VIDEOPHONE),
    DEF_KEY(KEY_GAMES),
    DEF_KEY(KEY_ZOOMIN),
    DEF_KEY(KEY_ZOOMOUT),
    DEF_KEY(KEY_ZOOMRESET),
    DEF_KEY(KEY_WORDPROCESSOR),
    DEF_KEY(KEY_EDITOR),
    DEF_KEY(KEY_SPREADSHEET),
    DEF_KEY(KEY_GRAPHICSEDITOR),
    DEF_KEY(KEY_PRESENTATION),
    DEF_KEY(KEY_DATABASE),
    DEF_KEY(KEY_NEWS),
    DEF_KEY(KEY_VOICEMAIL),
    DEF_KEY(KEY_ADDRESSBOOK),
    DEF_KEY(KEY_MESSENGER),
    DEF_KEY(KEY_DISPLAYTOGGLE),
    DEF_KEY(KEY_BRIGHTNESS_TOGGLE),
    DEF_KEY(KEY_SPELLCHECK),
    DEF_KEY(KEY_LOGOFF),
    // Money symbol keys
    DEF_KEY(KEY_DOLLAR),
    DEF_KEY(KEY_EURO),
    // Media keys
    DEF_KEY(KEY_FRAMEBACK),
    DEF_KEY(KEY_FRAMEFORWARD),
    DEF_KEY(KEY_CONTEXT_MENU),
    DEF_KEY(KEY_MEDIA_REPEAT),
    DEF_KEY(KEY_10CHANNELSUP),
    DEF_KEY(KEY_10CHANNELSDOWN),
    DEF_KEY(KEY_IMAGES),
    DEF_KEY(KEY_NOTIFICATION_CENTER),
    DEF_KEY(KEY_PICKUP_PHONE),
    DEF_KEY(KEY_HANGUP_PHONE),
    // Line control keys
    DEF_KEY(KEY_DEL_EOL),
    DEF_KEY(KEY_DEL_EOS),
    DEF_KEY(KEY_INS_LINE),
    DEF_KEY(KEY_DEL_LINE),
    // FN keys
    DEF_KEY(KEY_FN),
    DEF_KEY(KEY_FN_ESC),
    DEF_KEY(KEY_FN_F1),
    DEF_KEY(KEY_FN_F2),
    DEF_KEY(KEY_FN_F3),
    DEF_KEY(KEY_FN_F4),
    DEF_KEY(KEY_FN_F5),
    DEF_KEY(KEY_FN_F6),
    DEF_KEY(KEY_FN_F7),
    DEF_KEY(KEY_FN_F8),
    DEF_KEY(KEY_FN_F9),
    DEF_KEY(KEY_FN_F10),
    DEF_KEY(KEY_FN_F11),
    DEF_KEY(KEY_FN_F12),
    DEF_KEY(KEY_FN_1),
    DEF_KEY(KEY_FN_2),
    DEF_KEY(KEY_FN_D),
    DEF_KEY(KEY_FN_E),
    DEF_KEY(KEY_FN_F),
    DEF_KEY(KEY_FN_S),
    DEF_KEY(KEY_FN_B),
    DEF_KEY(KEY_FN_RIGHT_SHIFT),
    // Braille keys
    DEF_KEY(KEY_BRL_DOT1),
    DEF_KEY(KEY_BRL_DOT2),
    DEF_KEY(KEY_BRL_DOT3),
    DEF_KEY(KEY_BRL_DOT4),
    DEF_KEY(KEY_BRL_DOT5),
    DEF_KEY(KEY_BRL_DOT6),
    DEF_KEY(KEY_BRL_DOT7),
    DEF_KEY(KEY_BRL_DOT8),
    DEF_KEY(KEY_BRL_DOT9),
    DEF_KEY(KEY_BRL_DOT10),
    // Numeric keys
    DEF_KEY(KEY_NUMERIC_0),
    DEF_KEY(KEY_NUMERIC_1),
    DEF_KEY(KEY_NUMERIC_2),
    DEF_KEY(KEY_NUMERIC_3),
    DEF_KEY(KEY_NUMERIC_4),
    DEF_KEY(KEY_NUMERIC_5),
    DEF_KEY(KEY_NUMERIC_6),
    DEF_KEY(KEY_NUMERIC_7),
    DEF_KEY(KEY_NUMERIC_8),
    DEF_KEY(KEY_NUMERIC_9),
    DEF_KEY(KEY_NUMERIC_STAR),
    DEF_KEY(KEY_NUMERIC_POUND),
    DEF_KEY(KEY_NUMERIC_A),
    DEF_KEY(KEY_NUMERIC_B),
    DEF_KEY(KEY_NUMERIC_C),
    DEF_KEY(KEY_NUMERIC_D),
    // Misc keys
    DEF_KEY(KEY_CAMERA_FOCUS),
    DEF_KEY(KEY_WPS_BUTTON),
    // Touchpad keys
    DEF_KEY(KEY_TOUCHPAD_TOGGLE),
    DEF_KEY(KEY_TOUCHPAD_ON),
    DEF_KEY(KEY_TOUCHPAD_OFF),
    // Camera keys
    DEF_KEY(KEY_CAMERA_ZOOMIN),
    DEF_KEY(KEY_CAMERA_ZOOMOUT),
    DEF_KEY(KEY_CAMERA_UP),
    DEF_KEY(KEY_CAMERA_DOWN),
    DEF_KEY(KEY_CAMERA_LEFT),
    DEF_KEY(KEY_CAMERA_RIGHT),
    // Other keys
    DEF_KEY(KEY_ATTENDANT_ON),
    DEF_KEY(KEY_ATTENDANT_OFF),
    DEF_KEY(KEY_ATTENDANT_TOGGLE),
    DEF_KEY(KEY_LIGHTS_TOGGLE),
    // D-Pad buttons
    DEF_KEY(BTN_DPAD_UP),
    DEF_KEY(BTN_DPAD_DOWN),
    DEF_KEY(BTN_DPAD_LEFT),
    DEF_KEY(BTN_DPAD_RIGHT),
    // Display keys
    DEF_KEY(KEY_ALS_TOGGLE),
    DEF_KEY(KEY_ROTATE_LOCK_TOGGLE),
    DEF_KEY(KEY_REFRESH_RATE_TOGGLE),
    // App keys
    DEF_KEY(KEY_BUTTONCONFIG),
    DEF_KEY(KEY_TASKMANAGER),
    DEF_KEY(KEY_JOURNAL),
    DEF_KEY(KEY_CONTROLPANEL),
    DEF_KEY(KEY_APPSELECT),
    DEF_KEY(KEY_SCREENSAVER),
    DEF_KEY(KEY_VOICECOMMAND),
    DEF_KEY(KEY_ASSISTANT),
    DEF_KEY(KEY_KBD_LAYOUT_NEXT),
    DEF_KEY(KEY_EMOJI_PICKER),
    DEF_KEY(KEY_DICTATE),
    // Brightness keys
    DEF_KEY(KEY_BRIGHTNESS_MIN),
    DEF_KEY(KEY_BRIGHTNESS_MAX),
    // Input assist keys
    DEF_KEY(KEY_KBDINPUTASSIST_PREV),
    DEF_KEY(KEY_KBDINPUTASSIST_NEXT),
    DEF_KEY(KEY_KBDINPUTASSIST_PREVGROUP),
    DEF_KEY(KEY_KBDINPUTASSIST_NEXTGROUP),
    DEF_KEY(KEY_KBDINPUTASSIST_ACCEPT),
    DEF_KEY(KEY_KBDINPUTASSIST_CANCEL),
    // Diagonal movement keys
    DEF_KEY(KEY_RIGHT_UP),
    DEF_KEY(KEY_RIGHT_DOWN),
    DEF_KEY(KEY_LEFT_UP),
    DEF_KEY(KEY_LEFT_DOWN),
    // Media keys
    DEF_KEY(KEY_ROOT_MENU),
    DEF_KEY(KEY_MEDIA_TOP_MENU),
    DEF_KEY(KEY_NUMERIC_11),
    DEF_KEY(KEY_NUMERIC_12),
    DEF_KEY(KEY_AUDIO_DESC),
    DEF_KEY(KEY_3D_MODE),
    DEF_KEY(KEY_NEXT_FAVORITE),
    DEF_KEY(KEY_STOP_RECORD),
    DEF_KEY(KEY_PAUSE_RECORD),
    DEF_KEY(KEY_VOD),
    DEF_KEY(KEY_UNMUTE),
    DEF_KEY(KEY_FASTREVERSE),
    DEF_KEY(KEY_SLOWREVERSE),
    DEF_KEY(KEY_DATA),
    DEF_KEY(KEY_ONSCREEN_KEYBOARD),
    DEF_KEY(KEY_PRIVACY_SCREEN_TOGGLE),
    DEF_KEY(KEY_SELECTIVE_SCREENSHOT),
    // Nav keys
    DEF_KEY(KEY_NEXT_ELEMENT),
    DEF_KEY(KEY_PREVIOUS_ELEMENT),
    DEF_KEY(KEY_AUTOPILOT_ENGAGE_TOGGLE),
    DEF_KEY(KEY_MARK_WAYPOINT),
    DEF_KEY(KEY_SOS),
    DEF_KEY(KEY_NAV_CHART),
    DEF_KEY(KEY_FISHING_CHART),
    DEF_KEY(KEY_SINGLE_RANGE_RADAR),
    DEF_KEY(KEY_DUAL_RANGE_RADAR),
    DEF_KEY(KEY_RADAR_OVERLAY),
    DEF_KEY(KEY_TRADITIONAL_SONAR),
    DEF_KEY(KEY_CLEARVU_SONAR),
    DEF_KEY(KEY_SIDEVU_SONAR),
    DEF_KEY(KEY_NAV_INFO),
    DEF_KEY(KEY_BRIGHTNESS_MENU),
    // Macro keys
    DEF_KEY(KEY_MACRO1),
    DEF_KEY(KEY_MACRO2),
    DEF_KEY(KEY_MACRO3),
    DEF_KEY(KEY_MACRO4),
    DEF_KEY(KEY_MACRO5),
    DEF_KEY(KEY_MACRO6),
    DEF_KEY(KEY_MACRO7),
    DEF_KEY(KEY_MACRO8),
    DEF_KEY(KEY_MACRO9),
    DEF_KEY(KEY_MACRO10),
    DEF_KEY(KEY_MACRO11),
    DEF_KEY(KEY_MACRO12),
    DEF_KEY(KEY_MACRO13),
    DEF_KEY(KEY_MACRO14),
    DEF_KEY(KEY_MACRO15),
    DEF_KEY(KEY_MACRO16),
    DEF_KEY(KEY_MACRO17),
    DEF_KEY(KEY_MACRO18),
    DEF_KEY(KEY_MACRO19),
    DEF_KEY(KEY_MACRO20),
    DEF_KEY(KEY_MACRO21),
    DEF_KEY(KEY_MACRO22),
    DEF_KEY(KEY_MACRO23),
    DEF_KEY(KEY_MACRO24),
    DEF_KEY(KEY_MACRO25),
    DEF_KEY(KEY_MACRO26),
    DEF_KEY(KEY_MACRO27),
    DEF_KEY(KEY_MACRO28),
    DEF_KEY(KEY_MACRO29),
    DEF_KEY(KEY_MACRO30),
    DEF_KEY(KEY_MACRO_RECORD_START),
    DEF_KEY(KEY_MACRO_RECORD_STOP),
    DEF_KEY(KEY_MACRO_PRESET_CYCLE),
    DEF_KEY(KEY_MACRO_PRESET1),
    DEF_KEY(KEY_MACRO_PRESET2),
    DEF_KEY(KEY_MACRO_PRESET3),
    // LCD keys
    DEF_KEY(KEY_KBD_LCD_MENU1),
    DEF_KEY(KEY_KBD_LCD_MENU2),
    DEF_KEY(KEY_KBD_LCD_MENU3),
    DEF_KEY(KEY_KBD_LCD_MENU4),
    DEF_KEY(KEY_KBD_LCD_MENU5),
    // Trigger buttons
    DEF_KEY(BTN_TRIGGER_HAPPY),
    DEF_KEY(BTN_TRIGGER_HAPPY1),
    DEF_KEY(BTN_TRIGGER_HAPPY2),
    DEF_KEY(BTN_TRIGGER_HAPPY3),
    DEF_KEY(BTN_TRIGGER_HAPPY4),
    DEF_KEY(BTN_TRIGGER_HAPPY5),
    DEF_KEY(BTN_TRIGGER_HAPPY6),
    DEF_KEY(BTN_TRIGGER_HAPPY7),
    DEF_KEY(BTN_TRIGGER_HAPPY8),
    DEF_KEY(BTN_TRIGGER_HAPPY9),
    DEF_KEY(BTN_TRIGGER_HAPPY10),
    DEF_KEY(BTN_TRIGGER_HAPPY11),
    DEF_KEY(BTN_TRIGGER_HAPPY12),
    DEF_KEY(BTN_TRIGGER_HAPPY13),
    DEF_KEY(BTN_TRIGGER_HAPPY14),
    DEF_KEY(BTN_TRIGGER_HAPPY15),
    DEF_KEY(BTN_TRIGGER_HAPPY16),
    DEF_KEY(BTN_TRIGGER_HAPPY17),
    DEF_KEY(BTN_TRIGGER_HAPPY18),
    DEF_KEY(BTN_TRIGGER_HAPPY19),
    DEF_KEY(BTN_TRIGGER_HAPPY20),
    DEF_KEY(BTN_TRIGGER_HAPPY21),
    DEF_KEY(BTN_TRIGGER_HAPPY22),
    DEF_KEY(BTN_TRIGGER_HAPPY23),
    DEF_KEY(BTN_TRIGGER_HAPPY24),
    DEF_KEY(BTN_TRIGGER_HAPPY25),
    DEF_KEY(BTN_TRIGGER_HAPPY26),
    DEF_KEY(BTN_TRIGGER_HAPPY27),
    DEF_KEY(BTN_TRIGGER_HAPPY28),
    DEF_KEY(BTN_TRIGGER_HAPPY29),
    DEF_KEY(BTN_TRIGGER_HAPPY30),
    DEF_KEY(BTN_TRIGGER_HAPPY31),
    DEF_KEY(BTN_TRIGGER_HAPPY32),
    DEF_KEY(BTN_TRIGGER_HAPPY33),
    DEF_KEY(BTN_TRIGGER_HAPPY34),
    DEF_KEY(BTN_TRIGGER_HAPPY35),
    DEF_KEY(BTN_TRIGGER_HAPPY36),
    DEF_KEY(BTN_TRIGGER_HAPPY37),
    DEF_KEY(BTN_TRIGGER_HAPPY38),
    DEF_KEY(BTN_TRIGGER_HAPPY39),
    DEF_KEY(BTN_TRIGGER_HAPPY40),

    { NULL }
};
