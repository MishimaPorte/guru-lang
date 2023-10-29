#include "value.h"
#include "guru.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void __free_const_array(struct __consts *c) {
    free(c->vals);
    c->count = 0;
    c->capacity = 0;
}
uint16_t __add_const(struct __consts *c, void *val, size_t s, enum guru_type tt) {
    if (c->capacity <= c->count) {
        c->capacity = c->capacity < 8 ? 8 : c->capacity*2;
        c->vals = realloc(c->vals, c->capacity * sizeof(struct __guru_object));
        if (c->vals == NULL) error("not enough memory to countinue the rambling");
    }
    memcpy(&c->vals[c->count].as, val, s);
    c->vals[c->count].tag = tt;

    return c->count++;
}

// GARBAGE COLLECTION-RELATED STUFF
struct __blob_header *__alloc_blob(uint64_t s) {
    // TODO: implement occasional garbage collection
__start:
    if (__ffb->len >= s+sizeof(struct __blob_header) && __ffb->__rc == 0) {
        struct __blob_header *h = __ffb;
        uint64_t old_s = h->len;
        h->len = s;
        h->__rc = 1;

        __next_blob(h)->__rc = 0;
        __next_blob(h)->len = old_s - s - sizeof(struct __blob_header);
        if (__next_blob(h) < __ffb) {
            pit.__blobs.__first_free_offset = (void*)__mem(blobs) - (void*)__next_blob(h);
        }
        return h;
    }

    struct __blob_header *h = __ffb;
    for (;;) {
        if (h->len >= s + sizeof(struct __blob_header) && h->__rc == 0) {
            uint64_t old_s = h->len;
            h->__rc = 1;
            h->len = s;
            __next_blob(h)->__rc = 0;
            __next_blob(h)->len = old_s - s - sizeof(struct __blob_header);
            return h;
        }
        if (pit.__blobs.cap - ((void*)h - __mem(blobs)) < (sizeof(struct __blob_header) + s)) {
            __gc_collect();
            goto __start;
        } else {
            h = __next_blob(h);
        }
    }
};
struct __blob_header *__realloc_blob(struct __blob_header *b, size_t s) {
    if (__next_blob(b)->__rc == 0 && __next_blob(b)->len >= (s-b->len+sizeof(struct __guru_object))) {
        uint64_t i = __next_blob(b)->len - (s-b->len);
        b->len = s;
        __next_blob(b)->len = i;
        __next_blob(b)->__rc = 0;
        if (__next_blob(b) < __ffb) {
            pit.__blobs.__first_free_offset = (void*)__mem(blobs) - (void*)__next_blob(b);
        }
        return b;
    }
    struct __blob_header *h = __alloc_blob(s);
    memcpy(h->__cont, b->__cont, b->len);
    b->__rc = 0;
    if (b < __ffb) {
        pit.__blobs.__first_free_offset = (void*)__mem(blobs) - (void*)b;
    }
    return h;
}; // s - new size

void init_pit() {
    /*  Initial memory pool allocation;
     * */
    pit.__blobs.mem = malloc(__INITIAL_BLOB_PIT);
    pit.__blobs.cap = __INITIAL_BLOB_PIT;
    pit.__blobs.__first_free_offset = 0;
    __ffb->__rc = 0;
    __ffb->len = __INITIAL_BLOB_PIT - sizeof(struct __blob_header);
};

void __gc_collect() {
    uint64_t i = 0;
    struct __blob_header *prev = NULL;
    for (struct __blob_header *h = (struct __blob_header*) pit.__blobs.mem; i != pit.__blobs.cap;) {
        printf("length: %lu, rc: %lu, i: %lu, content: %.*s, ptr: %x\n", h->len, h->__rc, i, h->len, h->__cont, h);
        if (h->__rc == 0) {
            if (prev != NULL) {
                prev->len += h->len;
                prev->len += sizeof(struct __blob_header);
                i += (sizeof(struct __blob_header) + h->len);
                h = __next_blob(h);
                continue;
            }
            if (__ffb > h){
                pit.__blobs.__first_free_offset = (void*)h - pit.__blobs.mem;
            }
            prev = h;
        } else {
            prev = NULL;
        }
        i += (sizeof(struct __blob_header) + h->len);
        h = __next_blob(h);
    }
}
void collect_all() {
};
