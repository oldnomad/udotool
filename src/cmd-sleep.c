// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Tool `sleep`: sleep for a time
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "udotool.h"
#include "cmd.h"

#define NSEC_PER_SEC 1.0e9 // Nanoseconds per second

int cmd_sleep(double delay, int internal) {
    struct timespec tval;
    memset(&tval, 0, sizeof(tval));
    tval.tv_sec = (time_t)delay;
    tval.tv_nsec = (long)(NSEC_PER_SEC * (delay - tval.tv_sec));
    log_message(internal ? 2 : 1, "sleep: sleeping for %ld seconds and %ld nanoseconds", (long)tval.tv_sec, tval.tv_nsec);
    if (nanosleep(&tval, NULL) < 0) {
        log_message(-1, "sleep: error while sleeping: %s", strerror(errno));
        return -1;
    }
    return 0;
}
