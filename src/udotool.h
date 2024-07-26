// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Common declarations
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#define PROGRAM_NAME      "udotool"
#define PROGRAM_VERSION   "1.0"
#define PROGRAM_COPYRIGHT "(c) 2024 Alec Kojaev"

#define MAX_SCRIPT_LINE        4096 // Maximum line length for scripts
#define MAX_OBJECT_NAME          64 // Maximum length of an object (axis or key) name.

#define MAX_SLEEP_SEC         86400 // Maximum `sleep` delay, seconds
#define MIN_SLEEP_SEC         0.001 // Minimum `sleep` delay, seconds
#define DEFAULT_SLEEP_SEC     0.050 // Default loop delay

#define MAX_LOOP_DEPTH            8 // Maximum loop stack depth

#define UINPUT_ABS_MAXVALUE 1000000 // Maximum absolute position

extern int         CFG_DRY_RUN;
extern const char *CFG_DRY_RUN_PREFIX;

void log_message(int level, const char *fmt,...)
    __attribute__ ((format (printf, 2, 3)));

int run_script(const char *filename);
int run_command(int argc, const char *const argv[]);