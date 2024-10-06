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
#include <unistd.h>

#include "udotool.h"
#include "runner.h"
#include "uinput-func.h"

enum {
    CMD_HELP = 0,
    // Control transferring commands
    CMD_LOOP = 0x100,
    CMD_ENDLOOP,
    CMD_SCRIPT,
    // Generic commands
    CMD_SLEEP = 0x200,
    CMD_EXEC,
    CMD_ECHO,
    CMD_SET,
    // UINPUT commands
    CMD_OPEN = 0x300,
    CMD_INPUT,
    // High-level UINPUT commands
    CMD_KEYDOWN,
    CMD_KEYUP,
    CMD_KEY,
    CMD_MOVE,
    CMD_WHEEL,
    CMD_POSITION,
};

enum {
    CMD_OPT_REPEAT = 0,
    CMD_OPT_TIME,
    CMD_OPT_DELAY,
    CMD_OPT_R,
    CMD_OPT_H,
    CMD_OPT_DETACH,
};

#define CMD_OPTMASK(v)   (1u << (v))
#define HAS_OPTION(name) CMD_OPTMASK(CMD_OPT_##name)

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
      "[-time <seconds>] [<N>]",
      "Repeat following commands until nearest 'endloop'." },
    { "endloop",  CMD_ENDLOOP,  0,  0, 0,
      "",
      "End current loop." },
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
    { "help",     CMD_HELP,     0, -1, 0, // NOTE: -axis and -key are regular parameters
      "[<command> | -axis | -key]",
      "Print help information." },
    { NULL }
};
static const struct udotool_obj_id OPTLIST[] = {
    { "repeat", CMD_OPT_REPEAT },
    { "time",   CMD_OPT_TIME   },
    { "delay",  CMD_OPT_DELAY  },
    { "r",      CMD_OPT_R      },
    { "h",      CMD_OPT_H      },
    { "detach", CMD_OPT_DETACH },
    { NULL }
};

static const char  AXIS_KEYDOWN[] = "KEYDOWN";
static const char  AXIS_KEYUP[]   = "KEYUP";

static const struct udotool_verb_info *find_verb(const char *verb) {
    for (const struct udotool_verb_info *info = KNOWN_VERBS; info->verb != NULL; info++)
        if (strcmp(verb, info->verb) == 0)
            return info;
    log_message(-1, "unrecognized subcommand '%s'", verb);
    return NULL;
}

static int parse_double(const struct udotool_verb_info *info, const char *text, double *pval) {
    const char *ep = text;
    double value = strtod(text, (char **)&ep);
    if (ep == text || *ep != '\0') {
        log_message(-1, "%s: error parsing value '%s'", info->verb, text);
        return -1;
    }
    *pval = value;
    return 0;
}

static int parse_integer(const struct udotool_verb_info *info, const char *text, int *pval) {
    const char *ep = text;
    long value = strtol(text, (char **)&ep, 0);
    if (ep == text || *ep != '\0' || value < INT_MIN || value > INT_MAX) {
        log_message(-1, "%s: error parsing value '%s'", info->verb, text);
        return -1;
    }
    *pval = (int)value;
    return 0;
}

static int parse_abs_value(const struct udotool_verb_info *info, const char *text, double *pval) {
    double value = 0;
    if (parse_double(info, text, &value) < 0)
        return -1;
    value /= 100.0;
    if (value < 0 || value > 1.0) {
        log_message(-1, "%s: value is out of range in '%s'", info->verb, text);
        return -1;
    }
    *pval = value;
    return 0;
}

static int parse_rel_value(const struct udotool_verb_info *info, const char *text, double *pval) {
    double value = 0;
    if (parse_double(info, text, &value) < 0)
        return -1;
    if (value < INT_MIN || value > INT_MAX) {
        log_message(-1, "%s: value is out of range in '%s'", info->verb, text);
        return -1;
    }
    *pval = value;
    return 0;
}

static int print_help(int argc, const char *const argv[]) {
    static const char HELP_FMT[] = "%s %s\n    %s\n";

    if (argc <= 0 || argv == NULL) {
        for (const struct udotool_verb_info *info = KNOWN_VERBS; info->verb != NULL; info++)
            printf(HELP_FMT, info->verb, info->usage, info->description);
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
        const struct udotool_verb_info *info = find_verb(argv[i]);
        if (info != NULL)
            printf(HELP_FMT, info->verb, info->usage, info->description);
    }
    return 0;
}

static int calc_rtime(const struct udotool_verb_info *info, double rtime, struct timeval *pval) {
    if (rtime == 0) {
        timerclear(pval);
        return 0;
    }
    if (gettimeofday(pval, NULL) < 0) {
        log_message(-1, "%s: cannot get current time: %s", info->verb, strerror(errno));
        return -1;
    }
    struct timeval tval;
    timerclear(&tval);
    tval.tv_sec = (time_t)rtime;
    tval.tv_usec = (suseconds_t)(USEC_PER_SEC * (rtime - tval.tv_sec));
    timeradd(pval, &tval, pval);
    return 0;
}

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
            if (parse_integer(info, argv[++arg0], &repeat) < 0)
                return -1;
            if (repeat <= 0) {
                log_message(-1, "%s: repeat value is out of range: %s", info->verb, argv[arg0]);
                return -1;
            }
            break;
        case CMD_OPT_TIME:
            if ((arg0 + 1) >= argc)
                goto ON_NO_OPTVAL;
            if (parse_double(info, argv[++arg0], &rtime) < 0)
                return -1;
            if (rtime <= MIN_SLEEP_SEC || rtime > MAX_SLEEP_SEC) {
                log_message(-1, "%s: run time value is out of range: %s", info->verb, argv[arg0]);
                return -1;
            }
            break;
        case CMD_OPT_DELAY:
            if ((arg0 + 1) >= argc)
                goto ON_NO_OPTVAL;
            if (parse_double(info, argv[++arg0], &delay) < 0)
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
    if (ctxt->run_mode == 0) {
        switch (info->cmd) {
        case CMD_LOOP:
            if (run_ctxt_cmd(ctxt, info, argc + arg0, argv - arg0) < 0)
                return -1;
            ctxt->depth++;
            if (ctxt->depth >= MAX_LOOP_DEPTH) {
                log_message(-1, "%s: too many levels (max %d)", info->verb, MAX_LOOP_DEPTH);
                return -1;
            }
            return 0;
        case CMD_ENDLOOP:
            if (ctxt->depth == 0) {
                log_message(-1, "%s: endloop without loop",info->verb);
                return -1;
            }
            if (run_ctxt_cmd(ctxt, info, argc + arg0, argv - arg0) < 0)
                return -1;
            ctxt->depth--;
            if (ctxt->depth == 0)
                return run_ctxt_exec(ctxt);
            return 0;
        default:
            if (ctxt->depth > 0)
                return run_ctxt_cmd(ctxt, info, argc + arg0, argv - arg0);
            break;
        }
    }
    int key;
    double value;
    struct timeval tval;
    switch (info->cmd) {
    case CMD_HELP:
        return print_help(argc, argv);
    case CMD_LOOP:
        // run_mode == 1, depth < (MAX_LOOP_DEPTH - 1)
        if (argc > 0) {
            if (parse_integer(info, argv[0], &repeat) < 0)
                return -1;
            if (repeat <= 0) {
                log_message(-1, "%s: loop counter is out of range: %s", info->verb, argv[0]);
                return -1;
            }
        }
        if (calc_rtime(info, rtime, &tval) < 0)
            return -1;
        if (!timerisset(&tval) && repeat == 0) {
            log_message(-1, "%s: either counter or time should be specified", info->verb);
            return -1;
        }
        if (repeat == 0)
            repeat = INT_MAX;
        log_message(1, "%s: counter = %d, run time = %lu.%06lu", info->verb,
            repeat, (unsigned long)tval.tv_sec, (unsigned long)tval.tv_usec);
        ctxt->stack[ctxt->depth++] = (struct udotool_loop){ .count = repeat, .rtime = tval, .backref = ctxt->ref };
        return 0;
    case CMD_ENDLOOP:
        // run_mode == 1, 0 < depth < MAX_LOOP_DEPTH
        {
            struct udotool_loop *loop = &ctxt->stack[ctxt->depth - 1];
            loop->count--;
            if (gettimeofday(&tval, NULL) < 0) {
                log_message(-1, "%s: cannot get current time: %s", info->verb, strerror(errno));
                return -1;
            }
            if (loop->count <= 0 || (timerisset(&loop->rtime) && !timercmp(&loop->rtime, &tval, >))) {
                log_message(1, "%s: loop ended, counter = %d, current time = %lu.%06lu", info->verb,
                    loop->count, (unsigned long)tval.tv_sec, (unsigned long)tval.tv_usec);
                ctxt->depth--;
                return 0;
            }
            log_message(1, "%s: continue, counter = %d, current time = %lu.%06lu", info->verb,
                loop->count, (unsigned long)tval.tv_sec, (unsigned long)tval.tv_usec);
            ctxt->ref = loop->backref;
        }
        return 0;
    case CMD_SCRIPT:
        if (argc <= 0 || argv == NULL)
            return run_script(NULL);
        return run_script(argv[0]);
    case CMD_SLEEP:
        if (parse_double(info, argv[0], &delay) < 0)
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

int run_line(struct udotool_exec_context *ctxt, int argc, const char *const argv[]) {
    const char *verb;
    if (argc > 0) {
        verb = argv[0];
        --argc;
        ++argv;
    } else
        verb = "help";
    const struct udotool_verb_info *info = find_verb(verb);
    if (info == NULL)
        return -1;
    return run_verb(ctxt, info, argc, argv);
}

int run_ctxt_exec(struct udotool_exec_context *ctxt) {
    ctxt->run_mode = 1;
    int ret = 0;
    for (ctxt->ref = 0; ctxt->ref < ctxt->size; ctxt->ref++) {
        struct udotool_cmd *cmd = &ctxt->cmds[ctxt->ref];
        if ((ret = run_verb(ctxt, cmd->info, cmd->argc, cmd->argv)) < 0)
            break;
    }
    int ret2 = run_ctxt_free(ctxt);
    return ret == 0 ? ret2 : ret;
}

int run_ctxt_cmd(struct udotool_exec_context *ctxt, const struct udotool_verb_info *info,
                 int argc, const char *const argv[]) {
    const char ** argv_new = NULL;
    if (argc > 0) {
        size_t arg_size = (argc + 1) * sizeof(char *);
        for (int i = 0; i < argc; i++)
            if (argv[i] != NULL)
                arg_size += strlen(argv[i]) + 1;
        char *argp = malloc(arg_size);
        if (argp == NULL) {
            log_message(-1, "not enough memory");
            return -1;
        }
        argv_new = (const char **)argp;
        argp += (argc + 1) * sizeof(char *);
        for (int i = 0; i < argc; i++) {
            if (argv[i] == NULL)
                argv_new[i] = NULL;
            else {
                argv_new[i] = argp;
                strcpy(argp, argv[i]);
                argp += strlen(argv[i]) + 1;
            }
        }
        argv_new[argc] = NULL;
    }
    if (ctxt->size >= ctxt->max_size) {
        size_t size_new = ctxt->max_size + 16;
        struct udotool_cmd *cmds_new = realloc(ctxt->cmds, size_new * sizeof(*cmds_new));
        if (cmds_new == NULL) {
            free(argv_new);
            log_message(-1, "not enough memory");
            return -1;
        }
        ctxt->max_size = size_new;
        ctxt->cmds = cmds_new;
    }
    ctxt->cmds[ctxt->size++] = (struct udotool_cmd){ .info = info, .argc = argc, .argv = argv_new };
    return 0;
}

void run_ctxt_init(struct udotool_exec_context *ctxt) {
    memset(ctxt, 0, sizeof(*ctxt));
}

int run_ctxt_free(struct udotool_exec_context *ctxt) {
    int ret = 0;
    if (ctxt->depth > 0) {
        log_message(-1, "loop was not terminated, depth %zu", ctxt->depth);
        ret = -1;
    }
    if (ctxt->cmds != NULL) {
        for (size_t i = 0; i < ctxt->size; i++)
            free((void *)ctxt->cmds[i].argv);
        free(ctxt->cmds);
    }
    memset(ctxt, 0, sizeof(*ctxt));
    return ret;
}
