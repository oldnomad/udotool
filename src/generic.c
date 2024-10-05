// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Generic commands
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <errno.h>
#include <spawn.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "udotool.h"

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

int cmd_exec(int detach, int argc, const char *const argv[]) {
    pid_t pid;
    const char *command = argv[0];
    posix_spawnattr_t attr;
    int err, status;

    (void)argc;
    log_message(1, "exec: executing command '%s'%s", command, detach ? ", detached" : "");
    if (posix_spawnattr_init(&attr) != 0) {
        log_message(-1, "exec: error initializing attributes: %s", strerror(errno));
        return -1;
    }
    if (detach) {
        if (posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID) != 0) {
            log_message(-1, "exec: error initializing attributes: %s", strerror(errno));
            posix_spawnattr_destroy(&attr);
            return -1;
        }
    }
    err = posix_spawnp(&pid, command, NULL, &attr, (char *const*)argv, environ);
    posix_spawnattr_destroy(&attr);
    if (err != 0) {
        log_message(-1, "exec: cannot execute command '%s': %s", command, strerror(err));
        return -1;
    }
    log_message(1, "exec: started command '%s' at PID %d", command, pid);
    if (detach)
        return 0;
    if (waitpid(pid, &status, 0) < 0) {
        log_message(-1, "exec: error waiting for command '%s' PID %d", command, pid);
        return -1;
    }
    log_message(1, "exec: command '%s' at PID %d finished with status %d", command, pid, status);
    return status == 0 ? 0 : -1;
}
