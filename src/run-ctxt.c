// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Command execution context
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifndef UDOTOOL_NOMMAN_TWEAK
#include <sys/mman.h>
#endif // !UDOTOOL_NOMMAN_TWEAK
#include <unistd.h>
#include <wordexp.h>

#include "udotool.h"
#include "runner.h"

/**
 * Initialize execution context.
 *
 * @param ctxt  execution context.
 * @return      zero on success, `-1` on error.
 */
int run_ctxt_init(struct udotool_exec_context *ctxt) {
    memset(ctxt, 0, sizeof(*ctxt));
#ifndef UDOTOOL_NOMMAN_TWEAK
    if ((ctxt->body = memfd_create("body", MFD_CLOEXEC)) < 0) {
#else
    if ((ctxt->body = open("/tmp", O_TMPFILE|O_RDWR|O_EXCL|O_CLOEXEC)) < 0) {
#endif
        log_message(-1, "error creating body storage: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Free execution context.
 *
 * If on freeing execution context an unfinished control flow block
 * is detected, an error message will be printed.
 *
 * @param ctxt  execution context.
 * @return      zero on success, `-1` on error.
 */
int run_ctxt_free(struct udotool_exec_context *ctxt) {
    int ret = 0;
    if (ctxt->depth > 0 || ctxt->cond_depth > 0) {
        log_message(-1, "control was not terminated, depth %zu/%zu", ctxt->depth, ctxt->cond_depth);
        ret = -1;
    }
    close(ctxt->body);
    memset(ctxt, 0, sizeof(*ctxt));
    ctxt->body = -1;
    return ret;
}

/**
 * Save a script line into the temporary file.
 *
 * @param ctxt  execution context.
 * @param line  script line.
 * @return      zero on success, `-1` on error.
 */
int run_ctxt_save_line(struct udotool_exec_context *ctxt, const char *line) {
    size_t len = strlen(line);
    if (write(ctxt->body, &ctxt->lineno, sizeof(ctxt->lineno)) < 0 ||
        write(ctxt->body, &len, sizeof(len)) < 0 ||
        write(ctxt->body, line, len) < 0) {
        log_message(-1, "error writing body storage: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Read a script line from the temporary file.
 *
 * This function allocates a buffer for the line using `malloc(3)`.
 * It is the caller's responsibility to free it.
 *
 * @param ctxt   execution context.
 * @param pline  pointer to buffer that will receive a pointer to the script line.
 * @return       zero on success, `-1` on error.
 */
static int ctxt_read_line(struct udotool_exec_context *ctxt, char **pline) {
    ssize_t rlen;
    unsigned lineno;
    size_t slen;
    char *line = NULL;

    if ((rlen = read(ctxt->body, &lineno, sizeof(lineno))) != sizeof(lineno)) {
        if (rlen == 0)
            return +1;
ON_ERROR:
        free(line);
        if (rlen < 0)
            log_message(-1, "error reading body storage: %s", strerror(errno));
        else
            log_message(-1, "unexpected EOF while reading body storage");
        return -1;
    }
    if ((rlen = read(ctxt->body, &slen, sizeof(slen))) != sizeof(slen))
        goto ON_ERROR;
    if ((line = malloc(slen + 1)) == NULL) {
        log_message(-1, "not enough memory");
        return -1;
    }
    if ((rlen = read(ctxt->body, line, slen)) != (ssize_t)slen)
        goto ON_ERROR;
    ctxt->lineno = lineno;
    line[slen] = '\0';
    *pline = line;
    return 0;
}

/**
 * Calculate current offset in the temporary file.
 *
 * @param ctxt  execution context.
 * @return      current offset, or `(off_t)-1` on error.
 */
off_t run_ctxt_tell_line(struct udotool_exec_context *ctxt) {
    off_t offset = lseek(ctxt->body, 0, SEEK_CUR);
    if (offset == (off_t)-1)
        log_message(-1, "error telling body storage: %s", strerror(errno));
    return offset;
}

/**
 * Jump to specified offset in the temporary file.
 *
 * @param ctxt    execution context.
 * @param offset  new offset value.
 * @return        zero on success, `-1` on error.
 */
int run_ctxt_jump_line(struct udotool_exec_context *ctxt, off_t offset) {
    if (lseek(ctxt->body, offset, SEEK_SET) == (off_t)-1) {
        log_message(-1, "error rewinding body storage: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Delete contents of the temporary file.
 *
 * @param ctxt  execution context.
 * @return      zero on success, `-1` on error.
 */
static int ctxt_drop_lines(struct udotool_exec_context *ctxt) {
    if (ftruncate(ctxt->body, 0) < 0) {
        log_message(-1, "error truncating body storage: %s", strerror(errno));
        return -1;
    }
    return run_ctxt_jump_line(ctxt, 0);
}

/**
 * Run a script line.
 *
 * This function handles all expansion methods.
 *
 * This function implements line skipping (in `if`/`else`)
 * without expansion. I've tried to keep the behavior of line
 * skipping as close to `wordexp(3)` as possible. In particular,
 * I use `$IFS` to split the first word. However, if the script
 * contains constructions that _expand_ to a control flow
 * construction (`if`/`else`/`loop`/`end`), this will break.
 *
 * @param ctxt  execution context.
 * @param line  script line.
 * @return      zero on success, `-1` on error.
 */
static int ctxt_run_line(struct udotool_exec_context *ctxt, char *line) {
    static const char WHITESPACE[] = " \t\r\n\f\v";
    if (ctxt->cond_omit) {
        log_message(4, "%s[%u]: [%zu/%zu] skipping line: %s", ctxt->filename, ctxt->lineno, ctxt->depth, ctxt->cond_depth, line);
        const char *ifs = getenv("IFS");
        if (ifs == NULL)
            ifs = WHITESPACE;
        size_t sep = strcspn(line, ifs);
        line[sep] = '\0';
        const struct udotool_verb_info *info = run_find_verb(line);
        if (info == NULL)
            return 0;
        switch (info->cmd) {
        case CMD_IF:
        case CMD_LOOP:
            ctxt->cond_depth++;
            break;
        case CMD_END:
            if (ctxt->cond_depth == 0) {
                // Since we are under `cond_omit`, `depth` > 0 is true
                ctxt->depth--;
                ctxt->cond_omit = 0;
            } else
                ctxt->cond_depth--;
            break;
        case CMD_ELSE:
            if (ctxt->cond_depth == 0)
                ctxt->cond_omit = 0;
            break;
        }
        return 0;
    }
    log_message(4, "%s[%u]: [%zu/%zu] executing line: %s", ctxt->filename, ctxt->lineno, ctxt->depth, ctxt->cond_depth, line);
    wordexp_t words;
    words.we_wordv = NULL;
    int ret = wordexp(line, &words, WRDE_SHOWERR);
    if (ret != 0) {
        switch (ret) {
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
            log_message(-1, "%s[%u]: parsing error %d", ctxt->filename, ctxt->lineno, ret);
            break;
        }
        return -1;
    }
    ret = 0;
    if (words.we_wordc > 0) // Empty line can be a result of expansion
        ret = run_line_args(ctxt, words.we_wordc, (const char *const*)words.we_wordv);
    wordfree(&words);
    return ret;
}

/**
 * Replay the script saved to the temporary file.
 *
 * After execution completes, this function destroys the contents
 * of the temporary file.
 *
 * @param ctxt  execution context.
 * @return      zero on success, `-1` on error.
 */
int run_ctxt_replay_lines(struct udotool_exec_context *ctxt) {
    int ret;
    if ((ret = run_ctxt_jump_line(ctxt, 0)) != 0)
        return ret;
    char *line = NULL;
    while ((ret = ctxt_read_line(ctxt, &line)) == 0) {
        ret = ctxt_run_line(ctxt, line);
        free(line);
        if (ret != 0)
            break;
    }
    return ctxt_drop_lines(ctxt);
}
