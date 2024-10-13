// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Common declarations
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#define MAX_SCRIPT_LINE        4096 // Maximum line length for scripts
#define MAX_OBJECT_NAME          64 // Maximum length of an object (axis or key) name.

#define MAX_SLEEP_SEC         86400 // Maximum `sleep` delay, seconds
#define MIN_SLEEP_SEC         0.001 // Minimum `sleep` delay, seconds
#define DEFAULT_SLEEP_SEC     0.050 // Default loop delay
#define DEFAULT_SETTLE_TIME   0.500 // Default settle time after setup

#define MAX_CTRL_DEPTH           16 // Maximum flow control stack depth
#define MAX_EVAL_DEPTH           16 // Maximum evaluation stack depth

#define UINPUT_ABS_MAXVALUE 1000000 // Maximum absolute position

#define NSEC_PER_SEC          1.0e9 // Nanoseconds per second
#define USEC_PER_SEC          1.0e6 // Microseconds per second

extern int         CFG_DRY_RUN;
extern const char *CFG_DRY_RUN_PREFIX;

void log_message(int level, const char *fmt,...)
    __attribute__ ((format (printf, 2, 3)));

int run_script(const char *filename);
int run_command(int argc, const char *const argv[]);

int cmd_echo(int argc, const char *const argv[]);
int cmd_exec(int detach, int argc, const char *const argv[]);
int cmd_set(const char *name, const char *value);
int cmd_sleep(double delay, int internal);
