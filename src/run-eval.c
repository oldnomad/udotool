// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Command execution evaluation
 *
 * Copyright (c) 2024 Alec Kojaev
 */
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "udotool.h"
#include "runner.h"

enum {
    OP_NONE = -1,
    OP_VALUE = 0,
    OP_OPEN,
    OP_CLOSE,
    // Comparisons
    OP_EQ = 100,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,
    OP_COMPARISON_LOW = 100,
    OP_COMPARISON_HIGH = 199,
    // Logical
    OP_NOT = 200,
    OP_AND,
    OP_OR,
};

static struct op_desc {
    const char *name;
    int         op;
    int         precedence;
} OPERATIONS[] = {
    { "(",    OP_OPEN,  0 },
    { ")",    OP_CLOSE, 0 },
    { "-not", OP_NOT,   1 },
    { "-and", OP_AND,  10 },
    { "-or",  OP_OR,   11 },
    { "-eq",  OP_EQ,    6 },
    { "-ne",  OP_NE,    6 },
    { "-lt",  OP_LT,    5 },
    { "-gt",  OP_GT,    5 },
    { "-le",  OP_LE,    5 },
    { "-ge",  OP_GE,    5 },
    { NULL }
};

struct op_elem {
    int op;
    int result;
};

struct eval_context {
    const struct udotool_verb_info
          *info;
    size_t op_depth;
    int    op_stack[MAX_EVAL_DEPTH];
    size_t val_depth;
    double val_stack[MAX_EVAL_DEPTH];
};

int run_parse_double(const struct udotool_verb_info *info, const char *text, double *pval) {
    const char *ep = text;
    double value = strtod(text, (char **)&ep);
    if (ep == text || *ep != '\0') {
        log_message(-1, "%s: error parsing value '%s'", info->verb, text);
        return -1;
    }
    *pval = value;
    return 0;
}

int run_parse_integer(const struct udotool_verb_info *info, const char *text, int *pval) {
    const char *ep = text;
    long value = strtol(text, (char **)&ep, 0);
    if (ep == text || *ep != '\0' || value < INT_MIN || value > INT_MAX) {
        log_message(-1, "%s: error parsing value '%s'", info->verb, text);
        return -1;
    }
    *pval = (int)value;
    return 0;
}

static int parse_opcode(const char *token) {
    for (struct op_desc *oper = OPERATIONS; oper->name != NULL; oper++)
        if (strcmp(token, oper->name) == 0)
            return oper->op;
    return OP_VALUE;
}

static const char *parse_opname(int op) {
    for (struct op_desc *oper = OPERATIONS; oper->name != NULL; oper++)
        if (oper->op == op)
            return oper->name;
    return "???";
}

static int parse_opprec(int op) {
    for (struct op_desc *oper = OPERATIONS; oper->name != NULL; oper++)
        if (oper->op == op)
            return oper->precedence;
    return -1;
}

static int parse_op_push(struct eval_context *ctxt, int op) {
    if (ctxt->op_depth >= (MAX_EVAL_DEPTH - 1)) {
        log_message(-1, "%s: condition complexity exceeded", ctxt->info->verb);
        return -1;
    }
    ctxt->op_stack[ctxt->op_depth++] = op;
    return 0;
}

static int parse_op_peek(struct eval_context *ctxt) {
    return ctxt->op_depth == 0 ? OP_NONE : ctxt->op_stack[ctxt->op_depth - 1];
}

static int parse_op_pop(struct eval_context *ctxt) {
    if (ctxt->op_depth == 0)
        return OP_NONE;
    return ctxt->op_stack[--(ctxt->op_depth)];
}

static int parse_val_push(struct eval_context *ctxt, double value) {
    if (ctxt->val_depth >= (MAX_EVAL_DEPTH - 1)) {
        log_message(-1, "%s: condition complexity exceeded", ctxt->info->verb);
        return -1;
    }
    ctxt->val_stack[ctxt->val_depth++] = value;
    return 0;
}

static int parse_val_pop(struct eval_context *ctxt, double *pvalue) {
    if (ctxt->val_depth == 0)
        return -1;
    *pvalue = ctxt->val_stack[--(ctxt->val_depth)];
    return 0;
}

static int parse_expr_apply(struct eval_context *ctxt) {
    int op = parse_op_pop(ctxt);
    double arg1 = 0, arg2 = 0, result = 0;
    switch (op) {
    default:
        log_message(-1, "%s: internal error, unexpected opcode %d", ctxt->info->verb, op);
        return -1;
    case OP_NOT:
        if (parse_val_pop(ctxt, &arg1) != 0) {
ON_ERROR:
            log_message(-1, "%s: missing value in operator %s", ctxt->info->verb, parse_opname(op));
            return -1;
        }
        result = arg1 == 0;
        break;
    case OP_AND:
        if (parse_val_pop(ctxt, &arg2) != 0 ||
            parse_val_pop(ctxt, &arg1) != 0)
            goto ON_ERROR;
        result = arg1 != 0 && arg2 != 0;
        break;
    case OP_OR:
        if (parse_val_pop(ctxt, &arg2) != 0 ||
            parse_val_pop(ctxt, &arg1) != 0)
            goto ON_ERROR;
        result = arg1 != 0 || arg2 != 0;
        break;
    case OP_EQ:
        if (parse_val_pop(ctxt, &arg2) != 0 ||
            parse_val_pop(ctxt, &arg1) != 0)
            goto ON_ERROR;
        result = arg1 == arg2;
        break;
    case OP_NE:
        if (parse_val_pop(ctxt, &arg2) != 0 ||
            parse_val_pop(ctxt, &arg1) != 0)
            goto ON_ERROR;
        result = arg1 != arg2;
        break;
    case OP_LT:
        if (parse_val_pop(ctxt, &arg2) != 0 ||
            parse_val_pop(ctxt, &arg1) != 0)
            goto ON_ERROR;
        result = arg1 < arg2;
        break;
    case OP_GT:
        if (parse_val_pop(ctxt, &arg2) != 0 ||
            parse_val_pop(ctxt, &arg1) != 0)
            goto ON_ERROR;
        result = arg1 > arg2;
        break;
    case OP_LE:
        if (parse_val_pop(ctxt, &arg2) != 0 ||
            parse_val_pop(ctxt, &arg1) != 0)
            goto ON_ERROR;
        result = arg1 <= arg2;
        break;
    case OP_GE:
        if (parse_val_pop(ctxt, &arg2) != 0 ||
            parse_val_pop(ctxt, &arg1) != 0)
            goto ON_ERROR;
        result = arg1 >= arg2;
        break;
    }
    log_message(3, "%s: evaluate %s(%f,%f) = %f", ctxt->info->verb,
        parse_opname(op), arg1, arg2, result);
    return parse_val_push(ctxt, result);
}

static int parse_op_higher(int op, int op2) {
    // Check whether `op2` is same or higher precedence than `op`
    int prec = parse_opprec(op);
    int prec2 = parse_opprec(op2);
    return prec2 <= prec;
}

int run_parse_condition(const struct udotool_verb_info *info, int argc, const char *const* argv, int *pval) {
    struct eval_context ctxt = { .info = info, .op_depth = 0, .val_depth = 0 };

    for (int i = 0; i < argc; i++) {
        int op = parse_opcode(argv[i]), op2;
        double result = 0;
        switch (op) {
        case OP_VALUE:
            if (run_parse_double(info, argv[i], &result) != 0)
                return -1;
            if (parse_val_push(&ctxt, result) != 0)
                return -1;
            break;
        case OP_OPEN:
            parse_op_push(&ctxt, OP_OPEN);
            break;
        case OP_CLOSE:
            while (parse_op_peek(&ctxt) != OP_OPEN) {
                if (parse_expr_apply(&ctxt) != 0)
                    return -1;
            }
            if (parse_op_pop(&ctxt) != OP_OPEN) {
                log_message(-1, "%s: unmatched parentheses", info->verb);
                return -1;
            }
            break;
        default: // Operator
            while ((op2 = parse_op_peek(&ctxt)) != OP_NONE && op2 != OP_OPEN && parse_op_higher(op, op2)) {
                if (parse_expr_apply(&ctxt) != 0)
                    return -1;
            }
            if (parse_op_push(&ctxt, op) != 0)
                return -1;
            break;
        }
    }
    while (ctxt.op_depth > 0) {
        if (parse_expr_apply(&ctxt) != 0)
            return -1;
    }
    if (ctxt.val_depth != 1) {
        log_message(-1, "%s: invalid expression", info->verb);
        return -1;
    }
    *pval = ctxt.val_stack[0] != 0;
    return 0;
}

#ifdef RUN_EVAL_TEST
// This is a test for condition evaluation
#include <stdarg.h>
#include <stdio.h>

int main(int argc, const char *argv[]) {
    int value;
    int ret = run_parse_condition(&(struct udotool_verb_info){ .verb = "test" }, argc - 1, &argv[1], &value);
    printf("Return code = %d, value = %d\n", ret, value);
}

void log_message(int level, const char *fmt,...) {
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
#endif
