
#include <stddef.h>
#include <stdio.h>
#include "debug.h"

static void __d_simple_instruction(const struct chunk *c, const char *name, const uint8_t *ip) {
    printf("[%05d] %-26s\n", ip - c->code - 1, name);
}

static void __d_one_u8_instruction(const struct chunk *c, const char *name, const uint8_t *ip) {
    printf("[%05d] %-26s %08d\n", ip - c->code - 1, name, *(ip));
}

static void __d_one_u16_instruction(const struct chunk *c, const char *name, const uint8_t *ip) {
    printf("[%05d] %-26s %08d\n", ip - c->code - 1, name, *(uint16_t*)(ip));
}

static void __d_one_u32_instruction(const struct chunk *c, const char *name, const uint8_t *ip) {
    printf("[%05d] %-26s %08d\n", ip - c->code - 1, name, *(uint32_t*)(ip));
}

void disassemble_chunk(const struct chunk *c) {
    uint8_t *ip = c->code;
    while (ip != c->code + c->opcount) {
        switch (*ip++) {
        case OP_EXIT: {
            __d_simple_instruction(c, "EXIT", ip);
            break;
        }
        case OP_PRINT_STDOUT: {
            __d_simple_instruction(c, "PRINT_STDOUT", ip);
            break;
        }
        case OP_PRINT_STDERR: {
            __d_simple_instruction(c, "PRINT_STDERR", ip);
            break;
        }
        case OP_FLOAT_NEGATE: {
            __d_simple_instruction(c, "FLOAT_NEGATE", ip);
            break;
        }
        case OP_CONST: {
            __d_one_u8_instruction(c, "CONST", ip);
            ip++;
            break;
        }
        case OP_CONST_16: {
            __d_one_u16_instruction(c, "CONST_16", ip);
            ip++;
            ip++;
            break;
        }
        case OP_FLOAT_UN_PLUS: {
            __d_simple_instruction(c, "FLOAT_UN_PLUS", ip);
            break;
        }
        case OP_RETURN_EXPRESSION: {
            __d_simple_instruction(c, "RETURN_EXPRESSION", ip);
            break;
        }
        case OP_GROUPING_EXPR: {
            __d_simple_instruction(c, "GROUPING_EXPR", ip);
            break;
        }
        case OP_FLOAT_SUM_2: {
            __d_simple_instruction(c, "FLOAT_SUM_2", ip);
            break;
        }
        case OP_FLOAT_SUB: {
            __d_simple_instruction(c, "FLOAT_SUB", ip);
            break;
        }
        case OP_FLOAT_MULT_2: {
            __d_simple_instruction(c, "FLOAT_MULT_2", ip);
            break;
        }
        case OP_FLOAT_DIVIDE: {
            __d_simple_instruction(c, "FLOAT_DIVIDE", ip);
            break;
        }
        case OP_BYTES_SUM_2: {
            __d_simple_instruction(c, "BYTES_SUM_2", ip);
            break;
        }
        case OP_BYTES_SUB: {
            __d_simple_instruction(c, "BYTES_SUB", ip);
            break;
        }
        case OP_BYTES_MULT_2: {
            __d_simple_instruction(c, "BYTES_MULT_2", ip);
            break;
        }
        case OP_BYTES_DIVIDE: {
            __d_simple_instruction(c, "BYTES_DIVIDE", ip);
            break;
        }
        case OP_LOAD_TRUTH: {
            __d_simple_instruction(c, "LOAD_TRUTH", ip);
            break;
        }
        case OP_LOAD_LIES: {
            __d_simple_instruction(c, "LOAD_LIES", ip);
            break;
        }
        case OP_LOGNOT: {
            __d_simple_instruction(c, "LOGNOT", ip);
            break;
        }
        case OP_LOAD_VOID: {
            __d_simple_instruction(c, "LOAD_VOID", ip);
            break;
        }
        case OP_LOAD_NOTHING: {
            __d_simple_instruction(c, "LOAD_NOTHING", ip);
            break;
        }
        case OP_EQ: {
            __d_simple_instruction(c, "EQ", ip);
            break;
        }
        case OP_NEQ: {
            __d_simple_instruction(c, "NEQ", ip);
            break;
        }
        case OP_GT: {
            __d_simple_instruction(c, "GT", ip);
            break;
        }
        case OP_GTE: {
            __d_simple_instruction(c, "GTE", ip);
            break;
        }
        case OP_LT: {
            __d_simple_instruction(c, "LT", ip);
            break;
        }
        case OP_LTE: {
            __d_simple_instruction(c, "LTE", ip);
            break;
        }
        case OP_STR_CONC: {
            __d_simple_instruction(c, "STR_CONC", ip);
            break;
        }
        case OP_STRING_EQ: {
            __d_simple_instruction(c, "STRING_EQ", ip);
            break;
        }
        case OP_OP: {
            __d_simple_instruction(c, "OP", ip);
            break;
        }
        case OP_POP_MANY: {
            __d_simple_instruction(c, "POP_MANY", ip);
            break;
        }
        case OP_DEFINE_GLOBAL: {
            size_t i = ip - c->code - 1;
            struct __blob_header *blob = c->consts.vals[*ip++].as.blob;
            printf("[%05d] %-26s [%.*s]\n", i, "DEFINE_GLOBAL", blob->len, blob->__cont);
            break;
        }
        case OP_DEFINE_GLOBAL_16: {
            size_t i = ip - c->code - 1;
            struct __blob_header *blob = c->consts.vals[*(uint16_t*)ip++].as.blob;
            ip++;
            printf("[%05d] %-26s [%.*s]\n", i, "DEFINE_GLOBAL_16", blob->len, blob->__cont);
            break;
        }
        case OP_LOAD_GLOBAL: {
            size_t i = ip - c->code - 1;
            struct __blob_header *blob = c->consts.vals[*ip++].as.blob;
            printf("[%05d] %-26s [%.*s]\n", i, "LOAD_GLOBAL", blob->len, blob->__cont);
            break;
        }
        case OP_LOAD_GLOBAL_16: {
            size_t i = ip - c->code - 1;
            struct __blob_header *blob = c->consts.vals[*(uint16_t*)ip++].as.blob;
            ip++;
            printf("[%05d] %-26s [%.*s]\n", i, "LOAD_GLOBAL_16", blob->len, blob->__cont);
            break;
        }
        case OP_ASSIGN_GLOBAL: {
            size_t i = ip - c->code - 1;
            struct __blob_header *blob = c->consts.vals[*ip++].as.blob;
            printf("[%05d] %-26s [%.*s]\n", i, "ASSIGN_GLOBAL", blob->len, blob->__cont);
            break;
        }
        case OP_ASSIGN_GLOBAL_16: {
            size_t i = ip - c->code - 1;
            struct __blob_header *blob = c->consts.vals[*(uint16_t*)ip++].as.blob;
            ip++;
            printf("[%05d] %-26s [%.*s]\n", i, "ASSIGN_GLOBAL_16", blob->len, blob->__cont);
            break;
        }
        case OP_CLEAN_AFTER_BLOCK: {
            __d_simple_instruction(c, "CLEAN_AFTER_BLOCK", ip);
            break;
        }
        case OP_JUMP_IF_FALSE: {
            __d_one_u32_instruction(c, "JUMP_IF_FALSE", ip);
            ip +=4;
            break;
        }
        case OP_JUMP_IF_TRUE: {
            __d_one_u32_instruction(c, "JUMP_IF_TRUE", ip);
            ip +=4;
            break;
        }
        case OP_JUMP: {
            __d_one_u32_instruction(c, "JUMP", ip);
            ip +=4;
            break;
        }
        case OP_JUMP_BACK: {
            __d_one_u32_instruction(c, "JUMP_BACK", ip);
            ip +=4;
            break;
        }
        case OP_JUMP_IF_FALSE_ONSTACK: {
            __d_one_u32_instruction(c, "JUMP_IF_FALSE_ONSTACK", ip);
            ip +=4;
            break;
        }
        case OP_JUMP_IF_TRUE_ONSTACK: {
            __d_one_u32_instruction(c, "JUMP_IF_TRUE_ONSTACK", ip);
            ip +=4;
            break;
        }
        case OP_LOAD_8: {
            __d_one_u8_instruction(c, "LOAD_8", ip);
            ip++;
            break;
        }
        case OP_PUT_8: {
            __d_one_u8_instruction(c, "PUT_8", ip); ip++;
            break;
        }
        case OP_PUT_8_WITH_POP: {
            __d_one_u8_instruction(c, "PUT_8_WITH_POP", ip); ip++;
            break;
        }
        case OP_PUT_8_NOTHING: {
            __d_one_u8_instruction(c, "PUT_8_NOTHING", ip);ip++;
            break;
        }
        case OP_PUT_8_VOID: {
            __d_one_u8_instruction(c, "PUT_8_VOID", ip);ip++;
            break;
        }
        case OP_PRE_CALL: {
            __d_one_u8_instruction(c, "PRE_CALL", ip);ip++;
            break;
        }
        case OP_CALL: {
            printf("[%05d] %-26s %08d %08d\n", ip - c->code - 1, "CALL", *(ip), *(ip+1));
            ip +=2;
            break;
        }
        case OP_RETURN: {
            __d_simple_instruction(c, "RETURN", ip);
            break;
        }
        case OP_DECLARE_ANON_FUNCTION: {
            printf("[%05d] %-26s %08d ", ip - c->code - 1, "DECLARE_ANON_FUNCTION", *(uint32_t*)ip);
            ip+=4;
            printf("%08d\n", *ip++);
            break;
        }
        case OP_OFFLOAD_REGISTER: {
            __d_simple_instruction(c, "OFFLOAD_REGISTER", ip);
            break;
        }
        case OP_LOAD_REGISTER: {
            __d_simple_instruction(c, "LOAD_REGISTER", ip);
            break;
        }}
    }
}
