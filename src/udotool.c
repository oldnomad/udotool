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

#include "udotool.h"
#include "uinput-func.h"

#define VERSION_STRING PROGRAM_NAME " " PROGRAM_VERSION " " PROGRAM_COPYRIGHT

#define UINPUT_OPT_OFFSET 1000

static const char USAGE_NOTICE[] = "Usage: %s [<option>...] <subcommand>...\n\n"
                                   "Options:\n"
                                   "    -i <file>, --input <file>\n"
                                   "        Read commands from a file.\n"
                                   "        Use file name '-' for standard input.\n"
                                   "    -n, --dry-run\n"
                                   "        Instead of executing provided commands, print what will be done.\n"
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
static const char SHORT_OPTION[] = "+i:nvhV";
static const struct option LONG_OPTION[] = {
    { "input",       required_argument, NULL, 'i' },
    { "dry-run",     no_argument,       NULL, 'n' },
    { "verbose",     no_argument,       NULL, 'v' },
    { "help",        no_argument,       NULL, 'h' },
    { "version",     no_argument,       NULL, 'V' },
    { "dev",         required_argument, NULL, UINPUT_OPT_OFFSET + UINPUT_OPT_DEVICE  },
    { "dev-name",    required_argument, NULL, UINPUT_OPT_OFFSET + UINPUT_OPT_DEVNAME },
    { "dev-id",      required_argument, NULL, UINPUT_OPT_OFFSET + UINPUT_OPT_DEVID   },
    { NULL }
};

static int  CFG_VERBOSITY = 0;
int         CFG_DRY_RUN = 0;
const char *CFG_DRY_RUN_PREFIX = "";

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

int main(int argc, char *const argv[]) {
    int opt, optidx;
    const char *input_file = NULL;

    while ((opt = getopt_long(argc, argv, SHORT_OPTION, LONG_OPTION, &optidx)) != -1) {
        if (opt >= UINPUT_OPT_OFFSET) {
            if (uinput_set_option(opt - UINPUT_OPT_OFFSET, optarg) < 0)
                return -1;
            continue;
        }
        switch (opt) {
        case 'i':
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
    if (argc <= optind && input_file == NULL) {
        printf(USAGE_NOTICE, argv[0]);
        return EXIT_FAILURE;
    }

    if (CFG_DRY_RUN)
        log_message(-1, "%sno UINPUT actions will be performed\n", CFG_DRY_RUN_PREFIX);

    int ret;
    if (input_file != NULL) {
        if (argc > optind) {
            log_message(-1, "too many arguments for --input mode");
            ret = -1;
        } else
            ret = run_script(input_file);
    } else
        ret = run_command(argc - optind, (const char *const*)&argv[optind]);
    uinput_close();
    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
