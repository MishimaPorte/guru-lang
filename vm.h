#ifndef GURU_VM
#define GURU_VM
#include "bytecode.h"
#include "value.h"
#include <stdint.h>

void __print_val(struct __guru_object *val);

#define comp_binary_op(v, ip, op, ttag, lift)  { \
    if (__get_stack_val(v, 0)->tag != ttag) __runtime_error(v, ip-2, "expecting number operands"); \
    if (__get_stack_val(v, 1)->tag != ttag) __runtime_error(v, ip-3, "expecting number operands"); \
    struct __guru_object *right = vm_st_pop(v); \
    struct __guru_object *left = vm_st_pop(v); \
    vm_st_push(v, (left->as).lift op (right->as).lift ? &__GURU_TRUE : &__GURU_FALSE); \
   __untake_obj(left); \
   __untake_obj(right); \
    } while (0)

#define binary_op(v, ip, op, ttag, lift) do { \
    if (__get_stack_val(v, 0)->tag != ttag) __runtime_error(v, ip-2, "expecting number operands"); \
    if (__get_stack_val(v, 1)->tag != ttag) __runtime_error(v, ip-3, "expecting number operands"); \
    struct __guru_object *right = vm_st_pop(v); \
    struct __guru_object *left = vm_st_pop(v); \
    typeof((left->as).lift) res = (left->as).lift op (right->as).lift; \
    vm_st_push(v, __as_guru_##lift(res)); } while (0)

struct guru_vm {
    struct chunk *code;
    uint8_t *ip;
    uint16_t state;
    /* some bit flags:
     * 0x0001 stands for runtime error mode to signal graceful shutdown or something
     * */

    struct {
        struct __guru_object **stack;
        struct __guru_object **head;
    } stack;
};

enum exec_code {
    RESULT_OK, RESULT_COMPERR, RESULT_RERR,
};

struct guru_vm *vm_init();
void vm_free(struct guru_vm *v);
void __runtime_error(struct guru_vm *vm, uint8_t *ip, const char *format, ...);
enum exec_code run(struct guru_vm *v, struct chunk *c);

#define chprint(c) for (uint16_t i = 0; i < c->opcount; i++) printf("%02x ", c->code[i]);
#define consprint(c) for (uint16_t i = 0; i < c.count; i++) printf("%02f ", c.vals[i]);
#define hexprint(c) for (uint16_t i = 0; i < sizeof(*c); i++) printf("%02x ", ((uint8_t*) c)[i]);

#define __find_lninfo_for_op(v, code_n, line) do { \
    for (struct lninfo *i = v->code->line; i != NULL; i = i->next) \
        if (code_n >= i->opstart && code_n < i->opend) { \
            line = i->line; \
            break; }} while (0)

#define vm_st_push(v, val) *((v)->stack.head++) = val
#define vm_st_pop(v) (*(--(v)->stack.head), *(v)->stack.head)
#define __get_stack_val(v, i) ((v)->stack.head[-1 - i])
#define __val_eq(l, r) ((((l)->tag == (r)->tag)) && memcmp(&(l)->as, &(r)->as, sizeof((l)->as)) == 0 ? &__GURU_TRUE : &__GURU_FALSE)
#define __val_neq(l, r) (((l)->tag != (r)->tag) || memcmp(&(l)->as, &(r)->as, sizeof((l)->as)) != 0 ? &__GURU_TRUE : &__GURU_FALSE)

void repl(struct guru_vm *v);
void execf(struct guru_vm *v, const char *fname);
enum exec_code interpret(struct guru_vm *v, const char *source);
#endif
