// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * UINPUT I/O functions
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <linux/uinput.h>

#include "udotool.h"
#include "uinput-func.h"

/**
 * Default UINPUT emulation parameters.
 *
 * This group contains:
 * - UINPUT device path.
 * - Emulated device name.
 * - Settle time in seconds.
 * - Emulated device ID.
 * - Absolute axis definition (common for all absolute axes).
 */
static char UINPUT_DEVICE[PATH_MAX] = "/dev/uinput";
static char UINPUT_DEVNAME[UINPUT_MAX_NAME_SIZE] = "udotool";
static double UINPUT_SETTLE_TIME = DEFAULT_SETTLE_TIME;
static struct input_id UINPUT_ID = {
    .bustype = BUS_VIRTUAL,
    .vendor  = 0,
    .product = 0,
    .version = 0,
};
static struct input_absinfo UINPUT_AXIS_DEF = {
    .value = 0,
    .minimum = 0,                   // units
    .maximum = UINPUT_ABS_MAXVALUE, // units
    .fuzz = 0,
    .flat = 0,
    .resolution = 0, // unit/mm for main axes, unit/radian for ABS_R{X,Y,Z}
};

/**
 * Open callback and its data.
 */
static udotool_open_callback_t UINPUT_OPEN_CBK = NULL;
static void *UINPUT_OPEN_CBK_DATA = NULL;

/**
 * UINPUT device handle, or `-1` if not open yet.
 */
static int UINPUT_FD = -1;

/**
 * Set UINPUT option.
 *
 * @param option  option code.
 * @param value   option value.
 * @return        zero on success, or `-1` on error.
 */
int uinput_set_option(int option, const char *value) {
    size_t len;

    switch (option) {
    case UINPUT_OPT_DEVICE:
        len = strlen(value);
        if (len >= sizeof(UINPUT_DEVICE)) {
            log_message(-1, "UINPUT: device path is too long: %s", value);
            return -1;
        }
        strcpy(UINPUT_DEVICE, value);
        break;
    case UINPUT_OPT_DEVNAME:
        len = strlen(value);
        if (len >= sizeof(UINPUT_DEVNAME)) {
            log_message(-1, "UINPUT: device name is too long: %s", value);
            return -1;
        }
        strcpy(UINPUT_DEVNAME, value);
        break;
    case UINPUT_OPT_DEVID:
        {
            unsigned long uval;
            const char *sp = value, *ep = NULL;

            uval = strtoul(sp, (char **)&ep, 0);
            if (ep == sp || (*ep != ':' && *ep != '\0') || uval > USHRT_MAX) {
                log_message(-1, "UINPUT: error parsing device ID: %s", value);
                return -1;
            }
            UINPUT_ID.vendor = uval;
            if (*ep++ == '\0')
                break;
            sp = ep;
            uval = strtoul(sp, (char **)&ep, 0);
            if (ep == sp || (*ep != ':' && *ep != '\0') || uval > USHRT_MAX) {
                log_message(-1, "UINPUT: error parsing device ID: %s", value);
                return -1;
            }
            UINPUT_ID.product = uval;
            if (*ep++ == '\0')
                break;
            sp = ep;
            uval = strtoul(sp, (char **)&ep, 0);
            if (ep == sp || *ep != '\0' || uval > USHRT_MAX) {
                log_message(-1, "UINPUT: error parsing device ID: %s", value);
                return -1;
            }
            UINPUT_ID.version = uval;
        }
        break;
    case UINPUT_OPT_SETTLE:
        {
            double dval;
            const char *ep = NULL;

            dval = strtod(value, (char **)&ep);
            if (ep == value || *ep != '\0' ||
                dval < MIN_SLEEP_SEC || dval > MAX_SLEEP_SEC) {
                log_message(-1, "UINPUT: error parsing settle time: %s", value);
                return -1;
            }
            UINPUT_SETTLE_TIME = dval;
        }
        break;
    default:
        log_message(-1, "UINPUT: unrecognized option code %d", option);
        return -1;
    }
    return 0;
}

/**
 * Get UINPUT option.
 *
 * @param option  option code.
 * @param buffer  pointer to buffer for option value.
 * @param bufsize buffer size.
 * @return        zero on success, or `-1` on error.
 */
int uinput_get_option(int option, char *buffer, size_t bufsize) {
    size_t len;
    const char *pval;
    char intbuf[32];

    switch (option) {
    case UINPUT_OPT_DEVICE:
        pval = UINPUT_DEVICE;
        break;
    case UINPUT_OPT_DEVNAME:
        pval = UINPUT_DEVNAME;
        break;
    case UINPUT_OPT_DEVID:
        snprintf(intbuf, sizeof(intbuf), "0x%04X:0x%04X:0x%04X",
            UINPUT_ID.vendor, UINPUT_ID.product, UINPUT_ID.version);
        pval = intbuf;
        break;
    case UINPUT_OPT_SETTLE:
        if (UINPUT_SETTLE_TIME < MIN_SLEEP_SEC || UINPUT_SETTLE_TIME > MAX_SLEEP_SEC) {
            log_message(-1, "UINPUT: error using settle time: %.6f", UINPUT_SETTLE_TIME);
            return -1;
        }
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wformat-truncation"
        // Truncation cannot happen, since we limit settle time to 86400 seconds or less
        snprintf(intbuf, sizeof(intbuf), "%.6f", UINPUT_SETTLE_TIME);
#pragma GCC diagnostic pop
        pval = intbuf;
        break;
    default:
        log_message(-1, "UINPUT: unrecognized option code %d", option);
        return -1;
    }
    len = strlen(pval);
    if (len >= bufsize)
        len = bufsize;
    memcpy(buffer, pval, len);
    buffer[len] = '\0';
    return 0;
}

/**
 * Set UINPUT open callback.
 *
 * @param callback  callback function.
 * @param data      callback data.
 */
void uinput_set_open_callback(udotool_open_callback_t callback, void *data) {
    UINPUT_OPEN_CBK = callback;
    UINPUT_OPEN_CBK_DATA = data;
}

/**
 * Issue an IOCTL with an integer parameter.
 *
 * @param fd    device handle.
 * @param name  IOCTL name (for messages).
 * @param code  IOCTL code.
 * @param arg   IOCTL parameter.
 * @return      zero on success, or `-1` on error.
 */
static int uinput_ioctl_int(int fd, const char *name, unsigned long code, int arg) {
    log_message(2, "UINPUT: ioctl(%s, 0x%04X)", name, (unsigned)arg);
    if (ioctl(fd, code, arg) == -1) {
        log_message(-1, "UINPUT: ioctl %s error: %s", name, strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Issue an IOCTL with a pointer parameter.
 *
 * @param fd    device handle.
 * @param name  IOCTL name (for messages).
 * @param code  IOCTL code.
 * @param arg   IOCTL parameter.
 * @return      zero on success, or `-1` on error.
 */
static int uinput_ioctl_ptr(int fd, const char *name, unsigned long code, void *arg) {
    log_message(2, "UINPUT: ioctl(%s, ...)", name);
    if (ioctl(fd, code, arg) == -1) {
        log_message(-1, "UINPUT: ioctl %s error: %s", name, strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Issue an IOCTL with an integer parameter for a list of values.
 *
 * @param fd    device handle.
 * @param name  IOCTL name (for messages).
 * @param code  IOCTL code.
 * @param ids   a list of items to issue IOCTL for.
 * @return      zero on success, or `-1` on error.
 */
static int uinput_ioctl_ids(int fd, const char *name, unsigned long code, const struct udotool_obj_id ids[]) {
    for (const struct udotool_obj_id *idptr = ids; idptr->name != NULL; idptr++)
        if (uinput_ioctl_int(fd, name, code, idptr->value) < 0)
            return -1;
    return 0;
}

/**
 * Issue an IOCTL with an integer parameter for all high-resolution
 * wheel axes.
 *
 * @param fd    device handle.
 * @param name  IOCTL name (for messages).
 * @param code  IOCTL code.
 * @return      zero on success, or `-1` on error.
 */
static int uinput_ioctl_hires(int fd, const char *name, unsigned long code) {
    for (int i = 0; UINPUT_HIRES_AXIS[i].lo_axis >= 0; i++)
        if (uinput_ioctl_int(fd, name, code, UINPUT_HIRES_AXIS[i].hi_axis) < 0)
            return -1;
    return 0;
}

/**
 * Setup emulation parameters for UINPUT.
 *
 * @param fd  device handle.
 * @return    zero on success, or `-1` on error.
 */
static int uinput_setup(int fd) {
    if (uinput_ioctl_int(fd, "UI_SET_EVBIT", UI_SET_EVBIT, EV_KEY) < 0 ||
        uinput_ioctl_int(fd, "UI_SET_EVBIT", UI_SET_EVBIT, EV_REL) < 0 ||
        uinput_ioctl_int(fd, "UI_SET_EVBIT", UI_SET_EVBIT, EV_ABS) < 0)
        return -1;
    if (uinput_ioctl_int(fd, "UI_SET_PROPBIT", UI_SET_PROPBIT, INPUT_PROP_POINTER) < 0 ||
        uinput_ioctl_int(fd, "UI_SET_PROPBIT", UI_SET_PROPBIT, INPUT_PROP_DIRECT) < 0)
        return -1;

    for (int key = 0; key < KEY_MAX; key++) {
#ifdef UDOTOOL_LIBINPUT_QUIRK
        if (key >= BTN_TOOL_PEN && key <= BTN_TOOL_QUADTAP)
            continue;
#endif // UDOTOOL_LIBINPUT_QUIRK
        if (uinput_ioctl_int(fd, "UI_SET_KEYBIT", UI_SET_KEYBIT, key) < 0)
            return -1;
    }

    if (uinput_ioctl_ids(fd, "UI_SET_RELBIT", UI_SET_RELBIT, UINPUT_REL_AXES) < 0 ||
        uinput_ioctl_hires(fd, "UI_SET_RELBIT", UI_SET_RELBIT) < 0)
        return -1;

    if (uinput_ioctl_ids(fd, "UI_SET_ABSBIT", UI_SET_ABSBIT, UINPUT_ABS_AXES) < 0)
        return -1;
    struct uinput_abs_setup axis;
    for (const struct udotool_obj_id *idptr = UINPUT_ABS_AXES; idptr->name != NULL; idptr++) {
        memset(&axis, 0, sizeof(axis));
        axis.code    = idptr->value;
        axis.absinfo = UINPUT_AXIS_DEF;
        if (uinput_ioctl_ptr(fd, "UI_ABS_SETUP", UI_ABS_SETUP, &axis) < 0)
            return -1;
    }

    struct uinput_setup setup;
    memset(&setup, 0, sizeof(setup));
    setup.id = UINPUT_ID;
    strncpy(setup.name, UINPUT_DEVNAME, UINPUT_MAX_NAME_SIZE);
    if (uinput_ioctl_ptr(fd, "UI_DEV_SETUP", UI_DEV_SETUP, &setup) < 0)
        return -1;

    return uinput_ioctl_int(fd, "UI_DEV_CREATE", UI_DEV_CREATE, 0);
}

/**
 * Create emulation device, unless already created.
 *
 * On dry run this skips device creation, so open callback won't be called.
 *
 * @return  zero on success, or `-1` on error.
 */
int uinput_open(void) {
    if (UINPUT_FD >= 0)
        return 0;
    log_message(2, "%sUINPUT: open", CFG_DRY_RUN_PREFIX);
    if (CFG_DRY_RUN) {
        UINPUT_FD  = +1000;
        return 0;
    }

    UINPUT_FD = open(UINPUT_DEVICE, O_WRONLY | O_CLOEXEC);
    if (UINPUT_FD < 0) {
        log_message(-1, "UINPUT: device %s open error: %s", UINPUT_DEVICE, strerror(errno));
        return -1;
    }
    if (uinput_setup(UINPUT_FD) < 0) {
        close(UINPUT_FD);
        UINPUT_FD = -1;
        return -1;
    }

    char sysname[PATH_MAX];
    if (uinput_ioctl_ptr(UINPUT_FD, "UI_GET_SYSNAME", UI_GET_SYSNAME(sizeof(sysname)), sysname) == 0) {
        log_message(1, "UINPUT: opened device %s", sysname);
        if (UINPUT_OPEN_CBK != NULL)
            (*UINPUT_OPEN_CBK)(sysname, UINPUT_OPEN_CBK_DATA);
    }
    unsigned version = 0;
    if (uinput_ioctl_ptr(UINPUT_FD, "UI_GET_VERSION", UI_GET_VERSION, &version) == 0)
        log_message(1, "UINPUT: protocol version 0x%04X", version);

    log_message(2, "UINPUT: waiting to settle");
    struct timespec tval;
    memset(&tval, 0, sizeof(tval));
    tval.tv_sec = (time_t)UINPUT_SETTLE_TIME;
    tval.tv_nsec = (long)(NSEC_PER_SEC * (UINPUT_SETTLE_TIME - tval.tv_sec));
    log_message(2, "UINPUT: sleeping for %ld seconds and %ld nanoseconds", (long)tval.tv_sec, tval.tv_nsec);
    if (nanosleep(&tval, NULL) < 0)
        log_message(-1, "UINPUT: error while sleeping: %s", strerror(errno));

    return 0;
}

/**
 * Destroy emulation device, if created.
 */
void uinput_close() {
    if (UINPUT_FD < 0)
        return;
    if (!CFG_DRY_RUN) {
        uinput_ioctl_int(UINPUT_FD, "UI_DEV_DESTROY", UI_DEV_DESTROY, 0);
        close(UINPUT_FD);
    }
    UINPUT_FD = -1;
}

/**
 * Emit emulated event.
 *
 * @param type   event type.
 * @param code   event code.
 * @param value  event value.
 * @return       zero on success, or `-1` on error.
 */
static int uinput_emit(int type, int code, int value) {
    log_message(2, "UINPUT: injecting event 0x%04X, code 0x%04X, value %d",
        (unsigned)type, (unsigned)code, value);
    struct timeval ts;
    gettimeofday(&ts, NULL);
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.input_event_sec  = ts.tv_sec;
    ev.input_event_usec = ts.tv_usec;
    ev.type  = type;
    ev.code  = code;
    ev.value = value;
    if (write(UINPUT_FD, &ev, sizeof(ev)) == -1) {
        log_message(-1, "UINPUT write error: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Emit a synchronization event.
 *
 * @return  zero on success, or `-1` on error.
 */
int uinput_sync(void) {
    if (uinput_open() < 0)
        return -1;
    log_message(2, "%sUINPUT: sync", CFG_DRY_RUN_PREFIX);
    if (CFG_DRY_RUN)
        return 0;
    return uinput_emit(EV_SYN, SYN_REPORT, 0);
}

/**
 * Emit a key/button event.
 *
 * @param key    key/button code.
 * @param value  `1` for key down, or `0` for key up.
 * @param sync   if not zero, also emit a synchronization event.
 * @return       zero on success, or `-1` on error.
 */
int uinput_keyop(int key, int value, int sync) {
    if (uinput_open() < 0)
        return -1;
    if (key < 0)
        key = BTN_LEFT;
    log_message(2, "%sUINPUT: key%s 0x%03X%s",
            CFG_DRY_RUN_PREFIX,
            value ? "down" : "up", (unsigned)key, sync ? " (sync)" : "");
    if (CFG_DRY_RUN)
        return 0;
    if (uinput_emit(EV_KEY, key, value) < 0)
        return -1;
    if (sync && uinput_emit(EV_SYN, SYN_REPORT, 0) < 0)
        return -1;
    return 0;
}

/**
 * Emit a relative axis event.
 *
 * @param axis   axis code.
 * @param value  change in position.
 * @param sync   if not zero, also emit a synchronization event.
 * @return       zero on success, or `-1` on error.
 */
int uinput_relop(int axis, double value, int sync) {
    if (uinput_open() < 0)
        return -1;
    log_message(2, "%sUINPUT: rel 0x%02X value %lf%s",
            CFG_DRY_RUN_PREFIX,
            (unsigned)axis, value, sync ? " (sync)" : "");
    if (CFG_DRY_RUN)
        return 0;
    for (int i = 0; UINPUT_HIRES_AXIS[i].lo_axis >= 0; i++)
        if (axis == UINPUT_HIRES_AXIS[i].lo_axis) {
            if (uinput_emit(EV_REL, axis, (int)value) < 0)
                return -1;
            value *= UINPUT_HIRES_AXIS[i].divisor;
            axis   = UINPUT_HIRES_AXIS[i].hi_axis;
        }
    if (uinput_emit(EV_REL, axis, (int)value) < 0)
        return -1;
    if (sync && uinput_emit(EV_SYN, SYN_REPORT, 0) < 0)
        return -1;
    return 0;
}

/**
 * Emit an absolute axis event.
 *
 * @param axis   axis code.
 * @param value  new position.
 * @param sync   if not zero, also emit a synchronization event.
 * @return       zero on success, or `-1` on error.
 */
int uinput_absop(int axis, double value, int sync) {
    if (uinput_open() < 0)
        return -1;
    log_message(2, "%sUINPUT: abs 0x%02X value %lf%s",
            CFG_DRY_RUN_PREFIX,
            (unsigned)axis, value, sync ? " (sync)" : "");
    if (CFG_DRY_RUN)
        return 0;
    if (uinput_emit(EV_ABS, axis, (int)(UINPUT_ABS_MAXVALUE * value)) < 0)
        return -1;
    if (sync && uinput_emit(EV_SYN, SYN_REPORT, 0) < 0)
        return -1;
    return 0;
}
