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

/**
 * Operation opcodes.
 */
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

/**
 * Operation descriptions.
 */
static struct op_desc {
    const char *name;        ///< Operation name.
    int         op;          ///< Opcode.
    int         precedence;  ///< Precedence (zero is the highest).
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

/**
 * Evaluation context.
 */
struct eval_context {
    const struct udotool_verb_info
          *info;                       ///< Execution context.
    size_t op_depth;                   ///< Operation stack depth.
    int    op_stack[MAX_EVAL_DEPTH];   ///< Operation stack.
    size_t val_depth;                  ///< Value stack depth.
    double val_stack[MAX_EVAL_DEPTH];  ///< Value stack.
};

/**
 * Parse a double-precision floating-point value.
 *
 * @param info  verb for which evaluation is performed.
 * @param text  text to parse.
 * @param pval  pointer to buffer for parsed value.
 * @return      zero on success, or `-1` on error.
 */
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

/**
 * Parse an integer value.
 *
 * @param info  verb for which evaluation is performed.
 * @param text  text to parse.
 * @param pval  pointer to buffer for parsed value.
 * @return      zero on success, or `-1` on error.
 */
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

/**
 * Convert token to opcode.
 *
 * @param token  token to convert.
 * @return       opcode, or `OP_VALUE` if not an operation.
 */
static int parse_opcode(const char *token) {
    for (struct op_desc *oper = OPERATIONS; oper->name != NULL; oper++)
        if (strcmp(token, oper->name) == 0)
            return oper->op;
    return OP_VALUE;
}

/**
 * Get operation name.
 *
 * @param op  opcode.
 * @return    operation name.
 */
static const char *parse_opname(int op) {
    for (struct op_desc *oper = OPERATIONS; oper->name != NULL; oper++)
        if (oper->op == op)
            return oper->name;
    return "???";
}

/**
 * Get operation precedence.
 *
 * @param op  opcode.
 * @return    operation precedence.
 */
static int parse_opprec(int op) {
    for (struct op_desc *oper = OPERATIONS; oper->name != NULL; oper++)
        if (oper->op == op)
            return oper->precedence;
    return -1;
}

/**
 * Push an operation to operation stack.
 *
 * @param ctxt  evaluation context.
 * @param op    opcode to push.
 * @return      zero on success, or `-1` on error.
 */
static int parse_op_push(struct eval_context *ctxt, int op) {
    if (ctxt->op_depth >= (MAX_EVAL_DEPTH - 1)) {
        log_message(-1, "%s: condition complexity exceeded", ctxt->info->verb);
        return -1;
    }
    ctxt->op_stack[ctxt->op_depth++] = op;
    return 0;
}

/**
 * Peek an operation on the top of operation stack.
 *
 * @param ctxt  evaluation context.
 * @return      opcode on success, or `OP_NONE` if stack is empty.
 */
static int parse_op_peek(struct eval_context *ctxt) {
    return ctxt->op_depth == 0 ? OP_NONE : ctxt->op_stack[ctxt->op_depth - 1];
}

/**
 * Pop an operation from the top of operation stack.
 *
 * @param ctxt  evaluation context.
 * @return      opcode on success, or `OP_NONE` if stack is empty.
 */
static int parse_op_pop(struct eval_context *ctxt) {
    if (ctxt->op_depth == 0)
        return OP_NONE;
    return ctxt->op_stack[--(ctxt->op_depth)];
}

/**
 * Push a value to value stack.
 *
 * @param ctxt   evaluation context.
 * @param value  value to push.
 * @return       zero on success, or `-1` on error.
 */
static int parse_val_push(struct eval_context *ctxt, double value) {
    if (ctxt->val_depth >= (MAX_EVAL_DEPTH - 1)) {
        log_message(-1, "%s: condition complexity exceeded", ctxt->info->verb);
        return -1;
    }
    ctxt->val_stack[ctxt->val_depth++] = value;
    return 0;
}

/**
 * Pop a value from the top of value stack.
 *
 * @param ctxt    evaluation context.
 * @param pvalue  buffer for popped value.
 * @return        zero on success, or `-1` on error.
 */
static int parse_val_pop(struct eval_context *ctxt, double *pvalue) {
    if (ctxt->val_depth == 0)
        return -1;
    *pvalue = ctxt->val_stack[--(ctxt->val_depth)];
    return 0;
}

/**
 * Apply an operation from the top of operation stack.
 *
 * @param ctxt  evaluation context.
 * @return      zero on success, or `-1` on error.
 */
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

/**
 * Compare two operation precedences.
 *
 * @param op1  first opcode.
 * @param op2  second opcode.
 * @return     non-zero if the second operation is of same of higher
 *             precedence than the first operation.
 */
static int parse_op_higher(int op1, int op2) {
    int prec1 = parse_opprec(op1);
    int prec2 = parse_opprec(op2);
    return prec2 <= prec1;
}

/**
 * Parse and evaluate a condition expression.
 *
 * @param info  verb for which evaluation is performed.
 * @param argc  number of arguments.
 * @param argv  array of arguments.
 * @param pval  pointer to buffer for evaluated value.
 * @return      zero on success, or `-1` on error.
 */
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
//
// This is a test for condition evaluation
//
// To build: gcc -DRUN_EVAL_TEST run-eval.c -o run-eval
// To run:   ./run-eval <token>...
//
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
