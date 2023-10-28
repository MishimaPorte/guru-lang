#include "vm.h"
#include "bytecode.h"
#include "compiler.h"
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
    fprintf(stderr, "[line %d] runtime error\n", line);

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

    for (;;) {
        if (ip > c->code + c->opcount - 1) break;
        else if (v->state & 0x0001) goto end_error;
        switch (*ip++) {
        case OP_EXIT: exit(0);
        case OP_PR_STHEAD: {
           struct __guru_object *printed = vm_st_pop(v);
           printf("printing: %f\n", printed->as.numeric);
           __untake_obj(printed);
           break;
          };
        case OP_FLOAT_UN_PLUS: break;
        case OP_FLOAT_NEGATE: {
           if (__get_stack_val(v, 0)->tag != VAL_NUMBER) __runtime_error(v, ip-1, "expecting number operand");
           struct __guru_object *val = vm_st_pop(v);
           __untake_obj(val);
           double valnum = -val->as.numeric;
           vm_st_push(v, __as_guru_numeric(valnum));
           break;
          }
        case OP_LOGNOT: {
           if (__get_stack_val(v, 0)->tag != VAL_BOOL) __runtime_error(v, ip-1, "expecting boolean operand");
           __guru_not(*(v->stack.head-1));
           break;
        }
        case OP_STRING_EQ: {
           if (__get_stack_val(v, 0)->tag != BLOB_STRING) __runtime_error(v, ip-2, "expecting string operand");
           if (__get_stack_val(v, 1)->tag != BLOB_STRING) __runtime_error(v, ip-3, "expecting string operand");
           struct __blob_header *l = vm_st_pop(v)->as.blob;
           struct __blob_header *r = vm_st_pop(v)->as.blob;
           vm_st_push(v, (l->len == r->len && !memcmp(&l->__cont[0], &r->__cont[0], l->len) ? &__GURU_TRUE : &__GURU_FALSE));
           __untake_obj(l);
           __untake_obj(r);
           break;
          };
        case OP_STR_CONC: {
           if (__get_stack_val(v, 0)->tag != BLOB_STRING) __runtime_error(v, ip-2, "expecting string operand");
           if (__get_stack_val(v, 1)->tag != BLOB_STRING) __runtime_error(v, ip-3, "expecting string operand");
           struct __guru_object *val = vm_st_pop(v);
           struct __guru_object *head = *(v->stack.head - 1);
           uint64_t old_s = head->as.blob->len;
           head->as.blob = __realloc_blob(head->as.blob, head->as.blob->len + val->as.blob->len);
           memcpy(head->as.blob->__cont + old_s, val->as.blob->__cont, val->as.blob->len);
           __untake_obj(val);
           break;
          }
        case OP_LOAD_VOID:
           vm_st_push(v, &__GURU_VOID);
           break;
        case OP_LOAD_NOTHING:
           vm_st_push(v, &__GURU_NOTHING);
           break;
        case OP_LOAD_TRUTH: {
           uint64_t i = 1;
           vm_st_push(v, __alloc_VAL_BOOL(&i, 8));
           break;
                            }
        case OP_LOAD_LIES:{
           uint64_t i = 0;
           vm_st_push(v, __alloc_VAL_BOOL(&i, 8));
           break;
                          }
        case OP_NEQ: {
           struct __guru_object *l = vm_st_pop(v);
           struct __guru_object *r = vm_st_pop(v);
           vm_st_push(v, __val_neq(l, r));
           __untake_obj(l);
           __untake_obj(r);
           break;
          };
        case OP_EQ: {
           struct __guru_object *l = vm_st_pop(v);
           struct __guru_object *r = vm_st_pop(v);
           vm_st_push(v, __val_eq(l, r));
           __untake_obj(l);
           __untake_obj(r);
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
         // __runtime_error(v, "summation does not work any more");
           binary_op(v, ip, +, VAL_NUMBER, numeric);
           break;
          };
        case OP_CONST: {
           vm_st_push(v, c->consts.vals[*ip++]);
           break;
          };
        case OP_CONST_16: {
           uint16_t i = *((uint16_t*) ip++);
           vm_st_push(v, c->consts.vals[i]);
           ip++;
           break;
          };
        case OP_RETURN_EXPRESSION:
              __print_val(vm_st_pop(v));
             break;
        default: printf("unknown opcode! %d\n", *ip);
        }
    }
    for (struct __guru_object **o = v->stack.stack; *o != NULL; o++) {
        printf("kek: [%d] (%d)\n", (*o)->tag, (*o)->__rc);
    }
    return RESULT_OK;
end_error:
    return RESULT_RERR;
};

void __print_val(struct __guru_object *val) {
    switch (val->tag) {
        case VAL_BOOL: 
             printf("returning: %s\n", val->as.bool ? "true" : "false");
             break;
        case VAL_NUMBER: 
             printf("returning: %f\n", val->as.numeric);
             break;
        case VAL_NOTHING: 
             printf("returning: nothing\n");
             break;
        case VAL_VOID: 
             printf("returning: void\n");
             break;
        case BLOB_STRING:
             printf("returning: %.*s\n", val->as.blob->len, val->as.blob->__cont);
             break;
        default: return;
    }
    __untake_obj(val);
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
