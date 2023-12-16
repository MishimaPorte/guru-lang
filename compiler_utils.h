#ifndef GURU_COMPILER_UTILS
#define GURU_COMPILER_UTILS
#include "compiler.h"
#include <stdint.h>

void advance(struct compiler *comp);
void comp_err_report(struct compiler *comp, struct token *t, const char *msg);

void __begin_scope(struct compiler *comp);
void __end_scope(struct compiler *comp);

void __insert_op(struct compiler *comp, void *blob, size_t blob_size, enum guru_type t, enum opcode op, enum opcode op16);

uint32_t set_uint32(struct compiler *comp);
void patch_uint32(struct compiler *comp, uint32_t ptr, uint32_t patch);

uint8_t __loc_var_decl(struct compiler *comp);
void __loc_var_init(struct compiler *comp);
enum local_resolved {
    RESOLVED_NORMAL,
    RESOLVED_NEED_CLOSURE,
    RESOLVED_IS_CLOSURE,
    RESOLVED_NON_LOCAL,
};

enum local_resolved resolve_local(struct compiler *comp, struct local **l);

void __start_func_declaration(struct compiler *comp);
void __end_func_declaration(struct compiler *comp);

void jump_to(struct compiler *comp, uint8_t j, uint32_t wh);
uint32_t emit_jump(struct compiler *comp, uint8_t j);
void confirm_jump(struct compiler *comp, uint32_t offset);

uint32_t set_arity(struct compiler *comp);
void patch_arity(struct compiler *comp, uint32_t ptr, uint8_t arity);

#define had_err(c) ((c)->state & 0x0001)
#define had_panic(c) ((c)->state & 0x0002)
#define panic_if_not_this(c, tt, msg) do { \
    if (c->now.type == tt) advance(c); else comp_err_report(c, &c->now, msg); } while (0)

#define __enter_panic_mode(comp) ((comp)->state |= PANIC_MODE)

#endif
