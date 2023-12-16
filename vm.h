#ifndef GURU_VM
#define GURU_VM
#include "bytecode.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>

void __fprint_val(FILE *f, struct __guru_object *val);
void __print_val(struct __guru_object *val);


#define LOCAL_VARIABLE_STACK_SIZE 1024
#define vm_st_push(v) ((v)->stack.head++)
#define comp_binary_op(v, ip, op, ttag, lift)  { \
    if (__get_stack_val(v, 0).tag != ttag) __runtime_error(v, ip-2, "expecting number operands"); \
    if (__get_stack_val(v, 1).tag != ttag) __runtime_error(v, ip-3, "expecting number operands"); \
    struct __guru_object *right = vm_st_pop(v); \
    struct __guru_object *left = vm_st_pop(v); \
    *vm_st_push(v) = (left->as).lift op (right->as).lift ? __GURU_TRUE : __GURU_FALSE;} while (0)

#define binary_op(v, ip, op, ttag, lift) do { \
    if (__get_stack_val(v, 0).tag != ttag) __runtime_error(v, ip-2, "expecting number operands"); \
    if (__get_stack_val(v, 1).tag != ttag) __runtime_error(v, ip-3, "expecting number operands"); \
    struct __guru_object *right = vm_st_pop(v); \
    struct __guru_object *left = vm_st_pop(v); \
    vm_st_push(v)->as.lift = (left->as).lift op (right->as).lift; \
    vm_st_head(v)->tag = ttag; } while (0)

#define R_FUNC 2

struct guru_vm {
    struct chunk *code;
    uint8_t *ip;
    uint16_t state;
    uint8_t current_frame_offset;
    /* some bit flags:
     * 0x0001 stands for runtime error mode to signal graceful shutdown or something
     * */

    struct {
        struct __guru_object *stack;
        struct __guru_object *head;
    } stack;

    /*INFO: local variable stack, 0-16 (??) (in each call frame) are privileged:
     * 0: ip for function returning,
     * 1: frame offset for function returning
     * 2: this in method calls
     * 3: exception (?)
     * 4-15: unassigned (yet)
     * each function call grabs another frame
     * */
    struct __guru_object r[LOCAL_VARIABLE_STACK_SIZE];
};

enum exec_code {
    RESULT_OK, RESULT_COMPERR, RESULT_RERR,
};

struct guru_vm *vm_init();
void vm_free(struct guru_vm *v);
void __runtime_error(struct guru_vm *vm, uint8_t *ip, const char *format, ...);
enum exec_code run(struct guru_vm *v, struct chunk *c);

#define __find_lninfo_for_op(v, code_n, line) do { \
    for (struct lninfo *i = v->code->line; i != NULL; i = i->next) \
        if (code_n >= i->opstart && code_n < i->opend) { \
            line = i->line; \
            break; }} while (0)

#define vm_st_head(v) ((v)->stack.head-1)
#define vm_st_pop(v) (--(v)->stack.head)
#define __get_stack_val(v, i) ((v)->stack.head[-1 - i])
#define __val_eq(l, r) ((((l)->tag == (r)->tag)) && memcmp(&(l)->as, &(r)->as, sizeof((l)->as)) == 0 ? __GURU_TRUE : __GURU_FALSE)
#define __val_neq(l, r) (((l)->tag != (r)->tag) || memcmp(&(l)->as, &(r)->as, sizeof((l)->as)) != 0 ? __GURU_TRUE : __GURU_FALSE)
#define reg(v, n) ((v)->r[(v)->current_frame_offset + (n)])

void repl(struct guru_vm *v);
void execf(struct guru_vm *v, const char *fname);
enum exec_code interpret(struct guru_vm *v, const char *source);
#endif
