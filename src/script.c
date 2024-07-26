// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Script parsing
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udotool.h"
#include "runner.h"

static const char WHITESPACE[] = " \t\n\r\f\v"; // Whitespace characters

static const char *split_token(char *line, char **endptr, int allow_quote) {
    // NOTE: !!! This modifies `line` !!!
    if (line == NULL)
        return NULL;
    char *sp = line + strspn(line, WHITESPACE);
    if (*sp == '\0')
        return NULL; // End of line
    if (allow_quote && (*sp == '"' || *sp == '\'')) {
        // Quoted string
        char quote_char = *sp++;
        for (char *ep = sp; *ep != '\0'; ep++) {
            if (*ep == quote_char) {
                *ep = '\0';
                *endptr = &ep[1];
                return sp;
            }
            if (*ep == '\\') {
                if (ep[1] == '\0') {
                    // Escape at EOL, preserve
                    *endptr = NULL;
                    return sp;
                }
                strcpy(ep, &ep[1]);
                continue;
            }
        }
        // Quoted string is not closed, autoclose
        *endptr = NULL;
        return sp;
    }
    char *ep = sp + strcspn(sp, WHITESPACE);
    *endptr = *ep == '\0' ? NULL : &ep[1];
    *ep = '\0';
    return sp;
}

static int parse_script(FILE *input) {
    struct udotool_exec_context ctxt;
    char line[MAX_SCRIPT_LINE];
    int ret = 0;

    memset(&ctxt, 0, sizeof(ctxt));
    while (fgets(line, sizeof(line), input) != NULL) {
        const struct udotool_verb_info *info;
        char *endptr = NULL;
        const char *verb = split_token(line, &endptr, 0);
        if (verb == NULL || *verb == '#' || *verb == ';') // Empty line or comment
            continue;
        if ((info = run_find_verb(verb)) == NULL) {
            ret = -1;
            break;
        }

        int argc = 0, maxarg = 0;
        const char **argv = NULL;
        const char *token;
        while ((token = split_token(endptr, &endptr, 1)) != NULL) {
            if ((maxarg - argc) < 2) {
                maxarg += 16;
                const char **vec = realloc(argv, maxarg*sizeof(char *));
                if (vec == NULL) {
                    log_message(-1, "script: not enough memory");
                    ret = -1;
                    goto ON_ERROR;
                }
                argv = vec;
            }
            argv[argc++] = token;
        }
        if (argv != NULL)
            argv[argc] = NULL;
        ret = run_verb(info, &ctxt, argc, argv);

    ON_ERROR:
        free(argv);
        if (ret != 0)
            break;
    }
    int ret2 = run_context_free(&ctxt);
    return ret == 0 ? ret2 : ret;
}

int run_script(const char *filename) {
    if (filename == NULL || (filename[0] == '-' && filename[1] == '\0'))
        return parse_script(stdin);

    FILE *input = fopen(filename, "re");
    if (input == NULL) {
        log_message(-1, "%s: cannot open script file: %s", filename, strerror(errno));
        return -1;
    }
    int ret = parse_script(input);
    fclose(input);

    return ret;
}
