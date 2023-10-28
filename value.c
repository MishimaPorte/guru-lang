#include "value.h"
#include "guru.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__go_malloc_for_type(VAL_BOOL, bool)
__go_malloc_for_type(VAL_NUMBER, numeric)

void gdafree(struct gobj_a *c) {
    free(c->vals);
    c->count = 0;
    c->capacity = 0;
}
uint16_t gdaput(struct gobj_a *c, struct __guru_object *val) {
    if (c->capacity <= c->count) {
        c->capacity = c->capacity < 8 ? 8 : c->capacity*2;
        c->vals = realloc(c->vals, c->capacity * sizeof(struct __guru_object));
        if (c->vals == NULL) error("not enough memory to countinue the rambling");
    }
    c->vals[c->count] = val;

    return c->count++;
}

struct __guru_object *__alloc_BLOB_STRING(void *val, size_t n) {
    struct __blob_header *blob = __alloc_blob(n);
    blob->len = n;
    memcpy(blob->__cont, val, n);


    struct __guru_object *o = __alloc_go_header(BLOB_STRING);
    o->as.blob = blob;

    return o;
};

// GARBAGE COLLECTION-RELATED STUFF

struct __guru_object *__alloc_go_header(enum guru_type tt) {
    // TODO: implement occasional garbage collection
    struct __guru_object *o = mem(objects) + pit.__objects.ptr++;
    o->tag = tt;
    o->__rc = 1;
    ((struct __guru_object *) mem(objects) + pit.__objects.ptr)->tag = __PIT_OBJECT_END;

    return o;
};
struct __blob_header *__alloc_blob(uint64_t s) {
    // TODO: implement occasional garbage collection

    if (__ffb->len >= s+sizeof(struct __blob_header) && __ffb->__rc == 0) {
        struct __blob_header *h = __ffb;
        uint64_t old_s = h->len;
        h->len = s;
        h->__rc = 1;

        pit.__blobs.__first_free_offset += h->len; 
        pit.__blobs.__first_free_offset += sizeof(struct __blob_header); 
        __ffb->__rc = 0;
        __ffb->len = old_s - s - sizeof(struct __blob_header);
        return h;
    }

    struct __blob_header *h = __ffb;
    for (;;) {
        if (h->len >= s + sizeof(struct __blob_header) && h->__rc == 0) {
            uint64_t old_s = h->len;
            h->__rc = 1;
            h->len = s;
            __next_blob(h)->__rc = 0;
            __next_blob(h)->len = old_s - s;
            return h;
        }
        if (((void*)h - mem(blobs)) < (sizeof(struct __blob_header) + s)) {
            return NULL;
        }
    }
};
struct __blob_header *__realloc_blob(struct __blob_header *b, size_t s) {
    if (__next_blob(b)->__rc == 0 && __next_blob(b)->len >= (s-b->len)) {
        uint64_t i = s-b->len;
        b->len = s;
        __next_blob(b)->len = i;
        __next_blob(b)->__rc = 0;
        return b;
    }
    struct __blob_header *h = __alloc_blob(s);
    memcpy(h->__cont, b->__cont, b->len);
    b->__rc = 0;
    return h;
}; // s - new size

void init_pit() {
    /*  Initial memory pool allocation;
     * */
    pit.__blobs.mem = malloc(__INITIAL_BLOB_PIT);
    pit.__blobs.cap = __INITIAL_BLOB_PIT;
    pit.__blobs.ptr = 0;
    pit.__blobs.__first_free_offset = 0;
    __ffb->__rc = 0;
    __ffb->len = __INITIAL_BLOB_PIT - sizeof(struct __blob_header);
    pit.__objects.mem = malloc(__INITIAL_BLOB_PIT * (sizeof(struct __guru_object)));
    pit.__objects.cap = __INITIAL_BLOB_PIT * (sizeof(struct __guru_object));
    pit.__objects.ptr = 0;
};

void __gc_collect() {
    struct __guru_object *__tail = (struct __guru_object *) pit.__objects.mem;
    for (struct __guru_object *o = (struct __guru_object *) pit.__objects.mem; o->tag != __PIT_OBJECT_END; o++) {
        if (o->__rc != 0) *__tail++ = *o;
    }
}
void collect_all() {
    // geniuos
    ((struct __guru_object *) pit.__objects.mem)->tag = __PIT_OBJECT_END;
};
