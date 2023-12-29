#ifndef GURU_COMPILER
#define GURU_COMPILER

#include "bytecode.h"
#include "guru.h"
#include "scanner.h"
#include "vm.h"
#include <stdint.h>
#include <stdlib.h>

#define PRIVILEGED_STACK_SLOTS 4

struct local {
    const struct token name;
    uint16_t depth;
    uint8_t is_const;
    uint8_t ofsetted;
    uint8_t compiled_function;
    uint8_t real;
    uint8_t is_link; // a local is a link if it has been offloaded to the heap inside a closure
};

struct compiler {
    struct scanner *scanner;
    struct chunk *compiled_chunk;
    struct token now;
    struct token then;
    struct u32_array_list *jumps_needed; // TODO: do this tomorrow
    uint32_t line;

    /*
     *  0x0001 is set if compiler had any kinds of errors;
     *  0x0002 is set if compiler is currently in panic mode;
     *  0x0004 is set if the compiler is now compiling something inside which it can do assignment;
     *  0x0008 is set if the compiler is now compiling a constructor;
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

    struct {
        uint8_t closure_count;
        uint8_t (*closures)[255];
    } closures;
};

static const uint16_t ROTTEN        = 0x0001;
static const uint16_t PANIC_MODE    = 0x0002;
static const uint16_t ASSIGN_PERMIT = 0x0004;
static const uint16_t F_CONSTRUCTOR = 0x0008;

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
    __PREC_CALL, // . () []
    __PREC_PRIMARY
};
static void __synchronize(struct compiler *comp);
static void __comp_with_precedence(struct compiler *comp, enum comp_precedence prec);

static void __comp_class(struct compiler *comp);
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
static void __comp_block_expression(struct compiler *comp);
static void __comp_identifier(struct compiler *comp);
static void __comp_liter(struct compiler *comp);
static void __comp_class(struct compiler *comp);
static void __comp_getset(struct compiler *comp);
static void __comp_index_sq(struct compiler *comp);

struct pratt_rule {
    void (*prefix_fn)(struct compiler *comp);
    void (*infix_fn)(struct compiler *comp);
    enum comp_precedence prec;
};


enum exec_code compile(const char *source, struct chunk *c);

static struct pratt_rule *__get_pratt_rule(enum tokent tt);
#endif
