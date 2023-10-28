#ifndef GURU_BYTECODE
#define GURU_BYTECODE
#include "value.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum opcode {
    OP_EXIT, OP_PR_STHEAD, OP_FLOAT_NEGATE,
    OP_CONST, OP_CONST_16,
    OP_FLOAT_UN_PLUS, OP_RETURN_EXPRESSION,
    OP_GROUPING_EXPR,
    OP_FLOAT_SUM_2, OP_FLOAT_SUB, OP_FLOAT_MULT_2, OP_FLOAT_DIVIDE,
    OP_BYTES_SUM_2, OP_BYTES_SUB, OP_BYTES_MULT_2, OP_BYTES_DIVIDE,
    OP_LOAD_TRUTH, OP_LOAD_LIES, OP_LOGNOT,
    OP_LOAD_VOID, OP_LOAD_NOTHING,
    OP_EQ, OP_NEQ, OP_GT, OP_GTE, OP_LT, OP_LTE,

    OP_STR_CONC, OP_STRING_EQ
};
struct lninfo {
    uint32_t opstart;
    uint32_t opend;
    int line;
    struct lninfo *next;
};
struct chunk {
    uint32_t opcount;
    uint32_t capacity;
    uint8_t *code;
    
    struct gobj_a consts;
    struct lninfo *line, *crnt_ln;

};

#define add_const(c, val) gdaput(&c->consts, val)

void chfree(struct chunk *c);
void chput(struct chunk *c, uint8_t opcode);
void chputn(struct chunk *c, void *val, size_t size);
void chputl(struct chunk *c);
struct chunk *challoc(void); 

#endif
