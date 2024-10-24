// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Execution declarations
 *
 * Copyright (c) 2024 Alec Kojaev
 */

/**
 * Command opcodes.
 */
enum {
    CMD_HELP = 0,
    // Control transferring commands
    CMD_LOOP = 0x100,
    CMD_IF,
    CMD_ELSE,
    CMD_END,
    CMD_SCRIPT,
    CMD_EXIT,
    // Generic commands
    CMD_SLEEP = 0x200,
    CMD_EXEC,
    CMD_ECHO,
    CMD_SET,
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

/**
 * Command description ("verb").
 */
struct udotool_verb_info {
    const char *verb;         ///< Command name.
    unsigned    cmd;          ///< Command opcode.
    int         min_argc;     ///< Minimum number of non-option arguments.
    int         max_argc;     ///< Maximum number of non-option arguments, or `-1`.
    unsigned    options;      ///< Command options bitmask.
    const char *usage;        ///< Arguments syntax, or `NULL`.
    const char *description;  ///< Human-readable description, or `NULL`.
};

/**
 * Flow control state.
 */
struct udotool_ctrl {
    int         cond;    ///< If zero, this is a loop.
    int         count;   ///< Remaining iterations (loop only).
    struct timeval
                rtime;   ///< End timestamp.
    off_t       offset;  ///< Back offset (loop only).
};

/**
 * Execution context.
 */
struct udotool_exec_context {
    const char *filename;               ///< Script file name.
    unsigned    lineno;                 ///< Current script line number.

    int         body;                   ///< Handle for temporary file.
    size_t      depth;                  ///< Control flow stack depth.
    int         cond_omit;              ///< Omit flag.
    size_t      cond_depth;             ///< Control flow depth in omitted commands.
    struct udotool_ctrl
                stack[MAX_CTRL_DEPTH];  ///< Control flow stack.
};

int   run_ctxt_init(struct udotool_exec_context *ctxt);
int   run_ctxt_free(struct udotool_exec_context *ctxt);
off_t run_ctxt_tell_line(struct udotool_exec_context *ctxt);
int   run_ctxt_jump_line(struct udotool_exec_context *ctxt, off_t offset);
int   run_ctxt_save_line(struct udotool_exec_context *ctxt, const char *line);
int   run_ctxt_replay_lines(struct udotool_exec_context *ctxt);

int run_parse_double(const struct udotool_verb_info *info, const char *text, double *pval);
int run_parse_integer(const struct udotool_verb_info *info, const char *text, int *pval);
int run_parse_condition(const struct udotool_verb_info *info, int argc, const char *const* argv, int *pval);

const struct udotool_verb_info *run_find_verb(const char *verb);
int run_line_args(struct udotool_exec_context *ctxt, int argc, const char *const argv[]);
