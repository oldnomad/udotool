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
#include <wordexp.h>

#include "udotool.h"
#include "runner.h"

static char *whitespace_trim(char *line) {
    char *sp, *ep;

    for (sp = line; *sp != '\0' && isspace(*sp); sp++);
    if (*sp != '\0') {
        for (ep = sp + strlen(sp); ep > sp && isspace(ep[-1]); ep--);
        *ep = '\0';
    }
    return sp;
}

static int parse_script(struct udotool_exec_context *ctxt, FILE *input) {
    char line[MAX_SCRIPT_LINE];
    int ret = 0;

    while (fgets(line, sizeof(line), input) != NULL) {
        ctxt->lineno++;
        wordexp_t words;
        const char *sp = whitespace_trim(line);
        if (*sp == '\0' || *sp == '#' || *sp == ';') // Empty line or comment
            continue;
        words.we_wordv = NULL;
        int wret = wordexp(sp, &words, WRDE_SHOWERR);
        if (wret != 0) {
            switch (wret) {
            case WRDE_BADCHAR:
                log_message(-1, "%s[%u]: illegal character", ctxt->filename, ctxt->lineno);
                break;
            case WRDE_NOSPACE:
                log_message(-1, "%s[%u]: not enough memory", ctxt->filename, ctxt->lineno);
                break;
            case WRDE_SYNTAX:
                log_message(-1, "%s[%u]: shell syntax error", ctxt->filename, ctxt->lineno);
                break;
            default:
                log_message(-1, "%s[%u]: parsing error %d", ctxt->filename, ctxt->lineno, wret);
                break;
            }
            ret = -1;
            goto ON_ERROR;
        }
        if (words.we_wordc > 0) // Empty line can be a result of expansion
            ret = cmd_verb(ctxt, words.we_wordv[0], words.we_wordc - 1, (const char *const*)&words.we_wordv[1]);
    ON_ERROR:
        wordfree(&words);
        if (ret != 0)
            break;
    }
    return ret;
}

int run_script(const char *filename) {
    struct udotool_exec_context ctxt;
    int ret;

    memset(&ctxt, 0, sizeof(ctxt));
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
    int ret2 = run_context_free(&ctxt);
    if (ret == 0)
        ret = ret2;
    return ret;
}

int run_command(int argc, const char *const argv[]) {
    struct udotool_exec_context ctxt;
    memset(&ctxt, 0, sizeof(ctxt));
    int ret;
    if (argc <= 0)
        ret = cmd_verb(&ctxt, "help", argc, argv);
    else
        ret = cmd_verb(&ctxt, argv[0], argc - 1, &argv[1]);
    int ret2 = run_context_free(&ctxt);
    return ret == 0 ? ret2 : ret;
}
