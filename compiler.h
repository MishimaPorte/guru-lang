#ifndef GURU_COMPILER
#define GURU_COMPILER

#include "bytecode.h"
#include "guru.h"
#include "scanner.h"
#include "vm.h"
#include <stdint.h>
#include <stdlib.h>

#define PRIVILEGED_STACK_SLOTS 3

struct local {
    const struct token name;
    uint16_t depth;
    uint8_t ofsetted;
};

struct compiler {
    struct scanner *scanner;
    struct chunk *compiled_chunk;
    struct token now;
    struct token then;
    uint32_t line;

    /*
     *  0x0001 is set if compiler had any kinds of errors;
     *  0x0002 is set if compiler is currently in panic mode;
     *  0x0004 is set if the compiler is now compiling something inside which it can do assignment;
     * */
    uint16_t state;

    struct local locals[UINT16_MAX + 1];
    uint8_t local_count;
    uint16_t scope_depth;

    /*  increments each time we enter a block expression and decremetns on exit.
     * */
    uint16_t block_stack;

    struct {
        uint8_t s[255];
        uint8_t head;
    } stack;
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
static void __synchronize(struct compiler *comp);
static void __comp_with_precedence(struct compiler *comp, enum comp_precedence prec);

static void __comp_call(struct compiler *comp);
static void __comp_fun_expression(struct compiler *comp);
static void __comp_if_expression(struct compiler *comp);
static void __comp_expression(struct compiler *comp);
static void __comp_unary(struct compiler *comp);
static void __comp_string(struct compiler *comp);
static void __comp_binary(struct compiler *comp);
static void __comp_number(struct compiler *comp);
static void __comp_grouping(struct compiler *comp);
static void __comp_statement(struct compiler *comp);
static void __comp_and(struct compiler *comp);
static void __comp_or(struct compiler *comp);

struct pratt_rule {
    void (*prefix_fn)(struct compiler *comp);
    void (*infix_fn)(struct compiler *comp);
    enum comp_precedence prec;
};

enum exec_code compile(const char *source, struct chunk *c);

static struct pratt_rule *__get_pratt_rule(enum tokent tt);
#endif
