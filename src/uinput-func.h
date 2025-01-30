// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Declarations for UINPUT functions
 *
 * Copyright (c) 2024 Alec Kojaev
 */

/**
 * UINPUT option codes.
 */
enum {
    UINPUT_OPT_DEVICE = 0,  ///< UINPUT device path.
    UINPUT_OPT_DEVNAME,     ///< Emulated device name.
    UINPUT_OPT_DEVID,       ///< Emulated device ID.
    UINPUT_OPT_SETTLE,      ///< Device settle time.
};

/**
 * Axis type flag masks.
 */
enum {
    UDOTOOL_AXIS_REL = 0x01,   ///< Relative axes only.
    UDOTOOL_AXIS_ABS = 0x02,   ///< Absolute axes only.
    UDOTOOL_AXIS_BOTH = 0x03,  ///< Both types of axes.
};

/**
 * Named item.
 */
struct udotool_obj_id {
    const char *name;  ///< Item name.
    int value;         ///< Item value.
};

/**
 * High-resolution wheel axis mapping.
 */
struct udotool_hires_axis {
    int lo_axis;  ///< Low-resolution axis code.
    int hi_axis;  ///< High-resolution axis code.
    int divisor;  ///< Conversion factor.
};

/**
 * Device open callback.
 */
typedef void (*udotool_open_callback_t)(const char *sysname, void *data);

extern const struct udotool_obj_id UINPUT_REL_AXES[];
extern const struct udotool_obj_id UINPUT_ABS_AXES[];
extern const struct udotool_obj_id UINPUT_KEYS[];

extern const struct udotool_hires_axis UINPUT_HIRES_AXIS[];

int uinput_set_option(int option, const char *value);
int uinput_get_option(int option, char *buffer, size_t bufsize);
void uinput_set_open_callback(udotool_open_callback_t callback, void *data);

int uinput_find_key(const char *prefix, const char *key);
int uinput_find_axis(const char *prefix, const char *name, unsigned mask, int *pflag);

int uinput_open(void);
void uinput_close(void);
int uinput_sync(void);
int uinput_keyop(int key, int value, int sync);
int uinput_relop(int axis, double value, int sync);
int uinput_absop(int axis, double value, int sync);
