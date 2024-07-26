// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Command declarations
 *
 * Copyright (c) 2024 Alec Kojaev
 */
enum {
    CMD_HELP = 0,
    // Control transferring commands
    CMD_LOOP = 0x100,
    CMD_ENDLOOP,
    CMD_SCRIPT,
    // Generic commands
    CMD_SLEEP = 0x200,
    CMD_EXEC,
    // UINPUT commands
    CMD_OPEN = 0x300,
    CMD_INPUT,
    // High-level UINPUT commands
    CMD_KEYDOWN,
    CMD_KEYUP,
    CMD_KEY,
    CMD_MOVE,
    CMD_WHEEL,
    CMD_POSITION,
};

enum {
    CMD_OPT_REPEAT = 0,
    CMD_OPT_DELAY,
    CMD_OPT_R,
    CMD_OPT_H,
};
#define CMD_OPTMASK(v) (1u << (v))

int cmd_help(int argc, const char *const argv[]);
int cmd_exec(int argc, const char *const argv[]);
int cmd_sleep(double delay, int internal);
