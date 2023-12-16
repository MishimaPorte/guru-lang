#ifndef GURU_BYTECODE
#define GURU_BYTECODE
#include "value.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum opcode {
    OP_EXIT, OP_FLOAT_NEGATE,
    OP_CONST, OP_CONST_16,
    OP_FLOAT_UN_PLUS, OP_RETURN_EXPRESSION,
    OP_GROUPING_EXPR,
    OP_FLOAT_SUM_2, OP_FLOAT_SUB, OP_FLOAT_MULT_2, OP_FLOAT_DIVIDE,
    OP_BYTES_SUM_2, OP_BYTES_SUB, OP_BYTES_MULT_2, OP_BYTES_DIVIDE,
    OP_LOAD_TRUTH, OP_LOAD_LIES, OP_LOGNOT,
    OP_LOAD_VOID, OP_LOAD_NOTHING,
    OP_EQ, OP_NEQ, OP_GT, OP_GTE, OP_LT, OP_LTE,

    OP_STR_CONC, OP_STRING_EQ,

    OP_OP, OP_POP_MANY, OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_16, OP_LOAD_GLOBAL, OP_LOAD_GLOBAL_16,
    OP_ASSIGN_GLOBAL, OP_ASSIGN_GLOBAL_16,

    OP_CLEAN_AFTER_BLOCK,

    //JUMPS
    OP_JUMP_IF_FALSE, OP_JUMP_IF_TRUE, OP_JUMP, OP_JUMP_BACK,
    OP_JUMP_IF_FALSE_ONSTACK, OP_JUMP_IF_TRUE_ONSTACK,
    //register-based opcodes
    //used to offload a local to the heap
    OP_LOAD_CLOSURE, OP_PUT_CLOSURE,

    OP_LOAD_LINK, OP_PUT_LINK,
    OP_LOAD_8, OP_PUT_8, OP_PUT_8_WITH_POP,
    OP_PUT_8_NOTHING,
    OP_PUT_8_VOID,

    OP_CLOSURES, OP_CALL, OP_RETURN, OP_DECLARE_FUNCTION, OP_DECLARE_ANON_FUNCTION,

    OP_OFFLOAD_REGISTER, OP_LOAD_REGISTER,
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
    
    struct __consts consts;
    struct lninfo *line, *crnt_ln;

};

#define add_const(c, val, s, tt) __add_const(&(c)->consts, (val), (s), (tt))

void chfree(struct chunk *c);
void chput(struct chunk *c, uint8_t opcode);
void chunputif(struct chunk *c, enum opcode o, enum opcode o2);
void chputn(struct chunk *c, void *val, size_t size);
void chputl(struct chunk *c);
struct chunk *challoc(void); 

#endif
