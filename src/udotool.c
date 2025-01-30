// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Main procedure
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udotool.h"
#include "uinput-func.h"
#include "config.h"
#include "execute.h"

/**
 * Full version string.
 */
#define VERSION_STRING PROGRAM_NAME " " PROGRAM_VERSION " " PROGRAM_COPYRIGHT

/**
 * Offset for UINPUT options.
 *
 * This must be hugher than any option code processed in `main()`.
 */
#define UINPUT_OPT_OFFSET 1000

#define QUOTE(v)  #v
#define EQUOTE(v) QUOTE(v)

/**
 * Usage message.
 */
static const char USAGE_NOTICE[] = "Usage: %s [<option>...] <subcommand>...\n\n"
                                   "Options:\n"
                                   "    -i [<file>], --input [<file>]\n"
                                   "        Read commands from a file or from standard input.\n"
                                   "        Use file name '-' for standard input (default).\n"
                                   "    -n, --dry-run\n"
                                   "        Instead of executing provided commands, print what will be done.\n"
                                   "    --settle-time <time>\n"
                                   "        Use specified settle time (default is " EQUOTE(DEFAULT_SETTLE_TIME) ")\n"
                                   "    --dev <dev-path>\n"
                                   "        Use specified UINPUT device.\n"
                                   "    --dev-name <name>\n"
                                   "        Use specified emulated device name.\n"
                                   "    --dev-id <vendor-id>:<product-id>[:<version>]\n"
                                   "        Use specified emulated device ID.\n"
                                   "    -v, --verbose\n"
                                   "        Increase command verbosity.\n"
                                   "        This option can be specified multiple times.\n"
                                   "    -h, --help\n"
                                   "        Print this notice and exit.\n"
                                   "    -V, --version\n"
                                   "        Print version information and exit.\n\n"
                                   "Use subcommand \"help\" to get a list of all available subcommands.\n";
/**
 * Command line options.
 */
static const char SHORT_OPTION[] = "+i:nvhV";
static const struct option LONG_OPTION[] = {
    { "input",       optional_argument, NULL, 'i' },
    { "dry-run",     no_argument,       NULL, 'n' },
    { "verbose",     no_argument,       NULL, 'v' },
    { "help",        no_argument,       NULL, 'h' },
    { "version",     no_argument,       NULL, 'V' },
    { "settle-time", required_argument, NULL, UINPUT_OPT_OFFSET + UINPUT_OPT_SETTLE  },
    { "dev",         required_argument, NULL, UINPUT_OPT_OFFSET + UINPUT_OPT_DEVICE  },
    { "dev-name",    required_argument, NULL, UINPUT_OPT_OFFSET + UINPUT_OPT_DEVNAME },
    { "dev-id",      required_argument, NULL, UINPUT_OPT_OFFSET + UINPUT_OPT_DEVID   },
    { NULL }
};

int         CFG_VERBOSITY = 0;        ///< Message verbosity level.
int         CFG_DRY_RUN = 0;          ///< Dry run mode.
const char *CFG_DRY_RUN_PREFIX = "";  ///< Message prefix for dry run, or an empty string.

/**
 * Print a message.
 *
 * Message levels are:
 * - `-1` for error messages.
 * - `0` for mandatory messages.
 * - positive for verbosity-controlled optional messages.
 *
 * @param level  message level.
 * @param fmt    `printf`-like message format.
 * @param ...    message format arguments.
 */
void log_message(int level, const char *fmt,...) {
    if (level > CFG_VERBOSITY)
        return;
    va_list args;
    va_start(args, fmt);
    if (level > 0)
        fprintf(stderr, "[%d] ", level);
    else if (level < 0)
        fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, fmt, args);
    putc('\n', stderr);
    va_end(args);
}

/**
 * Load UINPUT option from an environment variable.
 *
 * @param opt      UINPUT option code.
 * @param envname  environment variable.
 */
static void load_preset(int opt, const char *envname) {
    const char *envdata = getenv(envname);
    if (envdata == NULL)
        return;
    uinput_set_option(opt, envdata);
}

int main(int argc, char *const argv[]) {
    int opt, optidx, has_file = 0;
    const char *input_file = NULL;

    load_preset(UINPUT_OPT_SETTLE, "UDOTOOL_SETTLE_TIME");
    load_preset(UINPUT_OPT_DEVICE, "UDOTOOL_DEVICE_PATH");
    load_preset(UINPUT_OPT_DEVNAME, "UDOTOOL_DEVICE_NAME");
    load_preset(UINPUT_OPT_DEVID, "UDOTOOL_DEVICE_ID");
    while ((opt = getopt_long(argc, argv, SHORT_OPTION, LONG_OPTION, &optidx)) != -1) {
        if (opt >= UINPUT_OPT_OFFSET) {
            if (uinput_set_option(opt - UINPUT_OPT_OFFSET, optarg) < 0)
                return EXIT_FAILURE;
            continue;
        }
        switch (opt) {
        case 'i':
            has_file = 1;
            if (optarg != NULL && strcmp(optarg, "-") != 0)
                input_file = optarg;
            break;
        case 'n':
            CFG_DRY_RUN = 1;
            CFG_DRY_RUN_PREFIX = "[DRY RUN] ";
            break;
        case 'v':
            ++CFG_VERBOSITY;
            break;
        case 'h':
            printf(USAGE_NOTICE, argv[0]);
            return EXIT_SUCCESS;
        case 'V':
            printf("%s\n", VERSION_STRING);
            return EXIT_SUCCESS;
        default: // '?'
            printf(USAGE_NOTICE, argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (argc <= optind && has_file == 0) {
        printf(USAGE_NOTICE, argv[0]);
        return EXIT_FAILURE;
    }

    if (CFG_DRY_RUN)
        log_message(0, "%sno UINPUT actions will be performed\n", CFG_DRY_RUN_PREFIX);

    int ret;
    if (has_file) {
        if (argc > optind) {
            log_message(-1, "too many arguments for --input mode: %s", argv[optind]);
            ret = -1;
        } else
            ret = exec_file(input_file);
    } else
        ret = exec_args(argc - optind, (const char *const*)&argv[optind]);
    uinput_close();
    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
