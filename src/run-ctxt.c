// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Command execution
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

#include "udotool.h"
#include "runner.h"

int run_ctxt_init(struct udotool_exec_context *ctxt) {
    memset(ctxt, 0, sizeof(*ctxt));
#ifndef UDOTOOL_NOMMAN_TWEAK
    if ((ctxt->body = memfd_create("body", MFD_CLOEXEC)) < 0) {
#else
    if ((ctxt->body = open("/tmp", O_TMPFILE|O_RDWR|O_EXCL)) < 0) {
#endif
        log_message(-1, "error creating body storage: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int run_ctxt_free(struct udotool_exec_context *ctxt) {
    int ret = 0;
    if (ctxt->depth > 0) {
        log_message(-1, "loop was not terminated, depth %zu", ctxt->depth);
        ret = -1;
    }
    close(ctxt->body);
    memset(ctxt, 0, sizeof(*ctxt));
    ctxt->body = -1;
    return ret;
}

int run_ctxt_save_line(struct udotool_exec_context *ctxt, const char *line) {
    size_t len = strlen(line);
    if (write(ctxt->body, &len, sizeof(len)) < 0 ||
        write(ctxt->body, line, len) < 0) {
        log_message(-1, "error writing body storage: %s", strerror(errno));
        return -1;
    }
    return 0;
}

off_t run_ctxt_tell_line(struct udotool_exec_context *ctxt) {
    off_t offset = lseek(ctxt->body, 0, SEEK_CUR);
    if (offset == (off_t)-1)
        log_message(-1, "error telling body storage: %s", strerror(errno));
    return offset;
}

int run_ctxt_jump_line(struct udotool_exec_context *ctxt, off_t offset) {
    if (lseek(ctxt->body, offset, SEEK_SET) == (off_t)-1) {
        log_message(-1, "error rewinding body storage: %s", strerror(errno));
        return -1;
    }
    return 0;
}

static int ctxt_drop_lines(struct udotool_exec_context *ctxt) {
    if (ftruncate(ctxt->body, 0) < 0) {
        log_message(-1, "error truncating body storage: %s", strerror(errno));
        return -1;
    }
    return run_ctxt_jump_line(ctxt, 0);
}

int run_ctxt_replay_lines(struct udotool_exec_context *ctxt) {
    int ret;
    if ((ret = run_ctxt_jump_line(ctxt, 0)) != 0)
        return ret;
    size_t len = 0;
    ssize_t rlen;
    char *line;
    while ((rlen = read(ctxt->body, &len, sizeof(len))) > 0) {
        if (rlen != sizeof(len)) {
ON_EOF_ERROR:
            log_message(-1, "unexpected EOF while reading body storage");
            ret = -1;
            break;
        }
        if ((line = malloc(len + 1)) == NULL) {
            log_message(-1, "not enough memory");
            ret = -1;
            break;
        }
        rlen = read(ctxt->body, line, len);
        ret = -1;
        if (rlen == (ssize_t)len) {
            line[len] = '\0';
            ret = run_line(ctxt, line, 1);
        }
        free(line);
        if (rlen < 0 || ret != 0)
            break;
        if (rlen != (ssize_t)len)
            goto ON_EOF_ERROR;
    }
    if (rlen < 0) {
        log_message(-1, "error reading body storage: %s", strerror(errno));
        ret = -1;
    }
    return ctxt_drop_lines(ctxt);
}
