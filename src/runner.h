// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Execution declarations
 *
 * Copyright (c) 2024 Alec Kojaev
 */
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

struct udotool_loop {
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
    struct udotool_loop
                stack[MAX_LOOP_DEPTH];
};

int run_ctxt_init(struct udotool_exec_context *ctxt);
int run_ctxt_free(struct udotool_exec_context *ctxt);

int run_line_args(struct udotool_exec_context *ctxt, int argc, const char *const argv[]);
int run_line(struct udotool_exec_context *ctxt, const char *line, int in_body);
