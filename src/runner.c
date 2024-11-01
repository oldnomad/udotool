// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Command execution
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "udotool.h"
#include "runner.h"
#include "uinput-func.h"

/**
 * Maximum number of digits in a decimal representation.
 *
 * Note that this overshoots the exact value, e.g. for a 32-bit integer
 * it gives 11 (exact value is 10), and for a 64-bit one it gives 22
 * (exact value is 19).
 *
 * @param sz  size of an integer type.
 * @return maximum number of digits.
 */
#define MAX_VALUE_TEXT(sz) (((sz)*CHAR_BIT - 1)/3 + 1)

/**
 * Command option codes.
 */
enum {
    CMD_OPT_REPEAT = 0,  ///< `-repeat` (used in `key`).
    CMD_OPT_TIME,        ///< `-time` (used in `key` and `loop`).
    CMD_OPT_DELAY,       ///< `-delay` (used in `key`).
    CMD_OPT_R,           ///< `-r` (used in `move` and `position`).
    CMD_OPT_H,           ///< `-h` (used in `wheel`).
    CMD_OPT_DETACH,      ///< `-detach` (used in `exec`).
};

#define CMD_OPTMASK(v)   (1u << (v))
#define HAS_OPTION(name) CMD_OPTMASK(CMD_OPT_##name)

/**
 * Command descriptions.
 */
static const struct udotool_verb_info KNOWN_VERBS[] = {
    { "keydown",  CMD_KEYDOWN,  1, -1, 0,
      "<key>...",
      "Press down specified keys." },
    { "keyup",    CMD_KEYUP,    1, -1, 0,
      "<key>...",
      "Release specified keys." },
    { "key",      CMD_KEY,      1, -1, HAS_OPTION(REPEAT)|HAS_OPTION(TIME)|HAS_OPTION(DELAY),
      "[-repeat <N>] [-time <seconds>] [-delay <seconds>] <key>...",
      "Press down and release specified keys." },
    { "move",     CMD_MOVE,     1,  3, HAS_OPTION(R),
      "[-r] <delta-x> [<delta-y> [<delta-z>]]",
      "Move pointer by specified delta." },
    { "wheel",    CMD_WHEEL,    1,  1, HAS_OPTION(H),
      "[-h] <delta>",
      "Move wheel by specified delta." },
    { "position", CMD_POSITION, 1,  3, HAS_OPTION(R),
      "[-r] <pos-x> [<pos-y> [<pos-z>]]",
      "Move pointer to specified absolute position." },
    { "open",     CMD_OPEN,     0,  0, 0,
      "",
      "Initialize UINPUT." },
    { "input",    CMD_INPUT,    1, -1, 0,
      "<axis>=<value>...",
      "Generate a packet of input values." },
    { "loop",     CMD_LOOP,     0,  1, HAS_OPTION(TIME),
      "[-time <seconds>] [<N>]\n ...\nend",
      "Repeat a block of commands." },
    { "if",       CMD_IF,       1, -1, 0,
      "<condition>\n ...\n[else\n ...]\nend",
      "Execute a block of commands under condition." },
    { "else",     CMD_ELSE,     0,  0, 0,
      NULL,
      NULL },
    { "break",    CMD_BREAK,    0,  1, 0,
      "[<n>]",
      "Break from one or more loops." },
    { "end",      CMD_END,      0,  0, 0,
      NULL,
      NULL },
    { "sleep",    CMD_SLEEP,    1,  1, 0,
      "<seconds>",
      "Sleep for specified time." },
    { "exec",     CMD_EXEC,     1, -1, HAS_OPTION(DETACH),
      "[-detach] <command> [<arg>...]",
      "Execute specified command." },
    { "echo",     CMD_ECHO,     0, -1, 0,
      "<arg>...",
      "Print specified arguments to standard output." },
    { "set",      CMD_SET,      1, 2, 0,
      "<var-name> [<value>]",
      "Set specified environment variable to specified value." },
    { "script",   CMD_SCRIPT,   1,  1, 0,
      "<filename>",
      "Execute commands from specified file." },
    { "exit",     CMD_EXIT,     0,  0, 0,
      "",
      "Finish executing current script." },
    { "help",     CMD_HELP,     0, -1, 0, // NOTE: -axis and -key are regular parameters
      "[<command> | -axis | -key]",
      "Print help information." },
    { NULL }
};
/**
 * Command option mappings.
 */
static const struct udotool_obj_id OPTLIST[] = {
    { "repeat", CMD_OPT_REPEAT },
    { "time",   CMD_OPT_TIME   },
    { "delay",  CMD_OPT_DELAY  },
    { "r",      CMD_OPT_R      },
    { "h",      CMD_OPT_H      },
    { "detach", CMD_OPT_DETACH },
    { NULL }
};

/**
 * Pseudo-axis name for key down event.
 */
static const char  AXIS_KEYDOWN[] = "KEYDOWN";
/**
 * Pseudo-axis name for key up event.
 */
static const char  AXIS_KEYUP[] = "KEYUP";

/**
 * Get a command description.
 *
 * @param verb  command name.
 * @return      command description, or `NULL` if not found.
 */
const struct udotool_verb_info *run_find_verb(const char *verb) {
    for (const struct udotool_verb_info *info = KNOWN_VERBS; info->verb != NULL; info++)
        if (strcmp(verb, info->verb) == 0)
            return info;
    log_message(-1, "unrecognized subcommand '%s'", verb);
    return NULL;
}

/**
 * Parse an absolute axis value.
 *
 * Values for absolute axes are specified in percent of the maximum value.
 *
 * @param info  verb for which the value is parsed.
 * @param text  text to parse.
 * @param pval  pointer to buffer for parsed value.
 * @return      zero on success, or `-1` on error.
 */
static int parse_abs_value(const struct udotool_verb_info *info, const char *text, double *pval) {
    double value = 0;
    if (run_parse_double(info, text, &value) < 0)
        return -1;
    value /= 100.0;
    if (value < 0 || value > 1.0) {
        log_message(-1, "%s: value is out of range in '%s'", info->verb, text);
        return -1;
    }
    *pval = value;
    return 0;
}

/**
 * Parse a relative axis value.
 *
 * Values for relative axes are usually integer, except for the wheel axes.
 *
 * @param info  verb for which the value is parsed.
 * @param text  text to parse.
 * @param pval  pointer to buffer for parsed value.
 * @return      zero on success, or `-1` on error.
 */
static int parse_rel_value(const struct udotool_verb_info *info, const char *text, double *pval) {
    double value = 0;
    if (run_parse_double(info, text, &value) < 0)
        return -1;
    if (value < INT_MIN || value > INT_MAX) {
        log_message(-1, "%s: value is out of range in '%s'", info->verb, text);
        return -1;
    }
    *pval = value;
    return 0;
}

/**
 * Print help message.
 *
 * If no arguments are specified, this function prints help for all commands.
 *
 * If specified, an argument may be a command name, or `"-axis"`, or `"-keys"`.
 *
 * Unknown commands are skipped, with an error message, but no error status.
 *
 * @param argc  number of arguments.
 * @param argv  array of arguments.
 * @return      zero.
 */
static int print_help(int argc, const char *const argv[]) {
    static const char HELP_FMT[] = "%s %s\n    %s\n";

    if (argc <= 0 || argv == NULL) {
        for (const struct udotool_verb_info *info = KNOWN_VERBS; info->verb != NULL; info++)
            if (info->usage != NULL && info->description != NULL) {
                printf(HELP_FMT, info->verb, info->usage, info->description);
                putchar('\n');
            }
        return 0;
    }
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-axis") == 0) {
                printf("Relative axes:\n");
                for (const struct udotool_obj_id *idptr = UINPUT_REL_AXES; idptr->name != NULL; idptr++)
                    printf(" - %s (0x%02X)\n", idptr->name, (unsigned)idptr->value);
                printf("Absolute axes:\n");
                for (const struct udotool_obj_id *idptr = UINPUT_ABS_AXES; idptr->name != NULL; idptr++)
                    printf(" - %s (0x%02X)\n", idptr->name, (unsigned)idptr->value);
            } else if (strcmp(argv[i], "-keys") == 0) {
                printf("Known keys:\n");
                for (const struct udotool_obj_id *idptr = UINPUT_KEYS; idptr->name != NULL; idptr++)
                    printf(" - %s (0x%03X)\n", idptr->name, (unsigned)idptr->value);
            } else
                log_message(0, "unknown section %s", argv[i]);
            continue;
        }
        const struct udotool_verb_info *info = run_find_verb(argv[i]);
        if (info != NULL && info->usage != NULL && info->description != NULL)
            printf(HELP_FMT, info->verb, info->usage, info->description);
    }
    return 0;
}

/**
 * Calculate end time for a timed operation.
 *
 * @param rtime   time, in seconds.
 * @param currts  pointer to current timestamp.
 * @param endts   pointer to buffer for calculated timestamp.
 * @return        zero on success, or `-1` on error.
 */
static int calc_rtime(double rtime, const struct timeval *currts, struct timeval *endts) {
    if (rtime == 0) {
        timerclear(endts);
        return 0;
    }
    struct timeval tval;
    timerclear(&tval);
    tval.tv_sec = (time_t)rtime;
    tval.tv_usec = (suseconds_t)(USEC_PER_SEC * (rtime - tval.tv_sec));
    timeradd(currts, &tval, endts);
    return 0;
}

/**
 * Set environment variables for next loop iteration.
 *
 * @param ctrl    loop control state, or `NULL` to unset.
 * @param currts  pointer to current timestamp.
 */
static void loop_setenv(const struct udotool_ctrl *ctrl, struct timeval *currts) {
    if (ctrl == NULL) {
        unsetenv("UDOTOOL_LOOP_COUNT");
        unsetenv("UDOTOOL_LOOP_RTIME");
        return;
    }
    char cbuf[MAX_VALUE_TEXT(sizeof(ctrl->count)) + 1];
    char rbuf[MAX_VALUE_TEXT(sizeof(time_t)) + 5];
    snprintf(cbuf, sizeof(cbuf), "%d", ctrl->count);
    if (timerisset(&ctrl->etime)) {
        struct timeval tval;
        timerclear(&tval);
        timersub(&ctrl->etime, currts, &tval);
        time_t sec = tval.tv_sec;
        unsigned msec = (unsigned)((tval.tv_usec + 500)/1000);
        if (sec < 0 || msec > 1000)
            strcpy(rbuf, "ERR");
        else
            snprintf(rbuf, sizeof(rbuf), "%ld.%03u", (long)tval.tv_sec, msec);
    } else
        strcpy(rbuf, "*");
    setenv("UDOTOOL_LOOP_COUNT", cbuf, 1);
    setenv("UDOTOOL_LOOP_RTIME", rbuf, 1);
}

/**
 * Execute a command.
 *
 * Note that only non-omitted commands get that far. For example,
 * if in `if/else/end` the condition was false, we never get `else`
 * in this function, it's processed elsewhere.
 *
 * @param ctxt  execution context.
 * @param info  verb to execute.
 * @param argc  number of arguments.
 * @param argv  array of arguments.
 * @return      zero on success, or `-1` on error.
 */
static int run_verb(struct udotool_exec_context *ctxt, const struct udotool_verb_info *info, int argc, const char *const argv[]) {
    int repeat = 0, alt = 0;
    double delay = DEFAULT_SLEEP_SEC, rtime = 0;
    int arg0;
    for (arg0 = 0; arg0 < argc && argv[arg0][0] == '-'; arg0++) {
        int optval = -1;
        for (const struct udotool_obj_id *opt = OPTLIST; opt->name != NULL; opt++)
            if ((info->options & CMD_OPTMASK(opt->value)) != 0 && strcmp(&argv[arg0][1], opt->name) == 0) {
                optval = opt->value;
                break;
            }
        if (optval < 0)
            break;
        switch (optval) {
        case CMD_OPT_REPEAT:
            if ((arg0 + 1) >= argc) {
        ON_NO_OPTVAL:
                log_message(-1, "%s: missing parameter for option %s", info->verb, argv[arg0]);
                return -1;
            }
            if (run_parse_integer(info, argv[++arg0], &repeat) < 0)
                return -1;
            if (repeat <= 0) {
                log_message(-1, "%s: repeat value is out of range: %s", info->verb, argv[arg0]);
                return -1;
            }
            break;
        case CMD_OPT_TIME:
            if ((arg0 + 1) >= argc)
                goto ON_NO_OPTVAL;
            if (run_parse_double(info, argv[++arg0], &rtime) < 0)
                return -1;
            if (rtime <= MIN_SLEEP_SEC || rtime > MAX_SLEEP_SEC) {
                log_message(-1, "%s: run time value is out of range: %s", info->verb, argv[arg0]);
                return -1;
            }
            break;
        case CMD_OPT_DELAY:
            if ((arg0 + 1) >= argc)
                goto ON_NO_OPTVAL;
            if (run_parse_double(info, argv[++arg0], &delay) < 0)
                return -1;
            if (delay <= MIN_SLEEP_SEC || delay > MAX_SLEEP_SEC) {
                log_message(-1, "%s: delay value is out of range: %s", info->verb, argv[arg0]);
                return -1;
            }
            break;
        case CMD_OPT_R:
        case CMD_OPT_H:
        case CMD_OPT_DETACH:
            alt = 1;
            break;
        default:
            log_message(-1, "%s: unrecognized option %s", info->verb, argv[arg0]);
            return -1;
        }
    }
    argc -= arg0;
    argv += arg0;

    if (argc < info->min_argc) {
        log_message(-1, "%s: not enough arguments", info->verb);
        return -1;
    }
    if (info->max_argc >= 0 && argc > info->max_argc) {
        log_message(-1, "%s: too many arguments", info->verb);
        return -1;
    }
    int key;
    double value;
    struct timeval endts, currts;
    off_t offset;
    struct udotool_ctrl *ctrl;
    switch (info->cmd) {
    case CMD_HELP:
        return print_help(argc, argv);
    case CMD_LOOP:
        if (argc > 0) {
            if (run_parse_integer(info, argv[0], &repeat) < 0)
                return -1;
            if (repeat <= 0) {
                log_message(-1, "%s: loop counter is out of range: %s", info->verb, argv[0]);
                return -1;
            }
        }
        if (gettimeofday(&currts, NULL) < 0) {
            log_message(-1, "%s: cannot get current time: %s", info->verb, strerror(errno));
            return -1;
        }
        if (calc_rtime(rtime, &currts, &endts) < 0)
            return -1;
        if (!timerisset(&endts) && repeat == 0) {
            log_message(-1, "%s: either counter or time should be specified", info->verb);
            return -1;
        }
        if (repeat == 0)
            repeat = INT_MAX;
        log_message(1, "%s: counter = %d, end time = %lu.%06lu", info->verb,
            repeat, (unsigned long)endts.tv_sec, (unsigned long)endts.tv_usec);
        if ((offset = run_ctxt_tell_line(ctxt)) == (off_t)-1)
            return -1;
        if (ctxt->depth >= (MAX_CTRL_DEPTH - 1)) {
            log_message(-1, "%s: too many levels (max %d)", info->verb, MAX_CTRL_DEPTH);
            return -1;
        }
        ctxt->stack[ctxt->depth++] = (struct udotool_ctrl){
            .cond = CTRL_COND_LOOP,
            .count = repeat, .etime = endts, .offset = offset
        };
        loop_setenv(&ctxt->stack[ctxt->depth - 1], &currts);
        return 0;
    case CMD_IF:
        if (run_parse_condition(info, argc, argv, &repeat) < 0)
            return -1;
        ctxt->cond_omit = !repeat;
        log_message(1, "%s: condition is %s", info->verb, repeat ? "true" : "false");
        if (ctxt->depth >= (MAX_CTRL_DEPTH - 1)) {
            log_message(-1, "%s: too many levels (max %d)", info->verb, MAX_CTRL_DEPTH);
            return -1;
        }
        ctxt->stack[ctxt->depth++] = (struct udotool_ctrl){ .cond = CTRL_COND_IF };
        return 0;
    case CMD_ELSE:
        // If we are here, that means that `if` branch was taken
        ctxt->cond_omit = 1;
        ctxt->cond_depth = 0;
        return 0;
    case CMD_BREAK:
        if (argc > 0) {
            if (run_parse_integer(info, argv[0], &repeat) < 0)
                return -1;
            if (repeat <= 0) {
                log_message(-1, "%s: loop depth is out of range: %s", info->verb, argv[0]);
                return -1;
            }
        } else
            repeat = 1;
        if (ctxt->depth <= 0) {
            log_message(-1, "%s: mismatched context", info->verb);
            return -1;
        }
        if (gettimeofday(&currts, NULL) < 0) {
            log_message(-1, "%s: cannot get current time: %s", info->verb, strerror(errno));
            return -1;
        }
        ctxt->cond_omit = 1;
        ctxt->cond_depth = 0;
        for (size_t d = ctxt->depth; d > 0; d--) {
            ctrl = &ctxt->stack[d - 1];
            if (repeat > 0) {
                if (CTRL_IS_LOOP(ctrl))
                    repeat--;
                ctxt->cond_depth++;
                ctxt->depth--;
            } else if (CTRL_IS_LOOP(ctrl)) {
                loop_setenv(ctrl, &currts);
                break;
            } else
                loop_setenv(NULL, &currts);
        }
        if (repeat > 0) {
            log_message(-1, "%s: mismatched context", info->verb);
            return -1;
        }
        // We've overshot by 1
        ctxt->cond_depth--;
        ctxt->depth++;
        log_message(1, "%s: going up %zu frames", info->verb, ctxt->cond_depth + 1);
        return 0;
    case CMD_END:
        if (ctxt->depth <= 0) {
            log_message(-1, "%s: mismatched context", info->verb);
            return -1;
        }
        ctrl = &ctxt->stack[ctxt->depth - 1];
        if (CTRL_IS_LOOP(ctrl)) {
            ctrl->count--;
            if (gettimeofday(&currts, NULL) < 0) {
                log_message(-1, "%s: cannot get current time: %s", info->verb, strerror(errno));
                return -1;
            }
            if (ctrl->count <= 0 || (timerisset(&ctrl->etime) && !timercmp(&ctrl->etime, &currts, >))) {
                log_message(1, "%s: loop ended, counter = %d, current time = %lu.%06lu", info->verb,
                    ctrl->count, (unsigned long)currts.tv_sec, (unsigned long)currts.tv_usec);
                ctxt->depth--;
                loop_setenv(NULL, &currts);
                for (size_t depth = ctxt->depth; depth > 0; depth--)
                    if (ctxt->stack[ctxt->depth - 1].cond == 0) {
                        loop_setenv(&ctxt->stack[ctxt->depth - 1], &currts);
                        break;
                    }
                return 0;
            }
            log_message(1, "%s: continue, counter = %d, current time = %lu.%06lu", info->verb,
                ctrl->count, (unsigned long)currts.tv_sec, (unsigned long)currts.tv_usec);
            int ret;
            if ((ret = run_ctxt_jump_line(ctxt, ctrl->offset)) != 0)
                return ret;
            loop_setenv(ctrl, &currts);
        } else
            ctxt->depth--;
        return 0;
    case CMD_EXIT:
        ctxt->depth = 0;
        return +1;
    case CMD_SCRIPT:
        if (argc <= 0 || argv == NULL)
            return run_script(NULL);
        return run_script(argv[0]);
    case CMD_SLEEP:
        if (run_parse_double(info, argv[0], &delay) < 0)
            return -1;
        if (delay <= MIN_SLEEP_SEC || delay > MAX_SLEEP_SEC) {
            log_message(-1, "%s: delay is out of range: %s", info->verb, argv[0]);
            return -1;
        }
        return cmd_sleep(delay, 0);
    case CMD_EXEC:
        return cmd_exec(alt, argc, argv);
    case CMD_ECHO:
        return cmd_echo(argc, argv);
    case CMD_SET:
        return cmd_set(argv[0], argv[1]);
    case CMD_OPEN:
        return uinput_open();
    case CMD_INPUT:
        for (int i = 0; i < argc; i++) {
            const char *sep = strchr(argv[i], '=');
            if (sep == NULL) {
                log_message(-1, "%s: missing separator in '%s'", info->verb, argv[i]);
                return -1;
            }
            size_t nlen = sep - argv[i];
            if (nlen >= MAX_OBJECT_NAME) {
                log_message(-1, "%s: axis name is too long in '%s'", info->verb, argv[i]);
                return -1;
            }
            char obj_name[MAX_OBJECT_NAME];
            memcpy(obj_name, argv[i], nlen);
            obj_name[nlen] = '\0';
            ++sep;
            if (strcasecmp(AXIS_KEYDOWN, obj_name) == 0) {
                if ((key = uinput_find_key(info->verb, sep)) < 0)
                    return -1;
                if (uinput_keyop(key, 1, 0) < 0)
                    return -1;
                continue;
            }
            if (strcasecmp(AXIS_KEYUP, obj_name) == 0) {
                if ((key = uinput_find_key(info->verb, sep)) < 0)
                    return -1;
                if (uinput_keyop(key, 0, 0) < 0)
                    return -1;
                continue;
            }
            int axis, abs_flag = 0;
            if ((axis = uinput_find_axis(info->verb, obj_name, UDOTOOL_AXIS_BOTH, &abs_flag)) < 0)
                return -1;
            value = 0;
            if (abs_flag) {
                if (parse_abs_value(info, sep, &value) < 0)
                    return -1;
                if (uinput_absop(axis, value, 0) < 0)
                    return -1;
            } else {
                if (parse_rel_value(info, sep, &value) < 0)
                    return -1;
                if (uinput_relop(axis, value, 0) < 0)
                    return -1;
            }
        }
        if (uinput_sync() < 0)
            return -1;
        return 0;
    case CMD_KEYDOWN:
        for (int i = 0; i < argc; i++) {
            if ((key = uinput_find_key(info->verb, argv[i])) < 0)
                return -1;
            if (uinput_keyop(key, 1, 1) < 0)
                return -1;
        }
        return 0;
    case CMD_KEYUP:
        for (int i = 0; i < argc; i++) {
            if ((key = uinput_find_key(info->verb, argv[i])) < 0)
                return -1;
            if (uinput_keyop(key, 0, 1) < 0)
                return -1;
        }
        return 0;
    case CMD_KEY:
        if (rtime != 0) {
            double maxcnt = rtime / delay;
            if (repeat == 0 || maxcnt < repeat)
                repeat = (int)maxcnt;
        }
        if (repeat == 0)
            repeat = 1;
        log_message(1, "%s: counter = %d", info->verb, repeat);
        for (int cnt = 0; cnt < repeat; cnt++) {
            for (int i = 0; i < argc; i++) {
                if ((key = uinput_find_key(info->verb, argv[i])) < 0)
                    return -1;
                if (uinput_keyop(key, 1, 1) < 0)
                    return -1;
            }
            for (int i = argc - 1; i >= 0; i--) {
                if ((key = uinput_find_key(info->verb, argv[i])) < 0)
                    return -1;
                if (uinput_keyop(key, 0, 1) < 0)
                    return -1;
            }
            if (cmd_sleep(delay, 1) < 0)
                return -1;
        }
        return 0;
    case CMD_MOVE:
        for (int i = 0; i < argc && i < 3; i++) {
            value = 0;
            if (parse_rel_value(info, argv[i], &value) < 0)
                return -1;
            if (uinput_relop(UINPUT_MAIN_REL_AXES[alt][i], value, 0) < 0)
                return -1;
        }
        return uinput_sync();
    case CMD_WHEEL:
        value = 0;
        if (parse_rel_value(info, argv[0], &value) < 0)
            return -1;
        return uinput_relop(UINPUT_MAIN_WHEEL_AXES[alt], value, 1);
    case CMD_POSITION:
        for (int i = 0; i < argc && i < 3; i++) {
            value = 0;
            if (parse_abs_value(info, argv[i], &value) < 0)
                return -1;
            if (uinput_absop(UINPUT_MAIN_ABS_AXES[alt][i], value, 0) < 0)
                return -1;
        }
        return uinput_sync();
    }
    log_message(-1, "%s: unsupported", info->verb);
    return -1;
}

/**
 * Execute an expanded command line.
 *
 * If no arguments are specified, `help` is executed.
 *
 * @param ctxt  execution context.
 * @param argc  number of arguments.
 * @param argv  array of arguments.
 * @return      zero on success, or `-1` on error.
 */
int run_line_args(struct udotool_exec_context *ctxt, int argc, const char *const argv[]) {
    const char *verb;
    if (argc > 0) {
        verb = argv[0];
        --argc;
        ++argv;
    } else
        verb = "help";
    const struct udotool_verb_info *info = run_find_verb(verb);
    if (info == NULL)
        return -1;
    return run_verb(ctxt, info, argc, argv);
}
