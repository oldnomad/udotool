// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * UINPUT I/O functions
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#include <linux/uinput.h>

#include "udotool.h"
#include "uinput-func.h"

static char UINPUT_DEVICE[PATH_MAX] = "/dev/uinput";
static char UINPUT_DEVNAME[UINPUT_MAX_NAME_SIZE] = "udotool";
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

static int UINPUT_FD = -1;

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
    default:
        log_message(-1, "UINPUT: unrecognized option code %d", option);
        return -1;
    }
    return 0;
}

static int uinput_ioctl_int(int fd, const char *name, unsigned long code, int arg) {
    log_message(2, "UINPUT: ioctl(%s, 0x%04X)", name, (unsigned)arg);
    if (ioctl(fd, code, arg) == -1) {
        log_message(-1, "UINPUT: ioctl %s error: %s", name, strerror(errno));
        return -1;
    }
    return 0;
}

static int uinput_ioctl_ptr(int fd, const char *name, unsigned long code, void *arg) {
    log_message(2, "UINPUT: ioctl(%s, ...)", name);
    if (ioctl(fd, code, arg) == -1) {
        log_message(-1, "UINPUT: ioctl %s error: %s", name, strerror(errno));
        return -1;
    }
    return 0;
}

static int uinput_ioctl_ids(int fd, const char *name, unsigned long code, const struct udotool_obj_id ids[]) {
    for (const struct udotool_obj_id *idptr = ids; idptr->name != NULL; idptr++)
        if (uinput_ioctl_int(fd, name, code, idptr->value) < 0)
            return -1;
    return 0;
}

static int uinput_ioctl_hires(int fd, const char *name, unsigned long code) {
    for (int i = 0; UINPUT_HIRES_AXIS[i].lo_axis >= 0; i++)
        if (uinput_ioctl_int(fd, name, code, UINPUT_HIRES_AXIS[i].hi_axis) < 0)
            return -1;
    return 0;
}

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

int uinput_open(void) {
    if (UINPUT_FD >= 0)
        return 0;
    log_message(1, "%sUINPUT: open", CFG_DRY_RUN_PREFIX);
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
        setenv("UDOTOOL_SYSNAME", sysname, 1);
    }
    unsigned version = 0;
    if (uinput_ioctl_ptr(UINPUT_FD, "UI_GET_VERSION", UI_GET_VERSION, &version) == 0)
        log_message(1, "UINPUT: protocol version 0x%04X", version);
    return 0;
}

void uinput_close() {
    if (UINPUT_FD < 0)
        return;
    if (!CFG_DRY_RUN) {
        uinput_ioctl_int(UINPUT_FD, "UI_DEV_DESTROY", UI_DEV_DESTROY, 0);
        close(UINPUT_FD);
    }
    UINPUT_FD = -1;
}

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

int uinput_sync(void) {
    if (uinput_open() < 0)
        return -1;
    log_message(1, "%sUINPUT: sync", CFG_DRY_RUN_PREFIX);
    if (CFG_DRY_RUN)
        return 0;
    return uinput_emit(EV_SYN, SYN_REPORT, 0);
}

int uinput_keyop(int key, int value, int sync) {
    if (uinput_open() < 0)
        return -1;
    if (key < 0)
        key = BTN_LEFT;
    log_message(1, "%sUINPUT: key%s 0x%03X%s",
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

int uinput_relop(int axis, double value, int sync) {
    if (uinput_open() < 0)
        return -1;
    log_message(1, "%sUINPUT: rel 0x%02X value %lf%s",
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

int uinput_absop(int axis, double value, int sync) {
    if (uinput_open() < 0)
        return -1;
    log_message(1, "%sUINPUT: abs 0x%02X value %lf%s",
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
