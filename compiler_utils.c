#include "compiler_utils.h"
#include "bytecode.h"
#include "compiler.h"
#include "scanner.h"
#include <bits/pthreadtypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void __insert_op(struct compiler *comp, void *blob, size_t blob_size, enum guru_type t, enum opcode op, enum opcode op16) {
    uint16_t i = add_const(comp->compiled_chunk, blob, blob_size, t);
    if (i<UINT8_MAX) {
        chput(comp->compiled_chunk, op);
        chput(comp->compiled_chunk, (uint8_t) i);
        return;
    }
    if (i == UINT16_MAX) {
        comp_err_report(comp, &comp->then, "too many constants for a single chunk");
        return;
    }
    chput(comp->compiled_chunk, op16);
    chputn(comp->compiled_chunk, &i, sizeof(i));
}

void comp_err_report(struct compiler *comp, struct token *t, const char *msg) {
    if (had_panic(comp)) return;
    fprintf(stderr, "ERROR: line: %d, pos: %d ", t->line, t->pos);

    if (t->type == GURU_EOF) fprintf(stderr, "at the end");
    else fprintf(stderr, "at '%.*s'", t->len, t->lexeme);

    fprintf(stderr, ": %s.\n", msg);
    comp->state |= PANIC_MODE;
    comp->state |= ROTTEN;
}

void advance(struct compiler *comp) {
    memcpy(&comp->then, &comp->now, sizeof(struct token));

    for (;;) {
        scgett(comp->scanner, &comp->now);
        if (comp->now.line != comp->line) chputl(comp->compiled_chunk), comp->line = comp->now.line;
        if (comp->now.type != GURU_ERROR) break;

        comp_err_report(comp, &comp->now, "bad token");
    }
}
void __begin_scope(struct compiler *comp) {
    comp->scope_depth++;
};
void __end_scope(struct compiler *comp) {
    uint16_t i = 0;
    if (comp->local_count != 0) while (comp->locals[comp->local_count-1].depth == comp->scope_depth) {
        comp->local_count--;
        i++;
    }
    comp->scope_depth--;
};

uint8_t __loc_var_decl(struct compiler *comp) {
    if (comp->local_count == 0) goto end;
    uint16_t i = comp->local_count;
    do {
        struct local* local = &comp->locals[--i];
        if (local->depth < comp->scope_depth)
            break;
        if (comp->then.len == local->name.len && memcmp((void*) comp->then.lexeme, (void*)local->name.lexeme, local->name.len) == 0)
            comp_err_report(comp, &comp->then, "already a variable with this name in this scope.");
    } while (i != 0);
end:
    ""; // linter thing
    struct local *l = &comp->locals[comp->local_count++];
    memcpy((void*)&l->name, (void*)&comp->then, sizeof(struct token));
    l->depth = 0; // variable is uninited at first
    if (comp->stack.head != 0) {
        l->ofsetted = comp->stack.s[comp->stack.head-1]++;
        return l->ofsetted + PRIVILEGED_STACK_SLOTS;
    }
    return comp->local_count + PRIVILEGED_STACK_SLOTS - 1;
};

void __loc_var_init(struct compiler *comp) {
    comp->locals[comp->local_count - 1].depth = comp->scope_depth;
}

uint32_t set_arity(struct compiler *comp) {
    chput(comp->compiled_chunk, 0xff);

    return comp->compiled_chunk->opcount - 1;
};

void patch_arity(struct compiler *comp, uint32_t ptr, uint8_t arity) {
    comp->compiled_chunk->code[ptr] = arity;
};

uint8_t resolve_local(struct compiler *comp) {
    if (comp->local_count == 0) return 0;
    uint8_t i = comp->local_count;
    do {
        struct local* local = &comp->locals[--i];
        if (local->depth != 0 && comp->then.len == local->name.len && memcmp((void*) comp->then.lexeme, (void*)local->name.lexeme, local->name.len) == 0) {
            if (comp->stack.head != 0) {
                return local->ofsetted + PRIVILEGED_STACK_SLOTS;
            };
            return i + PRIVILEGED_STACK_SLOTS;
        }
        if (i == 0) break;
    } while (1);
    return 0;
};

void jump_to(struct compiler *comp, uint8_t j, uint32_t wh) {
    chput(comp->compiled_chunk, j);
    uint32_t i = comp->compiled_chunk->opcount + 4 - wh;
    *((uint32_t*)&comp->compiled_chunk->code[comp->compiled_chunk->opcount]) = i;
    comp->compiled_chunk->opcount += 4;
};

uint32_t emit_jump(struct compiler *comp, uint8_t j) {
    chput(comp->compiled_chunk, j);
    chput(comp->compiled_chunk, 0xff);
    chput(comp->compiled_chunk, 0xff);
    chput(comp->compiled_chunk, 0xff);
    chput(comp->compiled_chunk, 0xff);

    return comp->compiled_chunk->opcount - 4;
};

void confirm_jump(struct compiler *comp, uint32_t offset) {
    uint32_t j = comp->compiled_chunk->opcount - 4 - offset;
    *((uint32_t*)&comp->compiled_chunk->code[offset]) = j;
};

void __start_func_declaration(struct compiler *comp) {
    comp->stack.s[comp->stack.head] = 0;
    comp->stack.head++;
};
void __end_func_declaration(struct compiler *comp) {
    comp->stack.head--;
};
