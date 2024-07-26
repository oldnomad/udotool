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
    size_t      backref;
};

struct udotool_exec_context {
    int         run_mode;
    size_t      ref;

    size_t      depth;
    struct udotool_loop
                stack[MAX_LOOP_DEPTH];

    size_t      max_size;
    size_t      size;
    struct udotool_cmd
               *cmds;
};

const struct udotool_verb_info *run_find_verb(const char *verb);
int run_verb(const struct udotool_verb_info *info, struct udotool_exec_context *ctxt, int argc, const char *const argv[]);

int run_context_run(struct udotool_exec_context *ctxt);
int run_context_cmd(struct udotool_exec_context *ctxt, const struct udotool_verb_info *info,
                    int argc, const char *const argv[]);
int run_context_free(struct udotool_exec_context *ctxt);
