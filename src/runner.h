// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Execution declarations
 *
 * Copyright (c) 2024 Alec Kojaev
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

struct udotool_verb_info {
    const char *verb;
    unsigned    cmd;
    int         min_argc, max_argc;
    unsigned    options;
    const char *usage;
    const char *description;
};

struct udotool_cmd {
    const struct udotool_verb_info
               *info;
    int         argc;
    const char *const*
                argv;
};

struct udotool_ctrl {
    int         cond;
    int         count;
    struct timeval
                rtime;
    off_t       offset;
};

struct udotool_exec_context {
    const char *filename;
    unsigned    lineno;

    int         body;
    size_t      depth;
    int         cond_omit;
    size_t      cond_depth;
    struct udotool_ctrl
                stack[MAX_CTRL_DEPTH];
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
