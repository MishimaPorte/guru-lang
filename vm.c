#include "vm.h"
#include "bytecode.h"
#include "compiler.h"
#include "debug.h"
#include "ffi.h"
#include "guru.h"
#include "hashmap.h"
#include "object.h"
#include "value.h"
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


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
#ifdef DEBUG_MODE
           if (__get_stack_val(v, 0).tag != VAL_NUMBER) __runtime_error(v, ip-1, "expecting number operand, but got %s", type_names_map[__get_stack_val(v, 0).tag]);
#else
           if (__get_stack_val(v, 0).tag != VAL_NUMBER) __runtime_error(v, ip-1, "expecting number operand");
#endif
           vm_st_head(v)->as.numeric = -vm_st_head(v)->as.numeric;
           break;
        }
        case OP_LOGNOT: {
#ifdef DEBUG_MODE
           if (__get_stack_val(v, 0).tag != VAL_BOOL) __runtime_error(v, ip-1, "expecting boolean operand, but got %s", type_names_map[__get_stack_val(v, 0).tag]);
#else
           if (__get_stack_val(v, 0).tag != VAL_BOOL) __runtime_error(v, ip-1, "expecting boolean operand");
#endif
           if (vm_st_head(v)->as.bool) vm_st_head(v)->as.bool = 0;
           else vm_st_head(v)->as.bool = 1;
           break;
        }
        case OP_STRING_EQ: {
#ifdef DEBUG_MODE
           if (__get_stack_val(v, 0).tag != BLOB_STRING) __runtime_error(v, ip-2, "expecting string operand, but got %s", type_names_map[__get_stack_val(v, 0).tag]);
           if (__get_stack_val(v, 1).tag != BLOB_STRING) __runtime_error(v, ip-3, "expecting string operand, but got %s", type_names_map[__get_stack_val(v, 1).tag]);
#else
           if (__get_stack_val(v, 0).tag != BLOB_STRING) __runtime_error(v, ip-2, "expecting string operand");
           if (__get_stack_val(v, 1).tag != BLOB_STRING) __runtime_error(v, ip-3, "expecting string operand");
#endif
           struct __guru_object *l = vm_st_pop(v);
           struct __guru_object *r = vm_st_pop(v);
           struct __guru_object ret = (l->as.blob == r->as.blob || l->as.blob->len == r->as.blob->len && !memcmp(&l->as.blob->__cont[0], &r->as.blob->__cont[0], l->as.blob->len) ? __GURU_TRUE : __GURU_FALSE);
           obfree(l);
           obfree(r);
           *vm_st_push(v) = ret;
           break;
        };
        case OP_STR_CONC: {
#ifdef DEBUG_MODE
           if (__get_stack_val(v, 0).tag != BLOB_STRING) __runtime_error(v, ip-2, "expecting string operand, but got %s", type_names_map[__get_stack_val(v, 0).tag]);
           if (__get_stack_val(v, 1).tag != BLOB_STRING) __runtime_error(v, ip-3, "expecting string operand, but got %s", type_names_map[__get_stack_val(v, 1).tag]);
#else
           if (__get_stack_val(v, 0).tag != BLOB_STRING) __runtime_error(v, ip-2, "expecting string operand");
           if (__get_stack_val(v, 1).tag != BLOB_STRING) __runtime_error(v, ip-3, "expecting string operand");
#endif
           struct __guru_object *val = vm_st_pop(v);
           struct __guru_object *head = vm_st_pop(v);

           struct __blob_header *blob = __alloc_blob(val->as.blob->len + head->as.blob->len - 1);
           memcpy(blob->__cont, (void *) head->as.blob->__cont, head->as.blob->len - 1);
           memcpy(blob->__cont + head->as.blob->len - 1, (void *) val->as.blob->__cont, val->as.blob->len);

           obfree(val);
           obfree(head);

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
           *vm_st_push(v) = __GURU_TRUE;
           break;
        }
        case OP_LOAD_LIES:{
           *vm_st_push(v) = __GURU_FALSE;
           break;
        }
        case OP_NEQ: {
           struct __guru_object *l = vm_st_pop(v);
           struct __guru_object *r = vm_st_pop(v);
           *vm_st_push(v) = __val_neq(l, r);
           obfree(l);
           obfree(r);
           break;
        };
        case OP_EQ: {
           struct __guru_object *l = vm_st_pop(v);
           struct __guru_object *r = vm_st_pop(v);
           *vm_st_push(v) = __val_eq(l, r);
           obfree(l);
           obfree(r);
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
        case OP_INDEX_SET: {
           struct __guru_object *rval = vm_st_pop(v);
           struct __guru_object *index = vm_st_pop(v);
           struct __guru_object *arr = vm_st_head(v);
           switch (arr->tag) {
               case VAL_UNSAFE_ALLOCATED: {
                   ((uint8_t*)arr->as.pointer)[(uint64_t)index->as.numeric] = (uint8_t)rval->as.numeric;
                   break;
               };
               case VAL_ARRAY_REF: {
                    struct array_of_refs *ar = (struct array_of_refs*)arr->as.blob->__cont;
                    if (ar->len <= (size_t)index->as.numeric)
                        __runtime_error(v, ip, "out of bounds");
                   ar->__vals[(size_t)index->as.numeric] = (struct __guru_object*)__alloc_blob(sizeof(struct __guru_object))->__cont;
                   obtake(rval);
                   memcpy(ar->__vals[(size_t)index->as.numeric], rval, sizeof(struct __guru_object));
                   break;
               };
                default: __runtime_error(v, ip, "indexing failed, %d, %d, %d", arr->tag, index->tag, rval->tag);
           }
           break;
        };
        case OP_INDEX_GET: {
           struct __guru_object *val = vm_st_pop(v);
           struct __guru_object *mem = vm_st_pop(v);
           switch (mem->tag) {
               case VAL_UNSAFE_ALLOCATED: {
                   *vm_st_push(v) = (struct __guru_object) {
                        .as.numeric = (double) ((uint8_t*)mem->as.pointer)[(uint64_t)val->as.numeric],
                        .tag = VAL_NUMBER,
                   }; 
                   break;
               };
               case VAL_ARRAY_REF: {
                    struct array_of_refs *ar = (struct array_of_refs*)mem->as.blob->__cont;
                    if (ar->len <= (size_t)val->as.numeric)
                        __runtime_error(v, ip, "out of bounds");
                    *vm_st_push(v) = *(ar->__vals[(size_t)val->as.numeric]);
                   break;
               };
                default: __runtime_error(v, ip, "indexing failed");
           }
           break;
        };
        case OP_ASSIGN_GLOBAL_16: {
            uint16_t i = *((uint16_t*) ip++);
            struct __guru_object o = c->consts.vals[i];
            if (!get_global(o.as.blob->__cont, o.as.blob->len, NULL)) {
                __runtime_error(v, ip - 2, "undefined variable: %.*s\n", o.as.blob->len, o.as.blob->__cont);
                return RESULT_RERR;
            };
            obtake(vm_st_head(v));
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
            obtake(vm_st_head(v));
            set_global(o.as.blob->__cont, o.as.blob->len, vm_st_head(v));
            break;
        };
        case OP_PUT_8_WITH_POP: {
            obtake(vm_st_head(v));
            reg(v, *ip++) = *vm_st_pop(v);
            break;
        };
        case OP_LOAD_CLOSURE: {
            *vm_st_push(v) = ((struct guru_callable*) reg(v, R_FUNC).as.blob->__cont)->closures[*ip++]->o;
            break;
        };
        case OP_PUT_CLOSURE: {
            obtake(vm_st_head(v));
            ((struct guru_callable*) reg(v, R_FUNC).as.blob->__cont)->closures[*ip++]->o = *vm_st_head(v);
            break;
        };
        case OP_LOAD_LINK: {
            *vm_st_push(v) = ((struct __value_link*)reg(v, *ip++).as.blob->__cont)->o;
            break;
        };
        case OP_PUT_LINK: {
            obtake(vm_st_head(v));
            ((struct __value_link*)reg(v, *ip++).as.blob->__cont)->o = *vm_st_pop(v);
            break;
        };
        case OP_PUT_8: {
            obtake(vm_st_head(v));
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
        case OP_COLLECT_LOCALS: {
            uint16_t amount = *(uint16_t*)ip;
            ip+=2;
            for (uint16_t i = amount; i > 0; i--) obfree(&reg(v, PRIVILEGED_STACK_SLOTS - 1 + i));
            break;
        };
        case OP_RETURN: {
            ip = c->code + reg(v, R_RETURN_INSTR_PTR).as.bytes_4;
            v->current_frame_offset = reg(v, R_RETURN_STACK_FRAME).as.byte;
            break;
        };
        case OP_CALL: {
            uint8_t cfo_old = v->current_frame_offset;
            v->current_frame_offset = cfo_old + *ip++;
            uint8_t provided_arity = *ip++;
            for (uint8_t i = provided_arity; i > 0; --i)
                reg(v, i + PRIVILEGED_STACK_SLOTS - 1) = *vm_st_pop(v);
            if (vm_st_head(v)->tag == BLOB_NATIVE) {
                struct native_func *o = (struct native_func*) vm_st_pop(v)->as.blob->__cont;
                if (typecheck(&reg(v, PRIVILEGED_STACK_SLOTS), o) == 0)
                {
                    __runtime_error(v, ip, "attempting to call a native with wrong arguments\n");
                    return RESULT_RERR;
                };
                *vm_st_push(v) = o->func(&reg(v, PRIVILEGED_STACK_SLOTS));
                v->current_frame_offset = cfo_old;
                continue;
            } else if (vm_st_head(v)->tag == BLOB_CLASS) {
                struct guru_class *o = (struct guru_class*) vm_st_pop(v)->as.blob->__cont;
                struct __guru_object *obj = NULL;
                struct __blob_header *inst = alloc_instance(o);
                if ((obj = get_val(&o->fields, "__constructor", 14)) == NULL) {
                    *vm_st_push(v) = (struct __guru_object) {
                        .tag = BLOB_INST,
                        .as.blob = inst,
                    };
                } else {
                    if (obj->tag != BLOB_CALLABLE) {
                        __runtime_error(v, ip - 2, "constructor is anything but a function\n");
                        return RESULT_RERR;
                    }
                    reg(v, R_RETURN_INSTR_PTR) = (struct __guru_object){
                        .as.bytes_4 = (uint32_t) (ip - c->code),
                        .tag = VAL_4BYTE,
                    };
                    reg(v, R_RETURN_STACK_FRAME) = (struct __guru_object){
                        .tag = VAL_8BYTE,
                        .as.byte = cfo_old,
                    };
                    reg(v, R_FUNC) = *obj;
                    reg(v, R_THIS) = (struct __guru_object){
                        .tag = BLOB_INST,
                        .as.blob = inst,
                    };

                    if (provided_arity < ((struct guru_callable*)obj->as.blob->__cont)->arity) {
                        __runtime_error(v, ip - 2, "not enough function arguments\n");
                        return RESULT_RERR;
                    }
                    ip = c->code + ((struct guru_callable*)obj->as.blob->__cont)->func_begin;
                    continue;
                }
                v->current_frame_offset = cfo_old;
                continue;
            }
            if (vm_st_head(v)->tag != BLOB_CALLABLE) {
#ifdef DEBUG_MODE
                __runtime_error(v, ip - 2, "attempting to call a non-function, but a %s\n", type_names_map[vm_st_head(v)->tag]);
#else
                __runtime_error(v, ip - 2, "attempting to call a non-function\n");
#endif
                return RESULT_RERR;
            }
            reg(v, R_RETURN_INSTR_PTR) = (struct __guru_object){
                .as.bytes_4 = (uint32_t) (ip - c->code),
                .tag = VAL_4BYTE,
            };
            reg(v, R_RETURN_STACK_FRAME) = (struct __guru_object){
                .tag = VAL_8BYTE,
                .as.byte = cfo_old,
            };
            struct guru_callable *func = (struct guru_callable*) vm_st_head(v)->as.blob->__cont;
            if (func->__is_method)
                reg(v, R_THIS) = *func->this;
            else
                reg(v, R_THIS) = __GURU_NOTHING;
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
        case OP_LOAD_THIS: {
            *vm_st_push(v) = reg(v, R_THIS);
            break;
        }
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
            obtake(vm_st_head(v));
            set_global(o.as.blob->__cont, o.as.blob->len, vm_st_pop(v));
            break;
        };
        case OP_DEFINE_GLOBAL_16: {
            uint16_t i = *((uint16_t*) ip++);
            struct __guru_object o = c->consts.vals[i];
            obtake(vm_st_head(v));
            set_global(o.as.blob->__cont, o.as.blob->len, vm_st_pop(v));
            ip++;
        };
        case OP_OP_NOFREE: {
            vm_st_pop(v);
            break;
        };
        case OP_OP: {
            obfree(vm_st_pop(v));
            break;
        };
        case OP_SET: {
           struct __guru_object o = c->consts.vals[*ip++];

           if (o.tag != BLOB_STRING)
           {
               __runtime_error(v, ip, "setter failed");
               break;
           };

           obtake(vm_st_head(v));
           struct __guru_object *val = vm_st_pop(v);

           struct __guru_object *inst = vm_st_pop(v);
           if (inst->tag != BLOB_INST)
           {
#ifdef DEBUG_MODE
               __runtime_error(v, ip, "trying to set not an object's field, but %s's", type_names_map[inst->tag]);
#else
               __runtime_error(v, ip, "trying to set not an object's field");
#endif
               break;
           };

           struct __blob_header *obj = __alloc_blob(sizeof(struct __guru_object));
           if (val->tag == VAL_LINK) {
               memcpy(obj->__cont, &((struct __value_link*)val->as.blob->__cont)->o, sizeof(struct __guru_object));
           } else {
               memcpy(obj->__cont, val, sizeof(struct __guru_object));
           }

           set_field((struct guru_instance *)inst->as.blob->__cont, o.as.blob->__cont, o.as.blob->len, (struct __guru_object*)obj->__cont);
           *vm_st_push(v) = *val;
           break;
        };
        case OP_SET_16: {
           uint16_t i = *((uint16_t*) ip);
           ip+=2;
           struct __guru_object o = c->consts.vals[i];

           if (o.tag != BLOB_STRING)
           {
               __runtime_error(v, ip, "setter failed");
               break;
           };

           obtake(vm_st_head(v));
           struct __guru_object *val = vm_st_pop(v);

           struct __guru_object *inst = vm_st_pop(v);
           if (inst->tag != BLOB_INST)
           {
               __runtime_error(v, ip, "trying to set not an object's field");
               break;
           };

           struct __blob_header *obj = __alloc_blob(sizeof(struct __guru_object));
           if (val->tag == VAL_LINK) {
               memcpy(obj->__cont, &((struct __value_link*)val->as.blob->__cont)->o, sizeof(struct __guru_object));
           } else {
               memcpy(obj->__cont, val, sizeof(struct __guru_object));
           }

           set_field((struct guru_instance *)inst->as.blob->__cont, o.as.blob->__cont, o.as.blob->len, (struct __guru_object*)obj->__cont);
           *vm_st_push(v) = *val;
           break;
        };
        case OP_GET: {
           struct __guru_object o = c->consts.vals[*ip++];
           if (o.tag != BLOB_STRING)
           {
               __runtime_error(v, ip, "getter failed");
               break;
           };

           struct __guru_object *inst = vm_st_pop(v);
           if (inst->tag != BLOB_INST)
           {
#ifdef DEBUG_MODE
               __runtime_error(v, ip, "trying to get not from an instance, but from a %s", type_names_map[inst->tag]);
#else
               __runtime_error(v, ip, "trying to get not from an instance");
#endif
               break;
           };


           struct guru_instance *i = (struct guru_instance*)inst->as.blob->__cont;
           struct __guru_object *val = get_field(i, o.as.blob->__cont, o.as.blob->len);

           if (val == NULL)
               *vm_st_push(v) = __GURU_VOID;
           else {
               if (val->tag == BLOB_CALLABLE)
                   if (((struct guru_callable*)val->as.blob->__cont)->__is_method)
                   {
                        struct __blob_header *inst_blob = __alloc_blob(sizeof(struct __guru_object));
                        ((struct __guru_object*)inst_blob->__cont)->tag = BLOB_INST;
                        ((struct __guru_object*)inst_blob->__cont)->as.blob = inst->as.blob;
                        ((struct guru_callable*)val->as.blob->__cont)->this = (struct __guru_object*)inst_blob->__cont;
                   }
               *vm_st_push(v) = *val;
           }
           break;
        };
        case OP_FOR_LOOP: 
        case OP_FOR_LOOP_END:
           break;
        case OP_GET_16: {
           uint16_t con = *((uint16_t*) ip);
           struct __guru_object o = c->consts.vals[con];
           ip+=2;
           if (o.tag != BLOB_STRING)
           {
               __runtime_error(v, ip, "getter failed");
               break;
           };

           struct __guru_object *inst = vm_st_pop(v);
           if (o.tag != BLOB_INST)
           {
               __runtime_error(v, ip, "trying to get not from an instance");
               break;
           };

           struct guru_instance *i = (struct guru_instance *)inst->as.blob->__cont;
           struct __guru_object *val = get_field(i, o.as.blob->__cont, o.as.blob->len);
           if (val == NULL)
               *vm_st_push(v) = __GURU_VOID;
           else
               *vm_st_push(v) = *val;
           break;
        };
        case OP_CONST: {
           *vm_st_push(v) = c->consts.vals[*ip++];
           break;
        };
        case OP_DECLARE_CLASS: {
           struct __guru_object cl = c->consts.vals[*ip++];
           struct __blob_header *new_cl = alloc_class(); 

           uint8_t meth_count = *ip++;
           for (uint8_t i = meth_count; i > 0; i--) {
               uint16_t cons = *(uint16_t*)ip;
               ip+=2;
               struct __guru_object nomen = c->consts.vals[cons];

               struct __blob_header *bh = __alloc_blob(sizeof(struct __guru_object));

               ((struct __guru_object*)bh->__cont)->tag = BLOB_CALLABLE;
               ((struct __guru_object*)bh->__cont)->as.blob = vm_st_pop(v)->as.blob;

               set_val(&((struct guru_class*)new_cl->__cont)->fields, nomen.as.blob->__cont, nomen.as.blob->len, bh->__cont);

           } 
           *vm_st_push(v) = (struct __guru_object) {
               .tag = BLOB_CLASS,
               .as.blob = new_cl
           };
           break;
        };
        case OP_DECLARE_CLASS_16: {
           struct __guru_object cl = c->consts.vals[*ip];
           ip+=2;
           struct __blob_header *bh = alloc_class(); 

           copy_class((struct guru_class*)cl.as.blob->__cont, (struct guru_class*)bh->__cont);
           *vm_st_push(v) = (struct __guru_object) {
               .tag = BLOB_CLASS,
               .as.blob = bh
           };
           break;
        };
        case OP_DECLARE_FUNCTION: {
           struct guru_callable *callable = (struct guru_callable*) c->consts.vals[*ip++].as.blob->__cont;
           uint8_t co = *ip++;
           struct __blob_header *bh = alloc_guru_callable(callable->func_begin, callable->arity, co); 
           uint8_t i;
           struct guru_callable *gc = (struct guru_callable*)bh->__cont;
           gc->__is_method = callable->__is_method;
           for (i = 0; i < co; i++) {
               uint8_t place = *ip++;
               if (v->r[place].tag != VAL_LINK) {
                   struct __blob_header *bh = __alloc_blob(sizeof(struct __value_link));
                   ((struct __value_link*)bh->__cont)->o = v->r[place];
                   v->r[place].tag = VAL_LINK;
                   v->r[place].as.blob = bh;
                   gc->closures[i] = (struct __value_link*) bh->__cont;
               } else {
                   v->r[place].as.blob->__rc += 1;
                   gc->closures[i] = (struct __value_link*) v->r[place].as.blob->__cont;
               }
           };
           gc->closures[co] = NULL;

           *vm_st_push(v) = (struct __guru_object) {
               .tag = BLOB_CALLABLE,
               .as.blob = bh
           };
           break;
        };
        case OP_DECLARE_FUNCTION_16: {
           struct __guru_object cl = c->consts.vals[*ip];
           ip+=2;
           struct guru_callable *callable = (struct guru_callable*) cl.as.blob;
           uint8_t co = *ip++;
           struct __blob_header *bh = alloc_guru_callable(callable->func_begin, callable->arity, co); 
           uint8_t i;
           struct guru_callable *gc = (struct guru_callable*)bh->__cont;
           for (i = 0; i < co; i++) {
               uint8_t place = *ip++;
               if (v->r[place].tag != VAL_LINK) {
                   struct __blob_header *bh = __alloc_blob(sizeof(struct __value_link));
                   ((struct __value_link*)bh->__cont)->o = v->r[place];
                   v->r[place].tag = VAL_LINK;
                   v->r[place].as.blob = bh;
                   gc->closures[i] = (struct __value_link*) bh->__cont;
               } else {
                   v->r[place].as.blob->__rc += 1;
                   gc->closures[i] = (struct __value_link*) v->r[place].as.blob->__cont;
               }
           };
           gc->closures[co] = NULL;

           *vm_st_push(v) = (struct __guru_object) {
               .tag = BLOB_CALLABLE,
               .as.blob = bh
           };
           break;
        };
        case OP_CONST_16: {
           uint16_t i = *((uint16_t*) ip++);
           *vm_st_push(v) = c->consts.vals[i];
           ip++;
           obtake(vm_st_head(v));
           break;
        };
        default: printf("unknown opcode! %d\n", *(ip-1));
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
        case BLOB_NATIVE: 
             fprintf(f, "native [%p]\n", val->as.blob);
             break;
        case VAL_NUMBER: 
             fprintf(f, "%f\n", val->as.numeric);
             break;
        case VAL_NOTHING:
             fprintf(f, "nothing\n");
             break;
        case BLOB_CALLABLE: {
             struct guru_callable *func = (struct guru_callable*) val->as.blob->__cont;
             fprintf(f, "[function %p] (%d:%d)\n", val->as.blob, func->func_begin, func->arity);
             break;
        }
        case VAL_FILE: 
             fprintf(f, "file [%p]\n", val->as.pointer);
             break;
        case VAL_VOID: 
             fprintf(f, "void\n");
             break;
        case BLOB_STRING:
             fprintf(f, "%s\n", val->as.blob->__cont);
             break;
        case VAL_LINK: 
             __fprint_val(f, &((struct __value_link*) val->as.blob->__cont)->o);
             break;
        default:
#ifdef DEBUG_MODE
             fprintf(f, "%s tag: %d [%s]\n", "unknown thing printed!", val->tag, type_names_map[val->tag]);
#else
             fprintf(f, "%s tag: %d\n", "unknown thing printed!", val->tag);
#endif
             break;
             ;
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
