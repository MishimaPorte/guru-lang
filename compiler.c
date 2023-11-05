#include "compiler.h"
#include "bytecode.h"
#include "hashmap.h"
#include "scanner.h"
#include "value.h"
#include "vm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void __insert_op(struct compiler *comp, void *blob, size_t blob_size, enum guru_type t, enum opcode op, enum opcode op16) {
    uint16_t i = add_const(comp->compiled_chunk, blob, blob_size, t);
    if (i<UINT8_MAX) {
        chput(comp->compiled_chunk, op);
        chput(comp->compiled_chunk, (uint8_t) i);
        return;
    }
    if (i == UINT16_MAX) {
        comp_err_report(comp, &comp->then, "too many constants for a single chunk");
        return;
    }
    chput(comp->compiled_chunk, op16);
    chputn(comp->compiled_chunk, &i, sizeof(i));
}
static void comp_err_report(struct compiler *comp, struct token *t, const char *msg) {
    if (had_panic(comp)) return;
    fprintf(stderr, "ERROR: line: %d, pos: %d ", t->line, t->pos);

    if (t->type == GURU_EOF) fprintf(stderr, "at the end");
    else fprintf(stderr, "at '%.*s'", t->lexeme, t->len);

    fprintf(stderr, ": %s.", msg);
    comp->state |= 0x0001;
    comp->state |= 0x0002;
}

static void advance(struct compiler *comp) {
    memcpy(&comp->then, &comp->now, sizeof(struct token));

    for (;;) {
        scgett(comp->scanner, &comp->now);
        if (comp->now.line != comp->line) chputl(comp->compiled_chunk), comp->line = comp->now.line;
        if (comp->now.type != GURU_ERROR) break;

        comp_err_report(comp, &comp->now, "bad token");
    }
}

static void fin_chunk(struct chunk *c) { }

enum exec_code compile(const char *source, struct chunk *c) {

    struct compiler comp;
    comp.scanner = scalloc(source);
    comp.state = 0x0000;
    comp.line = 1;
    comp.compiled_chunk = c;


    advance(&comp);
    while (comp.now.type != GURU_EOF) {
        __comp_statement(&comp);
        if (comp.state & PANIC_MODE) {
            comp.state &= (~PANIC_MODE);
            comp.state |= ROTTEN;
            while (comp.now.type != GURU_EOF) {
                if (comp.then.type == GURU_SEMICOLON) goto synched;
                switch (comp.now.type) {
                    case GURU_VAR: case GURU_P_OUT:
                    case GURU_P_ERR: case GURU_RETURN:
                        goto synched;
                    default:;
                };
synched:
                advance(&comp);
                break;
            }
        }
    }
    advance(&comp);
    fin_chunk(c);
    return had_err(&comp) ? RESULT_OK : RESULT_COMPERR;
}

static void __comp_statement(struct compiler *comp) {
    if (comp->now.type == GURU_VAR) {
        advance(comp);
        if (comp->now.type != GURU_IDENTIFIER) {
            comp_err_report(comp, &comp->now, "expected identifier");
            return;
        };
        advance(comp);

        struct __blob_header *blob = get_int_str(comp->then.lexeme, comp->then.len);
        if (comp->now.type == GURU_EQUAL) {
            advance(comp);
            // advance(comp);
            __comp_expression(comp);
        } else {
            chput(comp->compiled_chunk, OP_LOAD_NOTHING);
        }
        __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_16);
    } else if (comp->now.type == GURU_P_OUT) {
        advance(comp);
        __comp_expression(comp);
        chput(comp->compiled_chunk, OP_PRINT_STDOUT);
    } else if (comp->now.type == GURU_P_ERR) {
        advance(comp);
        __comp_expression(comp);
        chput(comp->compiled_chunk, OP_PRINT_STDERR);
    } else {
        __comp_expression(comp);
        chput(comp->compiled_chunk, OP_OP);
    }
    if (comp->now.type == GURU_SEMICOLON) {
        advance(comp);
    } else {
        comp_err_report(comp, &comp->now, "unterminated statement!");
    }
}

static void __comp_number(struct compiler *comp) {
    double num = strtod(comp->then.lexeme, NULL);
    __insert_op(comp, &num, sizeof(num), VAL_NUMBER, OP_CONST, OP_CONST_16);
}

static void __comp_string(struct compiler *comp) {
    struct __blob_header *blob = get_int_str(comp->then.lexeme + 1, comp->then.len - 2);
    __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_CONST, OP_CONST_16);
}

static void __comp_grouping(struct compiler *comp) {

    __comp_expression(comp);
    panic_if_not_this(comp, GURU_RIGHT_PAREN, "unmatched left parentheses");
};

static void __comp_unary(struct compiler *comp) {
    enum tokent t = comp->then.type;
    __comp_with_precedence(comp, __PREC_UNARY);

    switch (t) {
        case GURU_MINUS: return chput(comp->compiled_chunk, OP_FLOAT_NEGATE);
        case GURU_BANG: return chput(comp->compiled_chunk, OP_LOGNOT);
        default: return;
    }
}

static void __comp_expression(struct compiler *comp) {
    __comp_with_precedence(comp, __PREC_ASSIGNMENT);
};

static void __comp_identifier(struct compiler *comp) {
    struct __blob_header *blob = get_int_str(comp->then.lexeme, comp->then.len);
    if (comp->now.type == GURU_EQUAL) {
        advance(comp);
        __comp_expression(comp);
        __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_ASSIGN_GLOBAL, OP_ASSIGN_GLOBAL_16);
        __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_LOAD_GLOBAL, OP_LOAD_GLOBAL_16);
    } else {
        __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_LOAD_GLOBAL, OP_LOAD_GLOBAL_16);
    }
};

static void __comp_liter(struct compiler *comp) {
    switch (comp->then.type) {
    case GURU_TRUE: return chput(comp->compiled_chunk, OP_LOAD_TRUTH);
    case GURU_FALSE: return chput(comp->compiled_chunk, OP_LOAD_LIES);
    case GURU_NOTHING: return chput(comp->compiled_chunk, OP_LOAD_NOTHING);
    case GURU_VOID: return chput(comp->compiled_chunk, OP_LOAD_VOID);
    default: comp_err_report(comp, &comp->then, "invalid literal"); // unreachable
    }
};

static void __comp_binary(struct compiler *comp) {
    enum tokent tt = comp->then.type;

    struct pratt_rule *pr = __get_pratt_rule(tt);
    __comp_with_precedence(comp, pr->prec + 1);

    switch (tt) {
    case GURU_PLUS: chput(comp->compiled_chunk, OP_FLOAT_SUM_2); break;
    case GURU_MINUS: chput(comp->compiled_chunk, OP_FLOAT_SUB); break;
    case GURU_STAR: chput(comp->compiled_chunk, OP_FLOAT_MULT_2); break;
    case GURU_SLASH: chput(comp->compiled_chunk, OP_FLOAT_DIVIDE); break;

    case GURU_STRING_CONCAT: chput(comp->compiled_chunk, OP_STR_CONC); break;
    case GURU_STRING_EQ: chput(comp->compiled_chunk, OP_STRING_EQ); break;
                

    case GURU_EQUAL_EQUAL:{
        chput(comp->compiled_chunk, OP_EQ);
        break;
    }
    case GURU_BANG_EQUAL: chput(comp->compiled_chunk, OP_NEQ); break;
    case GURU_GREATER: chput(comp->compiled_chunk, OP_GT); break;
    case GURU_GREATER_EQUAL: chput(comp->compiled_chunk, OP_GTE); break;
    case GURU_LESS: chput(comp->compiled_chunk, OP_LT); break;
    case GURU_LESS_EQUAL: chput(comp->compiled_chunk, OP_LTE); break;
    default: return;
    }

};

static void __comp_with_precedence(struct compiler *comp, enum comp_precedence prec) {
    advance(comp);

    void (*pref_fn)(struct compiler*) = __get_pratt_rule(comp->then.type)->prefix_fn;
    if (pref_fn == NULL) {
        comp_err_report(comp, &comp->then, "an expression is expected here");
        return;
    }
    
    pref_fn(comp);

    while (prec <= __get_pratt_rule(comp->now.type)->prec) {
        advance(comp);
        __get_pratt_rule(comp->then.type)->infix_fn(comp);
    }
};

static struct pratt_rule pratt_table[] = {
    [GURU_LEFT_PAREN] = {&__comp_grouping, NULL, __PREC_NONE},
    [GURU_RIGHT_PAREN] = {NULL, NULL, __PREC_NONE},
    [GURU_LEFT_BRACE] = {NULL, NULL, __PREC_NONE},
    [GURU_RIGHT_BRACE] = {NULL, NULL, __PREC_NONE},
    [GURU_COMMA] = {NULL, NULL, __PREC_NONE},
    [GURU_DOT] = {NULL, NULL, __PREC_NONE},
    [GURU_MINUS] = {&__comp_unary, &__comp_binary, __PREC_TERM},
    [GURU_PLUS] = {NULL, &__comp_binary, __PREC_TERM},
    [GURU_SEMICOLON] = {NULL, NULL, __PREC_NONE},
    [GURU_SLASH] = {NULL, &__comp_binary, __PREC_FACTOR},
    [GURU_STAR] = {NULL, &__comp_binary, __PREC_FACTOR},
    [GURU_BANG] = {&__comp_unary, NULL, __PREC_NONE},

    [GURU_BANG_EQUAL] = {NULL, &__comp_binary, __PREC_EQUALITY},
    [GURU_EQUAL_EQUAL] = {NULL, &__comp_binary, __PREC_EQUALITY},
    [GURU_STRING_CONCAT] = {NULL, &__comp_binary, __PREC_TERM},
    [GURU_STRING_EQ] = {NULL, &__comp_binary, __PREC_EQUALITY},

    [GURU_GREATER] = {NULL, &__comp_binary,__PREC_COMPARISON},
    [GURU_GREATER_EQUAL] = {NULL, &__comp_binary, __PREC_COMPARISON},
    [GURU_LESS] = {NULL, &__comp_binary, __PREC_COMPARISON},
    [GURU_LESS_EQUAL] = {NULL, &__comp_binary, __PREC_COMPARISON},

    [GURU_EQUAL] = {NULL, NULL, __PREC_NONE},

    [GURU_IDENTIFIER] = {&__comp_identifier, NULL, __PREC_NONE},
    [GURU_NUMBER] = {&__comp_number, NULL, __PREC_NONE},
    [GURU_STRING] = {&__comp_string, NULL, __PREC_NONE},

    [GURU_AND] = {NULL, NULL, __PREC_NONE},
    [GURU_OR] = {NULL, NULL, __PREC_NONE},
    // [GURU_INTEGER] = {&__comp_integer, NULL, __PREC_NONE},
    //
    [GURU_TRUE] = {&__comp_liter, NULL, __PREC_NONE},
    [GURU_FALSE] = {&__comp_liter, NULL, __PREC_NONE},
    [GURU_VOID] = {&__comp_liter, NULL, __PREC_NONE},
    [GURU_NOTHING] = {&__comp_liter, NULL, __PREC_NONE},

    [GURU_FOR] = {NULL, NULL, __PREC_NONE},
    [GURU_FUN] = {NULL, NULL, __PREC_NONE},

    [GURU_IF] = {NULL, NULL, __PREC_NONE},
    [GURU_ELSE] = {NULL, NULL, __PREC_NONE},

    [GURU_P_ERR] = {NULL, NULL, __PREC_NONE},
    [GURU_P_OUT] = {NULL, NULL, __PREC_NONE},

    [GURU_RETURN] = {NULL, NULL, __PREC_NONE},

    [GURU_CLASS] = {NULL, NULL, __PREC_NONE},
    [GURU_SUPER] = {NULL, NULL, __PREC_NONE},
    [GURU_THIS] = {NULL, NULL, __PREC_NONE},
};

static struct pratt_rule *__get_pratt_rule(enum tokent tt) {
    return &pratt_table[tt];
};
