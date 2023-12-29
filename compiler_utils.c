#include "compiler_utils.h"
#include "bytecode.h"
#include "compiler.h"
#include "scanner.h"
#include <bits/pthreadtypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//a monstrocity
void __insert_op(struct compiler *comp, void *blob, size_t blob_size, enum guru_type t, enum opcode op, enum opcode op16)
{
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

void comp_err_report(struct compiler *comp, struct token *t, const char *msg)
{
    if (had_panic(comp)) return;
    fprintf(stderr, "ERROR: line: %d, pos: %d ", t->line, t->pos);

    if (t->type == GURU_EOF) fprintf(stderr, "at the end");
    else fprintf(stderr, "at '%.*s'", t->len, t->lexeme);

    fprintf(stderr, ": %s.\n", msg);
    comp->state |= PANIC_MODE;
    comp->state |= ROTTEN;
}

void advance(struct compiler *comp)
{
    memcpy(&comp->then, &comp->now, sizeof(struct token));

    for (;;) {
        scgett(comp->scanner, &comp->now);
        if (comp->now.line != comp->line) chputl(comp->compiled_chunk), comp->line = comp->now.line;
        if (comp->now.type != GURU_ERROR) break;

        comp_err_report(comp, &comp->now, "bad token");
    }
}

void inline __begin_scope(struct compiler *comp)
{
    comp->scope_depth++;
};

uint16_t __count_locals(struct compiler *comp)
{
    uint16_t i = comp->local_count;
    uint16_t res = 0;
    if (comp->local_count != 0) while (comp->locals[i-1].depth == comp->scope_depth) {
        i--;
        res++;
    }
    return res;
};

uint16_t __end_scope(struct compiler *comp)
{
    uint16_t i = 0;
    if (comp->local_count != 0) while (comp->locals[comp->local_count-1].depth == comp->scope_depth) {
        comp->local_count--;
        i++;
    }
    comp->scope_depth--;
    return i;
};

uint8_t __loc_var_decl(struct compiler *comp)
{
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
    l->is_link = 0;
    memcpy((void*)&l->name, (void*)&comp->then, sizeof(struct token));
    l->depth = 0; // variable is uninited at first
    if (comp->stack.head != 0) {
        l->real = comp->local_count - 1;
        l->ofsetted = comp->stack.s[comp->stack.head-1]++;
        l->compiled_function = comp->stack.head;
        return l->ofsetted + PRIVILEGED_STACK_SLOTS;
    }
    return comp->local_count + PRIVILEGED_STACK_SLOTS - 1;
};

void __loc_const_init(struct compiler *comp)
{
    comp->locals[comp->local_count - 1].depth = comp->scope_depth;
    comp->locals[comp->local_count - 1].is_const = 1;
};
void __loc_var_init(struct compiler *comp)
{
    comp->locals[comp->local_count - 1].depth = comp->scope_depth;
    comp->locals[comp->local_count - 1].is_const = 0;
}

uint32_t set_uint32(struct compiler *comp)
{
    chput(comp->compiled_chunk, 0xff);
    chput(comp->compiled_chunk, 0xff);
    chput(comp->compiled_chunk, 0xff);
    chput(comp->compiled_chunk, 0xff);
    return comp->compiled_chunk->opcount - 4;
};
void patch_uint32(struct compiler *comp, uint32_t ptr, uint32_t patch)
{
    *(uint32_t*)&comp->compiled_chunk->code[ptr] = patch;
};

uint32_t set_arity(struct compiler *comp)
{
    chput(comp->compiled_chunk, 0xff);
    return comp->compiled_chunk->opcount - 1;
};

void patch_arity(struct compiler *comp, uint32_t ptr, uint8_t arity)
{
    comp->compiled_chunk->code[ptr] = arity;
};

enum local_resolved resolve_local(struct compiler *comp, struct local **l)
{
    if (comp->local_count == 0) return RESOLVED_NON_LOCAL;
    uint8_t i = comp->local_count;
    do {
        struct local* local = &comp->locals[--i];
        if (local->depth != 0 && comp->then.len == local->name.len && memcmp((void*) comp->then.lexeme, (void*)local->name.lexeme, local->name.len) == 0) {
            if (comp->stack.head > local->compiled_function && !local->is_link) {
            //INFO: this is a mess
create_closure:
                *comp->closures.closures[comp->closures.closure_count] = local->real+PRIVILEGED_STACK_SLOTS*comp->stack.head;
                local->is_link = ++comp->closures.closure_count;
                if (l != NULL) *l = local;
                return RESOLVED_NEED_CLOSURE;
            } else if (comp->stack.head > local->compiled_function && local->is_link) {
                for (uint8_t i = 0; i < comp->closures.closure_count; i++) {
                    if (*comp->closures.closures[i] == local->real+PRIVILEGED_STACK_SLOTS*comp->stack.head)
                        goto out;
                };
                goto create_closure;
out:
                if (l != NULL) *l = local;
                return RESOLVED_IS_CLOSURE;
            } else if (comp->stack.head != 0) {
                if (l != NULL) *l = local;
                return RESOLVED_NORMAL;
            } else {
                if (l != NULL) *l = local;
                return RESOLVED_NORMAL;
            }
        }
        if (i == 0) break;
    } while (1);
    return RESOLVED_NON_LOCAL;
};

void jump_to(struct compiler *comp, uint8_t j, uint32_t wh)
{
    chput(comp->compiled_chunk, j);
    uint32_t i = comp->compiled_chunk->opcount + 4 - wh;
    *((uint32_t*)&comp->compiled_chunk->code[comp->compiled_chunk->opcount]) = i;
    comp->compiled_chunk->opcount += 4;
};

uint32_t emit_jump(struct compiler *comp, uint8_t j)
{
    chput(comp->compiled_chunk, j);
    chput(comp->compiled_chunk, 0xff);
    chput(comp->compiled_chunk, 0xff);
    chput(comp->compiled_chunk, 0xff);
    chput(comp->compiled_chunk, 0xff);

    return comp->compiled_chunk->opcount - 4;
};

void confirm_jump(struct compiler *comp, uint32_t offset)
{
    uint32_t j = comp->compiled_chunk->opcount - 4 - offset;
    *((uint32_t*)&comp->compiled_chunk->code[offset]) = j;
};

void inline __start_func_declaration(struct compiler *comp)
{
    comp->stack.s[comp->stack.head++] = 0;
};

void inline __end_func_declaration(struct compiler *comp)
{
    comp->stack.head--;
};

struct u16_array_list *init_u16_array_list(uint16_t initial_cap)
{
    if (!initial_cap) initial_cap = 20;

    struct u16_array_list *al = (struct u16_array_list*)malloc(sizeof(struct u16_array_list));
    al->cap = 20;
    al->len = 0;
    al->__elems = (uint16_t*)malloc(sizeof(uint16_t) * initial_cap);

    return al;
};

uint16_t get(struct u16_array_list *al, uint16_t i)
{
    if (i > al->len) return 0;

    return al->__elems[i];
};

uint16_t append(struct u16_array_list *al, uint16_t i)
{
    if (al->len == al->cap)
    {
        al->__elems = realloc(al->__elems, al->cap*=2);
    }

    al->__elems[al->len] = i;
    return al->len++;
};

void free_u16_array_list(struct u16_array_list *al)
{
    al->cap = 0;
    al->len = 0;
    al->__elems = realloc(al->__elems, 0);
    free(al);
};
