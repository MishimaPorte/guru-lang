#include "vm.h"
#include "bytecode.h"
#include "compiler.h"
#include "debug.h"
#include "ffi.h"
#include "guru.h"
#include "value.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void __runtime_error(struct guru_vm *vm, uint8_t *ip, const char *format, ...) {
    vm->state |= 0x0001;

    size_t code_n = ip - vm->code->code - 1;
    uint16_t line = 0;

    __find_lninfo_for_op(vm, code_n, line);
    fprintf(stderr, "[line %d] runtime error\n", line+1);

    va_list args;
    va_start(args, format);
    fprintf(stderr, "|---> "); vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
};


struct guru_vm *vm_init() {
    struct guru_vm *v = calloc(1, sizeof(struct guru_vm));
    if (v == NULL) error("not enough memory");
    v->stack.stack = calloc(1024, sizeof(struct __guru_object*));
    v->stack.head = v->stack.stack;
    v->state = 0;
    v->current_frame_offset = 0;

    init_pit();

    return v;
};
void vm_free(struct guru_vm *v) {
    free(v);
};
enum exec_code run(struct guru_vm *v, struct chunk *c) {
    if (c->code == NULL) return RESULT_RERR;
    v->code = c;

    register uint8_t *ip = c->code;
    def_nats(v);
    disassemble_chunk(c);

    for (;;) {
        if (ip > c->code + c->opcount - 1) break;
        else if (v->state & 0x0001) goto end_error;
        switch (*ip++) {
        case OP_EXIT: exit(0);
        case OP_FLOAT_UN_PLUS: break;
        case OP_FLOAT_NEGATE: {
           if (__get_stack_val(v, 0).tag != VAL_NUMBER) __runtime_error(v, ip-1, "expecting number operand");
           vm_st_head(v)->as.numeric = -vm_st_head(v)->as.numeric;
           break;
        }
        case OP_LOGNOT: {
           if (__get_stack_val(v, 0).tag != VAL_BOOL) __runtime_error(v, ip-1, "expecting boolean operand");
           if (vm_st_head(v)->as.bool) vm_st_head(v)->as.bool = 0;
           else vm_st_head(v)->as.bool = 1;
           break;
        }
        case OP_STRING_EQ: {
           if (__get_stack_val(v, 0).tag != BLOB_STRING) __runtime_error(v, ip-2, "expecting string operand");
           if (__get_stack_val(v, 1).tag != BLOB_STRING) __runtime_error(v, ip-3, "expecting string operand");
           struct __guru_object *l = vm_st_pop(v);
           struct __guru_object *r = vm_st_pop(v);
           struct __guru_object ret = (l->as.blob == r->as.blob || l->as.blob->len == r->as.blob->len && !memcmp(&l->as.blob->__cont[0], &r->as.blob->__cont[0], l->as.blob->len) ? __GURU_TRUE : __GURU_FALSE);
           l->as.blob->__rc--;
           r->as.blob->__rc--;
           *vm_st_push(v) = ret;
           break;
        };
        case OP_STR_CONC: {
           if (__get_stack_val(v, 0).tag != BLOB_STRING) __runtime_error(v, ip-2, "expecting string operand");
           if (__get_stack_val(v, 1).tag != BLOB_STRING) __runtime_error(v, ip-3, "expecting string operand");
           struct __guru_object *val = vm_st_pop(v);
           struct __guru_object *head = vm_st_pop(v);

           struct __blob_header *blob = __alloc_blob(val->as.blob->len + head->as.blob->len - 1);
           memcpy(blob->__cont, (void *) head->as.blob->__cont, head->as.blob->len - 1);
           memcpy(blob->__cont + head->as.blob->len - 1, (void *) val->as.blob->__cont, val->as.blob->len);

           head->as.blob->__rc--;
           val->as.blob->__rc--;

           struct __guru_object *h = vm_st_push(v);
           h->tag = BLOB_STRING;
           h->as.blob = blob;
           break;
        }
        case OP_LOAD_VOID:
           *vm_st_push(v) = __GURU_VOID;
            break;
        case OP_LOAD_NOTHING:
           *vm_st_push(v) = __GURU_NOTHING;
            break;
        case OP_LOAD_TRUTH: {
           uint64_t i = 1;
           *vm_st_push(v) = __GURU_TRUE;
           break;
        }
        case OP_LOAD_LIES:{
           uint64_t i = 0;
           *vm_st_push(v) = __GURU_FALSE;
           break;
        }
        case OP_NEQ: {
           struct __guru_object *l = vm_st_pop(v);
           struct __guru_object *r = vm_st_pop(v);
           *vm_st_push(v) = __val_neq(l, r);
           break;
        };
        case OP_EQ: {
           struct __guru_object *l = vm_st_pop(v);
           struct __guru_object *r = vm_st_pop(v);
           *vm_st_push(v) = __val_eq(l, r);
           break;
        };
        case OP_GTE: {
           comp_binary_op(v, ip, >=, VAL_NUMBER, numeric);
           break;
        };
        case OP_LTE: {
           comp_binary_op(v, ip, <=, VAL_NUMBER, numeric);
           break;
        };
        case OP_LT: {
           comp_binary_op(v, ip, <, VAL_NUMBER, numeric);
           break;
        };
        case OP_GT: {
           comp_binary_op(v, ip, >, VAL_NUMBER, numeric);
           break;
        };
        case OP_FLOAT_MULT_2: {
           binary_op(v, ip, *, VAL_NUMBER, numeric);
           break;
        };
        case OP_FLOAT_SUB: {
           binary_op(v, ip, -, VAL_NUMBER, numeric);
           break;
        };
        case OP_FLOAT_DIVIDE: {
           binary_op(v, ip, /, VAL_NUMBER, numeric);
           break;
        };
        case OP_FLOAT_SUM_2: {
           binary_op(v, ip, +, VAL_NUMBER, numeric);
           break;
        };
        case OP_ASSIGN_GLOBAL_16: {
            uint16_t i = *((uint16_t*) ip++);
            struct __guru_object o = c->consts.vals[i];
            if (!get_global(o.as.blob->__cont, o.as.blob->len, NULL)) {
                __runtime_error(v, ip - 2, "undefined variable: %.*s\n", o.as.blob->len, o.as.blob->__cont);
                return RESULT_RERR;
            };
            set_global(o.as.blob->__cont, o.as.blob->len, vm_st_head(v));
            ip++;
            break;
        };
        case OP_ASSIGN_GLOBAL: {
            struct __guru_object o = c->consts.vals[*ip++];
            if (!get_global(o.as.blob->__cont, o.as.blob->len, NULL)) {
                __runtime_error(v, ip - 2, "undefined variable: %.*s\n", o.as.blob->len, o.as.blob->__cont);
                return RESULT_RERR;
            };
            set_global(o.as.blob->__cont, o.as.blob->len, vm_st_head(v));
            break;
        };
        case OP_CLEAN_AFTER_BLOCK: {
            uint16_t i = *((uint16_t*) ip++);
            struct __guru_object obj = *vm_st_pop(v);
            // for (uint16_t x = 0; x < i; x++) obfree(vm_st_pop(v));
            *vm_st_push(v) = obj;
            ip++;
            break;
        };
        case OP_PUT_8_WITH_POP: {
            reg(v, *ip++) = *vm_st_pop(v);
            break;
        };
        case OP_LOAD_CLOSURE: {
            *vm_st_push(v) = *(((struct guru_callable*) reg(v, R_FUNC).as.blob->__cont)->closures[*ip++]);
            break;
        };
        case OP_PUT_CLOSURE: {
            *(((struct guru_callable*) reg(v, R_FUNC).as.blob->__cont)->closures[*ip++]) = *vm_st_head(v);
            break;
        };
        case OP_LOAD_LINK: {
            *vm_st_push(v) = ((struct __value_link*)reg(v, *ip++).as.blob->__cont)->o;
            break;
        };
        case OP_PUT_LINK: {
            ((struct __value_link*)reg(v, *ip++).as.blob->__cont)->o = *vm_st_pop(v);
            break;
        };
        case OP_PUT_8: {
            reg(v, *ip++) = *vm_st_head(v);
            break;
        };
        case OP_LOAD_8: {
            *vm_st_push(v) = reg(v, *ip++);
            break;
        };
        case OP_JUMP_BACK: {
            uint32_t i = *((uint32_t*) ip++);
            ip+=3;
            ip -= i;
            break;
        };
        case OP_JUMP: {
            uint32_t i = *((uint32_t*) ip++);
            ip+=3;
            ip += i;
            break;
        };
        case OP_JUMP_IF_TRUE_ONSTACK: {
            uint32_t i = *((uint32_t*) ip++);
            ip+=3;
            if (test(vm_st_head(v)))
                ip += i;
            break;
        };
        case OP_JUMP_IF_TRUE: {
            uint32_t i = *((uint32_t*) ip++);
            ip+=3;
            if (test(vm_st_pop(v)))
                ip += i;
            break;
        };
        case OP_JUMP_IF_FALSE_ONSTACK: {
            uint32_t i = *((uint32_t*) ip++);
            ip+=3;
            if (!test(vm_st_head(v)))
                ip += i;
            break;
        };
        case OP_JUMP_IF_FALSE: {
            uint32_t i = *((uint32_t*) ip++);
            ip+=3;
            if (!test(vm_st_pop(v)))
                ip += i;
            break;
        };
        case OP_DECLARE_ANON_FUNCTION: {
            uint32_t code = *((uint32_t*) ip);
            ip += 4;
            uint8_t arity = *ip++;
            uint32_t i = *((uint32_t*) ip);
            ip = c->code + i;
            ip++;
            uint8_t co = *ip++;
            struct __blob_header *blob = alloc_guru_callable(code, arity, co);
            struct guru_callable *gc = (struct guru_callable*)blob->__cont;
            for (uint8_t i = 0; i < co; i++) {
                uint8_t place = *ip++;
                if (v->r[place].tag != VAL_LINK) {
                    struct __blob_header *bh = __alloc_blob(sizeof(struct __value_link));
                    ((struct __value_link*)bh->__cont)->o = v->r[place];
                    ((struct __value_link*)bh->__cont)->rc = 1;
                    v->r[place].tag = VAL_LINK;
                    v->r[place].as.blob = bh;
                    gc->closures[i] = &((struct __value_link*)bh->__cont)->o;
                } else {
                    ((struct __value_link*)v->r[place].as.blob->__cont)->rc += 1;
                    gc->closures[i] = &((struct __value_link*)v->r[place].as.blob->__cont)->o;
                }
            }
            *vm_st_push(v) = (struct __guru_object){.as.blob = blob, .tag = BLOB_CALLABLE};
            break;
        };
        case OP_RETURN: {
            ip = c->code + reg(v, 0).as.bytes_4;
            v->current_frame_offset = reg(v, 1).as.byte;
            break;
        };
        case OP_CALL: {
            uint8_t cfo_old = v->current_frame_offset;
            v->current_frame_offset = cfo_old + *ip++;
            uint8_t provided_arity = *ip++;
            for (uint8_t i = provided_arity; i > 0; i--)
                reg(v, i + PRIVILEGED_STACK_SLOTS - 1) = *vm_st_pop(v);
            if (vm_st_head(v)->tag == BLOB_NATIVE) {
                struct native_func *o = (struct native_func*) vm_st_pop(v)->as.blob->__cont;
                *vm_st_push(v) = o->func(&reg(v, PRIVILEGED_STACK_SLOTS));
                v->current_frame_offset = cfo_old;
                continue;
            }
            if (vm_st_head(v)->tag != BLOB_CALLABLE) {
                __runtime_error(v, ip - 2, "attempting to call a non-function\n");
                return RESULT_RERR;
            }
            reg(v, 0) = (struct __guru_object){
                .as.bytes_4 = (uint32_t) (ip - c->code),
                .tag = VAL_4BYTE,
            };
            reg(v, 1) = (struct __guru_object){
                .tag = VAL_BYTE,
                .as.byte = cfo_old,
            };
            struct guru_callable *func = (struct guru_callable*) vm_st_head(v)->as.blob->__cont;
            reg(v, R_FUNC) = *vm_st_pop(v);
            if (provided_arity < func->arity) {
                __runtime_error(v, ip - 2, "not enough function arguments\n");
                return RESULT_RERR;
            }
            ip = c->code + func->func_begin;
            break;
        };
        case OP_PUT_8_VOID: {
            reg(v, *ip++) = __GURU_VOID;
            break;
        };
        case OP_PUT_8_NOTHING: {
            reg(v, *ip++) = __GURU_NOTHING;
            break;
        };
        case OP_LOAD_GLOBAL: {
            struct __guru_object o = c->consts.vals[*ip++];
            if (!get_global(o.as.blob->__cont, o.as.blob->len, vm_st_push(v))) {
                __runtime_error(v, ip - 2, "undefined variable: %.*s\n", o.as.blob->len, o.as.blob->__cont);
                return RESULT_RERR;
            };
            break;
        };
        case OP_LOAD_GLOBAL_16: {
            uint16_t i = *((uint16_t*) ip++);
            struct __guru_object o = c->consts.vals[i];
            if (!get_global(o.as.blob->__cont, o.as.blob->len, vm_st_push(v))) {
                __runtime_error(v, ip - 2, "undefined variable: %.*s\n", o.as.blob->len, o.as.blob->__cont);
                return RESULT_RERR;
            };
            ip++;
            break;
        };
        case OP_DEFINE_GLOBAL: {
            struct __guru_object o = c->consts.vals[*ip++];
            set_global(o.as.blob->__cont, o.as.blob->len, vm_st_pop(v));
            break;
        };
        case OP_DEFINE_GLOBAL_16: {
            uint16_t i = *((uint16_t*) ip++);
            struct __guru_object o = c->consts.vals[i];
            set_global(o.as.blob->__cont, o.as.blob->len, vm_st_pop(v));
            ip++;
        };
        case OP_POP_MANY: {
          //TODO: implement
            break;
        };
        case OP_OP: {
            obfree(vm_st_pop(v));
            break;
        };
        case OP_CONST: {
           *vm_st_push(v) = c->consts.vals[*ip++];
           break;
        };
        case OP_CONST_16: {
           uint16_t i = *((uint16_t*) ip++);
           *vm_st_push(v) = c->consts.vals[i];
           ip++;
           break;
        };
        case OP_RETURN_EXPRESSION: {
           struct __guru_object *o = vm_st_pop(v);
           break;
        }
        default: printf("unknown opcode! %d\n", *ip);
        }
    }
    __gc_collect();
    return RESULT_OK;
end_error:
    return RESULT_RERR;
};

void __fprint_val(FILE *f, struct __guru_object *val) {
    switch (val->tag) {
        case VAL_BOOL: 
             fprintf(f, "%s\n", val->as.bool ? "true" : "false");
             break;
        case VAL_NUMBER: 
             fprintf(f, "%f\n", val->as.numeric);
             break;
        case VAL_NOTHING:
             fprintf(f, "nothing\n");
             break;
        case BLOB_CALLABLE: {
             struct guru_callable *func = (struct guru_callable*) val->as.blob->__cont;
             fprintf(f, "[function] (%d:%d)\n", func->func_begin, func->arity);
             break;
        }
        case VAL_VOID: 
             fprintf(f, "void\n");
             break;
        case BLOB_STRING:
             fprintf(f, "%s\n", val->as.blob->__cont);
             break;
        default: return;
    }
}

void repl(struct guru_vm *v) {
    char *buf = malloc(1024);
    printf("guru-pt-001.001 stack-based scripting language\n");
    printf(">> ");

    
    while (fgets(buf, 1024, stdin)) {
        
        printf(">> ");
    }
};

enum exec_code interpret(struct guru_vm *v, const char *source) {
    struct chunk *c = challoc();
    
    if (compile(source, c) != RESULT_OK) {
        chfree(c);
        return RESULT_COMPERR;
    }

    enum exec_code i = run(v, c);

    chfree(c);
    return i;
}
void execf(struct guru_vm *v, const char *fname) {
    FILE *fin = fopen(fname, "rb");
    if (fin == NULL) error("could not open the file");
    fseek(fin, 0l, SEEK_END);
    size_t fsize = ftell(fin);
    rewind(fin);
    char *src = (char*) malloc(fsize+1);
    if (src == NULL) error("could not allocate enough memory to load your script");
    size_t i = fread(src, sizeof(char), fsize, fin);
    if (i != fsize) error("could not read the whole script");
    src[i] = '\0';
    fclose(fin);

    enum exec_code res = interpret(v, src);

    switch (res) {
        case RESULT_RERR: case RESULT_COMPERR: exit(1);
        case RESULT_OK: exit(0);
    }

    free(src);
    int falser;
};
