// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Declarations for UINPUT functions
 *
 * Copyright (c) 2024 Alec Kojaev
 */
enum {
    UINPUT_OPT_DEVICE = 0,
    UINPUT_OPT_DEVNAME,
    UINPUT_OPT_DEVID,
};

enum {
    UDOTOOL_AXIS_REL = 0x01,
    UDOTOOL_AXIS_ABS = 0x02,
    UDOTOOL_AXIS_BOTH = 0x03,
};

struct udotool_obj_id {
    const char *name;
    int value;
};

struct udotool_hires_axis {
    int lo_axis;
    int hi_axis;
    int divisor;
};

extern const struct udotool_obj_id UINPUT_REL_AXES[];
extern const struct udotool_obj_id UINPUT_ABS_AXES[];
extern const struct udotool_obj_id UINPUT_KEYS[];

extern const struct udotool_hires_axis UINPUT_HIRES_AXIS[];

extern const int UINPUT_MAIN_REL_AXES[2][3];
extern const int UINPUT_MAIN_ABS_AXES[2][3];
extern const int UINPUT_MAIN_WHEEL_AXES[2];

int uinput_set_option(int option, const char *value);

int uinput_find_key(const char *prefix, const char *key);
int uinput_find_axis(const char *prefix, const char *name, unsigned mask, int *pflag);

int uinput_open(void);
void uinput_close(void);
int uinput_sync(void);
int uinput_keyop(int key, int value, int sync);
int uinput_relop(int axis, double value, int sync);
int uinput_absop(int axis, double value, int sync);
