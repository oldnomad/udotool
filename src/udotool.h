// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Common declarations
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#define MAX_OBJECT_NAME          64 ///< Maximum length of an object (axis or key) name.

#define MAX_SLEEP_SEC         86400 ///< Maximum delay, in seconds.
#define MIN_SLEEP_SEC         0.001 ///< Minimum delay, in seconds.
#define DEFAULT_SETTLE_TIME   0.500 ///< Default settle time after setup, in seconds.

#define UINPUT_ABS_MAXVALUE 1000000 ///< Maximum absolute axis position.

#define NSEC_PER_SEC          1.0e9 ///< Nanoseconds per second.
#define USEC_PER_SEC          1.0e6 ///< Microseconds per second.

extern int         CFG_VERBOSITY;
extern int         CFG_DRY_RUN;
extern const char *CFG_DRY_RUN_PREFIX;

void log_message(int level, const char *fmt,...)
    __attribute__ ((format (printf, 2, 3)));
