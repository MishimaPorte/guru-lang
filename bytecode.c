#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bytecode.h"
#include "guru.h"
#include "vm.h"
void chfree(struct chunk *c) {
    free(c->code);
    c->code = NULL;
    c->opcount = 0;
    c->capacity = 0;

    for (struct lninfo *i = c->line, *n = NULL; i != NULL; i = n) {
        n = i->next;
        free(i);
    }
    c->crnt_ln = NULL;
    c->line = NULL;

    free(c->consts.vals);
    c->consts.vals = NULL;
    c->consts.capacity = 0;
    c->consts.count = 0;
}
void chunputif(struct chunk *c, enum opcode o, enum opcode o2) {
    if (c->code[c->opcount-1] == o) c->opcount--;
    else chput(c, (uint8_t)o2);
};

void chputn(struct chunk *c, void *val, size_t size) {
    if (c->capacity <= c->opcount + size) {
        c->capacity = c->capacity < 8 ? 8 : c->capacity*2;
        c->code = realloc(c->code, c->capacity);
        if (c->code == NULL) error("not enough memory to countinue the rambling");
    }
    memcpy(&c->code[c->opcount], val, size);
    c->opcount += size;
}
void chput(struct chunk *c, uint8_t opcode) {
    if (c->capacity <= c->opcount) {
        c->capacity = c->capacity < 8 ? 8 : c->capacity*2;
        c->code = realloc(c->code, c->capacity);
        if (c->code == NULL) error("not enough memory to countinue the rambling");
    }
    c->code[c->opcount++] = opcode;
}

struct chunk *challoc() {
    struct chunk *c = calloc(1, sizeof(struct chunk));

    if (c == NULL) error("not enough memory");
    c->line = c->crnt_ln = calloc(1, sizeof(struct lninfo));
    if (c->line == NULL) error("not enough memory");

    return c;
} 
void chputl(struct chunk *c) {
    struct lninfo *i = calloc(1, sizeof(struct lninfo));
    if (i == NULL) perror("was unable to allocate things");

    i->line = (c->crnt_ln->line) + 1;
    c->crnt_ln->opend = c->opcount;
    i->opstart = c->opcount;
    c->crnt_ln->next = i;
    c->crnt_ln = i;
};
