#ifndef GURU_COMPILER
#define GURU_COMPILER

#include "bytecode.h"
#include "guru.h"
#include "scanner.h"
#include "vm.h"
#include <stdint.h>
#include <stdlib.h>
struct compiler {
    struct scanner *scanner;
    struct chunk *compiled_chunk;
    struct token now;
    struct token then;
    uint32_t line;

    uint16_t state;
    /*
     *  0x0001 is set if compiler had any kinds of errors
     *  0x0002 is set if compiler is currently in panic mode
     *  0x0004 is set if the compiler is now parsing something inside which it can do assignment
     * */
};

static const uint16_t ROTTEN        = 0x0001;
static const uint16_t PANIC_MODE    = 0x0002;
static const uint16_t ASSIGN_PERMIT = 0x0004;

enum comp_precedence {
    __PREC_NONE,
    __PREC_ASSIGNMENT, // =
    __PREC_OR, // or
    __PREC_AND, // and
    __PREC_EQUALITY, // == !=
    __PREC_COMPARISON, // < > <= >=
    __PREC_TERM, // + -
    __PREC_FACTOR, // * /
    __PREC_UNARY, // ! -
    __PREC_CALL, // . ()
    __PREC_PRIMARY
};

static void __comp_expression(struct compiler *comp);
static void __comp_unary(struct compiler *comp);
static void __comp_string(struct compiler *comp);
static void __comp_binary(struct compiler *comp);
static void __comp_number(struct compiler *comp);
static void __comp_grouping(struct compiler *comp);
static void __comp_with_precedence(struct compiler *comp, enum comp_precedence prec);
static void __comp_statement(struct compiler *comp);

static void __insert_op(struct compiler *comp, void *blob, size_t blob_size, enum guru_type t, enum opcode op, enum opcode op16);
struct pratt_rule {
    void (*prefix_fn)(struct compiler *comp);
    void (*infix_fn)(struct compiler *comp);
    enum comp_precedence prec;
};

#define had_err(c) !((c)->state & 0x0001)
#define had_panic(c) !((c)->state & 0x0002)
#define panic_if_not_this(c, tt, msg) do { \
    if (c->now.type == tt) advance(c); else comp_err_report(c, &c->now, msg); } while (0)
enum exec_code compile(const char *source, struct chunk *c);
static void advance(struct compiler *comp);
static void comp_err_report(struct compiler *comp, struct token *t, const char *msg);

#define comp_token(comp, tt) __comp_##tt(comp)

// functions for each token type
static struct pratt_rule *__get_pratt_rule(enum tokent tt);

#define __enter_panic_mode(comp) ((comp)->state |= PANIC_MODE)

#endif
