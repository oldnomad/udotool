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
 * Find an item value by name, where name is not null-terminated.
 *
 * @param ids   list of items.
 * @param name  name to look for.
 * @param nlen  name length.
 * @return      item value.
 */
static int uinput_findn_id(const struct udotool_obj_id ids[], const char *name, size_t nlen) {
    for (const struct udotool_obj_id *idptr = ids; idptr->name != NULL; idptr++) {
        if (strncasecmp(name, idptr->name, nlen) == 0 && idptr->name[nlen] == '\0')
            return idptr->value;
    }
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
 * Convert flag name to its value.
 *
 * @param prefix  prefix for error messages.
 * @param name    flag name.
 * @param nlen    flag name length.
 * @return        flag value.
 */
int uinput_findn_flag(const char *prefix, const char *name, size_t nlen) {
    int id;
    if ((id = uinput_findn_id(UINPUT_FLAGS, name, nlen)) < 0) {
        log_message(-1, "%s: unrecognized flag '%.*s'", prefix, (int)nlen, name);
        return -1;
    }
    return id;
}

/**
 * List of known flags.
 */
const struct udotool_obj_id UINPUT_FLAGS[] = {
    { "libinput", UINPUT_FLAG_LIBINPUT },
    { NULL }
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
#include "uinput-table-keys.h"
    { NULL }
};
