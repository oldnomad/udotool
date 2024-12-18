// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Script parsing
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udotool.h"
#include "runner.h"

/**
 * Trim leading and trailing whitespace from a line.
 *
 * Line modification is done in-place.
 *
 * @param line  line (will be modified).
 * @return      trimmed line.
 */
static char *whitespace_trim(char *line) {
    char *sp, *ep;

    for (sp = line; *sp != '\0' && isspace(*sp); sp++);
    if (*sp != '\0') {
        for (ep = sp + strlen(sp); ep > sp && isspace(ep[-1]); ep--);
        *ep = '\0';
    }
    return sp;
}

/**
 * Parse a script file.
 *
 * This actually just copies script to a temporary file,
 * in a binary format.
 *
 * @param ctxt   execution context.
 * @param input  input script.
 * @return       zero on success, `-1` on error.
 */
static int parse_script(struct udotool_exec_context *ctxt, FILE *input) {
    char line[MAX_SCRIPT_LINE];
    int ret = 0;

    while (fgets(line, sizeof(line), input) != NULL) {
        ctxt->lineno++;
        const char *sp = whitespace_trim(line);
        if (*sp == '\0' || *sp == '#' || *sp == ';') // Empty line or comment
            continue;
        ret = run_ctxt_save_line(ctxt, sp);
        if (ret != 0)
            break;
    }
    return ret;
}

/**
 * Run a script.
 *
 * Script file name is either a plain text file name, or a single minus
 * sign (`"-"`), or `NULL`. In the latter two cases, standard input will
 * be parsed and run.
 *
 * @param filename  input script file name, or `NULL`.
 * @return          zero on success, `-1` on error.
 */
int run_script(const char *filename) {
    struct udotool_exec_context ctxt;
    int ret;

    if ((ret = run_ctxt_init(&ctxt)) != 0)
        return ret;
    ctxt.lineno = 0;
    if (filename == NULL || (filename[0] == '-' && filename[1] == '\0')) {
        ctxt.filename = "-";
        ret = parse_script(&ctxt, stdin);
    } else {
        ctxt.filename = filename;
        FILE *input = fopen(filename, "re");
        if (input == NULL) {
            log_message(-1, "%s: cannot open script file: %s", filename, strerror(errno));
            return -1;
        }
        ret = parse_script(&ctxt, input);
        fclose(input);
    }
    if (ret == 0)
        ret = run_ctxt_replay_lines(&ctxt);
    int ret2 = run_ctxt_free(&ctxt);
    if (ret == 0)
        ret = ret2;
    return ret;
}

/**
 * Run a single command from command line.
 *
 * @param argc  number of arguments.
 * @param argv  array of arguments.
 * @return      zero on success, `-1` on error.
 */
int run_command(int argc, const char *const argv[]) {
    struct udotool_exec_context ctxt;
    int ret, ret2;

    if ((ret = run_ctxt_init(&ctxt)) != 0)
        return ret;
    ret = run_line_args(&ctxt, argc, argv);
    if (ret > 0)
        ret = 0;
    ret2 = run_ctxt_free(&ctxt);
    return ret == 0 ? ret2 : ret;
}
