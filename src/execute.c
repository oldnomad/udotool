// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Command execution
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>

#include <jim.h>

#include "udotool.h"
#include "execute.h"
#include "uinput-func.h"

static Jim_Interp *exec_init(void);
static int         exec_deinit(Jim_Interp *interp, int err);

static int exec_open     (Jim_Interp *interp, int argc, Jim_Obj *const*argv);
static int exec_input    (Jim_Interp *interp, int argc, Jim_Obj *const*argv);
static int exec_timedloop(Jim_Interp *interp, int argc, Jim_Obj *const*argv);
static int exec_names    (Jim_Interp *interp, int argc, Jim_Obj *const*argv);
static int exec_sleep    (Jim_Interp *interp, int argc, Jim_Obj *const*argv);

/**
 * Extra Tcl commands.
 */
static const struct exec_cmd {
    const char  *name;      ///< Command name.
    Jim_CmdProc *proc;      ///< Command procedure.
    const char  *old_name;  ///< Old command name, if necessary.
} COMMANDS[] = {
    { "open",      exec_open,      NULL },
    { "input",     exec_input,     NULL },
    { "timedloop", exec_timedloop, NULL },
    { "names",     exec_names,     NULL },
    { "sleep",     exec_sleep,     "::internal::sleep" },
    { NULL }
};

/**
 * Bootstrap Tcl script.
 */
static const char PREEXEC_SCRIPT[] = {
#include "exec-tcl.h"
    , 0x00
};

/**
 * Pseudo-axis name for key down event.
 */
static const char AXIS_KEYDOWN[] = "KEYDOWN";
/**
 * Pseudo-axis name for key up event.
 */
static const char AXIS_KEYUP[] = "KEYUP";
/**
 * Pseudo-axis name for sync event.
 */
static const char AXIS_SYNC[] = "SYNC";

int exec_args(int argc, const char *const*argv) {
    Jim_Interp *interp = exec_init();
    if (interp == NULL)
        return -1;
    Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
    if (list == NULL)
        return exec_deinit(interp, -1);
    for (int i = 0; i < argc; i++)
        Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, argv[i], -1));
    int ret = Jim_EvalObj(interp, list);
    return exec_deinit(interp, ret);
}

int exec_file(const char *filename) {
    Jim_Interp *interp = exec_init();
    if (interp == NULL)
        return -1;
    int ret;
    if (filename == NULL)
        ret = Jim_Eval(interp, "eval [info source [stdin read] stdin 1]");
    else
        ret = Jim_EvalFile(interp, filename);
    return exec_deinit(interp, ret);
}

/**
 * Set Tcl variable from UINPUT option.
 *
 * @param interp  interpreter.
 * @param name    variable name.
 * @param opt     option code.
 * @return        error code.
 */
static int set_opt_var(Jim_Interp *interp, const char *name, int opt) {
    char buffer[PATH_MAX];
    if (uinput_get_option(opt, buffer, sizeof(buffer)) < 0) {
        Jim_SetResultFormatted(interp, "error setting variable \"%s\" from option %d", name, opt);
        return JIM_ERR;
    }
    return Jim_SetVariableStrWithStr(interp, name, buffer);
}

static int set_verbosity_var(Jim_Interp *interp) {
    char buffer[32];
    int ret;
    snprintf(buffer, sizeof(buffer), "%d", CFG_VERBOSITY);
    if ((ret = Jim_SetVariableStrWithStr(interp, "::udotool::debug", buffer)) != JIM_OK)
        return ret;
    snprintf(buffer, sizeof(buffer), "%d", CFG_DRY_RUN);
    if ((ret = Jim_SetVariableStrWithStr(interp, "::udotool::dry_run", buffer)) != JIM_OK)
        return ret;
    return JIM_OK;
}

/**
 * Device open callback.
 *
 * @param sysname  device directory name.
 * @param data     interpreter.
 */
static void open_callback(const char *sysname, void *data) {
    Jim_Interp *interp = data;
    Jim_SetVariableStrWithStr(interp, "::udotool::sys_name", sysname);
}

/**
 * Initialize Tcl interpreter.
 *
 * @return new Tcl interpreter.
 */
static Jim_Interp *exec_init() {
    Jim_Interp *interp = Jim_CreateInterp();
    if (interp == NULL)
        return NULL;
    Jim_RegisterCoreCommands(interp);
    int ret;
    if ((ret = Jim_InitStaticExtensions(interp)) != JIM_OK) {
        exec_deinit(interp, ret);
        return NULL;
    }
    for (const struct exec_cmd *cmd = COMMANDS; cmd->name != NULL; cmd++) {
        if (cmd->old_name != NULL) {
            Jim_Obj *name = Jim_NewStringObj(interp, cmd->name, -1);
            Jim_Obj *old_name = Jim_NewStringObj(interp, cmd->old_name, -1);
            if ((ret = Jim_RenameCommand(interp, name, old_name)) != JIM_OK) {
                exec_deinit(interp, ret);
                return NULL;
            }
        }
        if ((ret = Jim_CreateCommand(interp, cmd->name, cmd->proc, NULL, NULL)) != JIM_OK) {
            exec_deinit(interp, ret);
            return NULL;
        }
    }
    if ((ret = set_opt_var(interp, "::udotool::device",      UINPUT_OPT_DEVICE)) != JIM_OK ||
        (ret = set_opt_var(interp, "::udotool::dev_name",    UINPUT_OPT_DEVNAME)) != JIM_OK ||
        (ret = set_opt_var(interp, "::udotool::dev_id",      UINPUT_OPT_DEVID)) != JIM_OK ||
        (ret = set_opt_var(interp, "::udotool::settle_time", UINPUT_OPT_SETTLE)) != JIM_OK ||
        (ret = set_verbosity_var(interp)) != JIM_OK) {
        exec_deinit(interp, ret);
        return NULL;
    }
    uinput_set_open_callback(open_callback, interp);
    if ((ret = Jim_EvalSource(interp, "exec-tcl.tcl", 1, PREEXEC_SCRIPT)) != JIM_OK) {
        exec_deinit(interp, ret);
        return NULL;
    }
    return interp;
}

/**
 * Print a (possibly complex) Tcl Object in a human-readable form.
 *
 * @param interp  interpreter.
 * @param obj     object to print.
 */
static void print_object(Jim_Interp *interp, Jim_Obj *obj) {
    if (obj == NULL)
        return;
    if (Jim_IsDict(obj)) {
        int len = 0;
        Jim_Obj **table = Jim_DictPairs(interp, obj, &len);
        if (table != NULL && len > 0) {
            for (int i = 0; i < len; i += 2) {
                Jim_Obj *key = table[i];
                Jim_Obj *val = table[i + 1];
                log_message(0, "%s: %s", Jim_GetString(key, NULL), Jim_GetString(val, NULL));
            }
        }
        return;
    }
    if (Jim_IsList(obj)) {
        int len = Jim_ListLength(interp, obj);
        for (int i = 0; i < len; i++) {
            Jim_Obj *elem = Jim_ListGetIndex(interp, obj, i);
            log_message(0, "- %s", Jim_GetString(elem, NULL));
        }
        return;
    }
    const char *text = Jim_GetString(obj, NULL);
    if (*text == '\0')
        return;
    log_message(0, "%s", text);
}

/**
 * Destroy Tcl interpreter.
 *
 * @param interp  interpreter.
 * @param err     final error code.
 * @return        exit code.
 */
static int exec_deinit(Jim_Interp *interp, int err) {
    uinput_set_open_callback(NULL, NULL);
    int ret = -1;
    if (err == JIM_ERR)
        Jim_MakeErrorMessage(interp);
    Jim_Obj *result = Jim_GetResult(interp);
    if (err == JIM_ERR)
        log_message(-1, "%s", Jim_GetString(result, NULL));
    else
        print_object(interp, result);
    ret = Jim_GetExitCode(interp);
    Jim_FreeInterp(interp);
    return ret;
}

/**
 * Parse an absolute axis value.
 *
 * Values for absolute axes are specified in percent of the maximum value.
 *
 * @param interp  interpreter.
 * @param obj     object to parse.
 * @param pval    pointer to buffer for parsed value.
 * @return        error code.
 */
static int parse_abs_value(Jim_Interp *interp, Jim_Obj *obj, double *pval) {
    double value = 0;
    int ret = Jim_GetDouble(interp, obj, &value);
    if (ret != JIM_OK)
        return ret;
    value /= 100.0;
    if (value < 0 || value > 1.0) {
        Jim_SetResultFormatted(interp, "value is out of range in \"%#s\"", obj);
        return JIM_ERR;
    }
    *pval = value;
    return JIM_OK;
}

/**
 * Parse a relative axis value.
 *
 * Values for relative axes are usually integer, except for the wheel axes.
 *
 * @param interp  interpreter.
 * @param obj     object to parse.
 * @param pval    pointer to buffer for parsed value.
 * @return        error code.
 */
static int parse_rel_value(Jim_Interp *interp, Jim_Obj *obj, double *pval) {
    double value = 0;
    int ret = Jim_GetDouble(interp, obj, &value);
    if (ret != JIM_OK)
        return ret;
    if (value < INT_MIN || value > INT_MAX) {
        Jim_SetResultFormatted(interp, "value is out of range in \"%#s\"", obj);
        return JIM_ERR;
    }
    *pval = value;
    return JIM_OK;
}

/**
 * Get time.
 *
 * @param interp  interpreter.
 * @param tv      pointer to buffer for time value.
 * @return        error code.
 */
static int get_time(Jim_Interp *interp, struct timeval *tv) {
    if (gettimeofday(tv, NULL) < 0) {
        Jim_SetResultFormatted(interp, "cannot get current time: %s", strerror(errno));
        return JIM_ERR;
    }
    return JIM_OK;
}

/**
 * Tcl command: open
 */
static int exec_open(Jim_Interp *interp, int argc, Jim_Obj *const*argv) {
    if (argc != 1) {
        Jim_WrongNumArgs(interp, 1, argv, "");
        return JIM_ERR;
    }
    if (uinput_open() != 0) {
        Jim_SetResultFormatted(interp, "device setup error");
        return JIM_ERR;
    }
    return JIM_OK;
}

/**
 * Tcl command: input
 */
static int exec_input(Jim_Interp *interp, int argc, Jim_Obj *const*argv) {
    const char *cmd = Jim_String(argv[0]);
    for (int n = 1; n < argc; n++) {
        int llen = Jim_ListLength(interp, argv[n]);
        if (llen == 0 || llen > 2) {
            Jim_SetResultFormatted(interp, "incorrect list length in \"%#s\"", argv[n]);
            return JIM_ERR;
        }
        int do_split = 0;
        Jim_Obj *axis = NULL, *value = NULL;
        if (llen == 1) {
            const char *arg = Jim_String(argv[n]);
            const char *sep = strchr(arg, '=');
            if (sep == NULL) {
                axis = argv[n];
                value = NULL;
            } else {
                do_split = 1;
                axis = Jim_NewStringObj(interp, arg, sep - arg);
                value = Jim_NewStringObj(interp, sep + 1, -1);
            }
        } else {
            axis = Jim_ListGetIndex(interp, argv[n], 0);
            value = Jim_ListGetIndex(interp, argv[n], 1);
        }

        const char *axis_str = Jim_String(axis);
        if (strcasecmp(AXIS_SYNC, axis_str) == 0) {
            if (uinput_sync() < 0) {
                Jim_SetResultFormatted(interp, "device sync error");
                goto ON_ERROR;
            }
            goto ON_CONTINUE;
        }
        if (value == NULL) {
            Jim_SetResultFormatted(interp, "missing separator in \"%#s\"", argv[n]);
            goto ON_ERROR;
        }
        if (strcasecmp(AXIS_KEYDOWN, axis_str) == 0) {
            int key;
            if ((key = uinput_find_key(cmd, Jim_String(value))) < 0) {
                Jim_SetResultFormatted(interp, "unknown key name in \"%#s\"", argv[n]);
                goto ON_ERROR;
            }
            if (uinput_keyop(key, 1, 0) < 0) {
                Jim_SetResultFormatted(interp, "device event error");
                goto ON_ERROR;
            }
            goto ON_CONTINUE;
        }
        if (strcasecmp(AXIS_KEYUP, axis_str) == 0) {
            int key;
            if ((key = uinput_find_key(cmd, Jim_String(value))) < 0) {
                Jim_SetResultFormatted(interp, "unknown key name in \"%#s\"", argv[n]);
                goto ON_ERROR;
            }
            if (uinput_keyop(key, 0, 0) < 0) {
                Jim_SetResultFormatted(interp, "device event error");
                goto ON_ERROR;
            }
            goto ON_CONTINUE;
        }
        int axis_code, abs_flag = 0;
        if ((axis_code = uinput_find_axis(cmd, axis_str, UDOTOOL_AXIS_BOTH, &abs_flag)) < 0) {
ON_ERROR:
            if (do_split) {
                Jim_FreeObj(interp, axis);
                Jim_FreeObj(interp, value);
            }
            return JIM_ERR;
        }
        double dval = 0;
        if (abs_flag) {
            if (parse_abs_value(interp, value, &dval) < 0)
                goto ON_ERROR;
            if (uinput_absop(axis_code, dval, 0) < 0) {
                Jim_SetResultFormatted(interp, "device event error");
                goto ON_ERROR;
            }
        } else {
            if (parse_rel_value(interp, value, &dval) < 0)
                goto ON_ERROR;
            if (uinput_relop(axis_code, dval, 0) < 0) {
                Jim_SetResultFormatted(interp, "device event error");
                goto ON_ERROR;
            }
        }
ON_CONTINUE:
        if (do_split) {
            Jim_FreeObj(interp, axis);
            Jim_FreeObj(interp, value);
        }
    }
    if (uinput_sync() < 0) {
        Jim_SetResultFormatted(interp, "device sync error");
        return JIM_ERR;
    }
    return JIM_OK;
}

/**
 * Tcl command: timedloop
 */
static int exec_timedloop(Jim_Interp *interp, int argc, Jim_Obj *const*argv) {
    if (argc < 3 || argc > 6) {
        Jim_WrongNumArgs(interp, 1, argv, "time ?num? ?varTime? ?varNum? body");
        return JIM_ERR;
    }
    int ret;

    double rep_time = 0;
    jim_wide rep_num = -1;
    Jim_Obj *var_time = NULL;
    Jim_Obj *var_num = NULL;
    Jim_Obj *body = argv[argc - 1];
    --argc;
    if ((ret = Jim_GetDouble(interp, argv[1], &rep_time)) != JIM_OK)
        return ret;
    if (rep_time < 0 || rep_time > MAX_SLEEP_SEC) {
        Jim_SetResultFormatted(interp, "loop time out of range: %#s", argv[1]);
        return JIM_ERR;
    }
    if (argc > 2 && (ret = Jim_GetWideExpr(interp, argv[2], &rep_num)) != JIM_OK)
        return ret;
    if (argc > 3) {
        var_time = argv[3];
        if (Jim_Length(var_time) == 0)
            var_time = NULL;
    }
    if (argc > 4) {
        var_num = argv[4];
        if (Jim_Length(var_num) == 0)
            var_num = NULL;
    }

    struct timeval start_ts, end_ts;
    if ((ret = get_time(interp, &start_ts)) != JIM_OK)
        return ret;
    timerclear(&end_ts);
    if (rep_time != 0) {
        end_ts.tv_sec = (time_t)rep_time;
        end_ts.tv_usec = (suseconds_t)(USEC_PER_SEC * (rep_time - end_ts.tv_sec));
        timeradd(&start_ts, &end_ts, &end_ts);
    }
    for (jim_wide rep = 0; rep_num < 0 || rep < rep_num; rep++) {
        double iter_time = 0;
        if (rep > 0) {
            struct timeval curr_ts;
            if ((ret = get_time(interp, &curr_ts)) != JIM_OK)
                return ret;
            if (timerisset(&end_ts) && !timercmp(&curr_ts, &end_ts, <))
                break;
            timersub(&curr_ts, &start_ts, &curr_ts);
            iter_time = curr_ts.tv_sec + ((double)curr_ts.tv_usec)/USEC_PER_SEC;
        }
        if (var_time != NULL) {
            Jim_Obj *expr_time = Jim_NewDoubleObj(interp, iter_time);
            if ((ret = Jim_SetVariable(interp, var_time, expr_time)) != JIM_OK)
                return ret;
        }
        if (var_num != NULL) {
            Jim_Obj *expr_num = Jim_NewIntObj(interp, rep);
            if ((ret = Jim_SetVariable(interp, var_num, expr_num)) != JIM_OK)
                return ret;
        }
        ret = Jim_EvalObj(interp, body);
        if (ret != JIM_OK && ret != JIM_CONTINUE)
            break;
        ret = JIM_OK;
    }
    if (var_time != NULL)
        Jim_UnsetVariable(interp, var_time, 0);
    if (var_num != NULL)
        Jim_UnsetVariable(interp, var_num, 0);
    return ret;
}

/**
 * Tcl command: names.
 */
static int exec_names(Jim_Interp *interp, int argc, Jim_Obj *const*argv) {
    static const char *const commands[] = { "axis", "key", NULL };
    if (argc != 2) {
        Jim_WrongNumArgs(interp, argc, argv, "subcommand ?args ...?");
        return JIM_ERR;
    }
    int cmd = 0;
    if (Jim_GetEnum(interp, argv[1], commands, &cmd, "subcommand", JIM_ERRMSG|JIM_ENUM_ABBREV) != JIM_OK)
        return Jim_CheckShowCommands(interp, argv[1], commands);
    Jim_Obj *result = NULL;
    switch (cmd) {
    case 0: // Axis names
        result = Jim_NewListObj(interp, NULL, 0);
        for (const struct udotool_obj_id *axis = UINPUT_REL_AXES; axis->name != NULL; axis++) {
            Jim_Obj *elem = Jim_NewListObj(interp, NULL, 0);
            Jim_ListAppendElement(interp, elem, Jim_NewStringObj(interp, axis->name, -1));
            Jim_ListAppendElement(interp, elem, Jim_NewIntObj(interp, axis->value));
            Jim_ListAppendElement(interp, result, elem);
        }
        for (const struct udotool_obj_id *axis = UINPUT_ABS_AXES; axis->name != NULL; axis++) {
            Jim_Obj *elem = Jim_NewListObj(interp, NULL, 0);
            Jim_ListAppendElement(interp, elem, Jim_NewStringObj(interp, axis->name, -1));
            Jim_ListAppendElement(interp, elem, Jim_NewIntObj(interp, axis->value));
            Jim_ListAppendElement(interp, result, elem);
        }
        break;
    case 1: // Key/button names
        result = Jim_NewListObj(interp, NULL, 0);
        for (const struct udotool_obj_id *key = UINPUT_KEYS; key->name != NULL; key++) {
            Jim_Obj *elem = Jim_NewListObj(interp, NULL, 0);
            Jim_ListAppendElement(interp, elem, Jim_NewStringObj(interp, key->name, -1));
            Jim_ListAppendElement(interp, elem, Jim_NewIntObj(interp, key->value));
            Jim_ListAppendElement(interp, result, elem);
        }
        break;
    }
    Jim_SetResult(interp, result);
    return JIM_OK;
}

/**
 * Tcl command: sleep (override).
 */
static int exec_sleep(Jim_Interp *interp, int argc, Jim_Obj *const*argv) {
    if (argc != 2) {
        Jim_WrongNumArgs(interp, 1, argv, "seconds");
        return JIM_ERR;
    }
    double delay = 0;
    int ret;
    if ((ret = Jim_GetDouble(interp, argv[1], &delay)) != JIM_OK)
        return ret;
    struct timespec tval;
    memset(&tval, 0, sizeof(tval));
    tval.tv_sec = (time_t)delay;
    tval.tv_nsec = (long)((delay - tval.tv_sec)*NSEC_PER_SEC);
    while (nanosleep(&tval, &tval) != 0) {
        if (errno != EINTR) {
            Jim_SetResultFormatted(interp, "error when sleeping: %s", strerror(errno));
            return JIM_ERR;
        }
    }
    Jim_SetEmptyResult(interp);
    return JIM_OK;
}
