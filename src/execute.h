// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Execution declarations
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#define EXECUTOR_NAME "Jim Tcl" ///< Executor environment name

/**
 * Execute a command specified in command line.
 *
 * @param argc  number of command elements.
 * @param argv  command elements.
 * @return      exit code.
 */
int exec_args(int argc, const char *const*argv);

/**
 * Execute a script from a file or from stdin.
 *
 * @param filename  file name, or null for stdin.
 * @return          exit code.
 */
int exec_file(const char *filename);

/**
 * Print executor version number.
 *
 * @param prefix  prefix to print before the version.
 */
void exec_print_version(const char *prefix);
