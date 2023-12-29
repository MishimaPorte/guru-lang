#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "bytecode.h"
#include "debug.h"

static void __d_simple_instruction(const struct chunk *c, const char *name, const uint8_t *ip) {
    printf("[%05ld] %-26s\n", ip - c->code - 1, name);
}

static void __d_one_u8_instruction(const struct chunk *c, const char *name, const uint8_t *ip) {
    printf("[%05ld] %-26s %08i\n", ip - c->code - 1, name, *(ip));
}

static void __d_one_u16_instruction(const struct chunk *c, const char *name, const uint8_t *ip) {
    printf("[%05ld] %-26s %08u\n", ip - c->code - 1, name, *(uint16_t*)(ip));
}

static void __d_one_u32_instruction(const struct chunk *c, const char *name, const uint8_t *ip) {
    printf("[%05ld] %-26s %08u\n", ip - c->code - 1, name, *(uint32_t*)(ip));
}

uint8_t *disassemble_chunk(const struct chunk *c) {
    uint8_t *ip = c->code;
    while (ip != c->code + c->opcount) {
        switch (*ip++) {
        case OP_EXIT: {
            __d_simple_instruction(c, "EXIT", ip);
            break;
        }
        case OP_FOR_LOOP: {
            __d_simple_instruction(c, "FOR_LOOP", ip);
            break;
        }
        case OP_FOR_LOOP_END: {
            __d_simple_instruction(c, "FOR_LOOP_END", ip);
            break;
        }
        case OP_FLOAT_NEGATE: {
            __d_simple_instruction(c, "FLOAT_NEGATE", ip);
            break;
        }
        case OP_SET: {
            printf("[%05ld] %-26s [%s]\n", ip - c->code - 1, "SET", c->consts.vals[*ip].as.blob->__cont);
            ip++;
            break;
        }
        case OP_SET_16: {
            __d_one_u16_instruction(c, "SET_16", ip);
            ip++;
            ip++;
            break;
        }
        case OP_GET: {
            printf("[%05ld] %-26s", ip - c->code - 1, "GET");
            printf(" [%s]\n", c->consts.vals[*ip++].as.blob->__cont);
            break;
        }
        case OP_GET_16: {
            printf("[%05ld] %-26s", ip - c->code - 1, "GET_16");
            printf(" [%s]\n", c->consts.vals[*ip++].as.blob->__cont);
            ip++;
            break;
        }
        case OP_DECLARE_FUNCTION: {
            ip+=1;
            printf("[%05ld] %-26s %08i\n", ip - c->code - 1, "DECLARE_FUNCTION", *(ip));
            uint8_t closure_count = *ip++;
            for (uint8_t i = closure_count; i > 0; i--) {
                printf("\tCLOSURE: [%d]\n", *ip++);
            };
            break;
        }
        case OP_DECLARE_CLASS: {
            ip+=1;
            printf("[%05ld] %-26s %08i\n", ip - c->code - 1, "DECLARE_CLASS", *(ip));
            uint8_t meth_count = *ip++;
            for (uint8_t i = meth_count; i > 0; i--) {
                uint16_t cons = *(uint16_t*)ip;
                ip+=2;
                printf("\tMETHOD: [%s]\n", c->consts.vals[cons].as.blob->__cont);
            };
            break;
        }
        case OP_DECLARE_CLASS_16: {
            ip+=2;
            printf("[%05ld] %-26s %08i\n", ip - c->code - 1, "DECLARE_CLASS", *(ip));
            uint8_t meth_count = *ip++;
            for (uint8_t i = meth_count; i > 0; i--) {
                uint16_t cons = *(uint16_t*)ip;
                ip+=2;
                printf("\tMETHOD: [%s]", c->consts.vals[cons].as.blob->__cont);
            };
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
        case OP_OP_NOFREE: {
            __d_simple_instruction(c, "OP_NOFREE", ip);
            break;
        }
        case OP_OP: {
            __d_simple_instruction(c, "OP", ip);
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
        case OP_COLLECT_LOCALS: {
            __d_one_u16_instruction(c, "COLLECT_LOCALS", ip);
            ip +=2;
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
        case OP_LOAD_CLOSURE: {
            __d_one_u8_instruction(c, "OP_LOAD_CLOSURE", ip);
            ip++;
            break;
        }
        case OP_PUT_CLOSURE: {
            __d_one_u8_instruction(c, "OP_PUT_CLOSURE", ip); ip++;
            break;
        }
        case OP_LOAD_LINK: {
            __d_one_u8_instruction(c, "LOAD_LINK", ip);
            ip++;
            break;
        }
        case OP_PUT_LINK: {
            __d_one_u8_instruction(c, "PUT_LINK", ip); ip++;
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
        case OP_CALL: {
            printf("[%05d] %-26s %08d %08d\n", ip - c->code - 1, "CALL", *(ip), *(ip+1));
            ip +=2;
            break;
        }
        case OP_LOAD_THIS: {
            __d_simple_instruction(c, "LOAD_THIS", ip);
            break;
        }
        case OP_RETURN: {
            __d_simple_instruction(c, "RETVRN", ip);
            break;
        }
        default: {
            char digit[12];
            sprintf(digit, "UNKNOWN %03d", *(ip-1));
            __d_simple_instruction(c, digit, ip);
            break;
        }}
    }
    return ip;
}
