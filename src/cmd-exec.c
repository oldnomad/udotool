// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Tool `open`: initialize UINPUT
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <spawn.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "udotool.h"
#include "cmd.h"

int cmd_exec(int argc, const char *const argv[]) {
    (void)argc;
    pid_t pid;
    const char *command = argv[0];
    int err, status;
    if ((err = posix_spawnp(&pid, command, NULL, NULL, (char *const*)argv, environ)) != 0) {
        log_message(-1, "exec: cannot execute command '%s': %s", command, strerror(err));
        return -1;
    }
    log_message(1, "exec: started command '%s' at PID %d", command, pid);
    if (waitpid(pid, &status, 0) < 0) {
        log_message(-1, "exec: error waiting for command '%s' PID %d", command, pid);
        return -1;
    }
    log_message(1, "exec: command '%s' at PID %d finished with status %d", command, pid, status);
    return status == 0 ? 0 : -1;
}
