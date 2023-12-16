#include "value.h"
#include "guru.h"
#include "hashmap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t test(const struct __guru_object *val) {
    switch (val->tag) {
        case VAL_VOID: case VAL_NOTHING:
            return 0;
        case VAL_BOOL:
            return val->as.bool;
        case VAL_NUMBER:
            return val->as.numeric > 0.0;
        case BLOB_STRING: case BLOB_BLOB: {
            return val->as.blob->len == 0 ? 0 : 1;
        };
        default:
            return 0;
    }
};

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

struct __blob_header *__alloc_blob(uint64_t s) {
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

    init_hashmap(&pit.int_strings);
    init_hashmap(&pit.globals);
    pit.storage = malloc(__INITIAL_STORAGE_PIT);
};

uint8_t get_global(const void *name, size_t nsize, struct __guru_object *val) {
    struct __blob_header *blob = get_val(&pit.globals, name, nsize);
    if (blob == NULL) return 0;
    if (val == NULL) return 1;
    memcpy((void*)val, blob->__cont, blob->len);

    return 1;
};
void set_global(const void *name, size_t nsize, const struct __guru_object *val) {
    struct __blob_header *blob = __alloc_blob(sizeof(struct __guru_object));
    memcpy(blob->__cont, val, sizeof(struct __guru_object));

    void *alr;
    if ((alr = get_val(&pit.globals, name, nsize)) != NULL)
        ((struct __blob_header *)alr)->__rc--;;
    set_val(&pit.globals, name, nsize, blob);
};

struct __blob_header *get_int_str(const void *key, size_t ksize) {
    struct __blob_header *blob = get_val(&pit.int_strings, key, ksize);
    if (blob == NULL) {
        blob = __alloc_blob(ksize+1);
        memcpy(blob->__cont, (void *) key, ksize);
        *(blob->__cont + ksize) = '\0';
        set_val(&pit.int_strings, key, ksize, blob);
    } else {
        blob->__rc++;
    }

    return blob;
};

struct __blob_header *alloc_guru_callable(uint32_t start, uint8_t arity, uint8_t closures) {
    struct __blob_header *blob = __alloc_blob(sizeof(struct guru_callable)+closures*sizeof(struct __guru_object*));
    ((struct guru_callable*)blob->__cont)->arity = arity;
    ((struct guru_callable*)blob->__cont)->func_begin = start;

    return blob;
};

void obfree(struct __guru_object *o) {
    switch (o->tag) {
      case BLOB_STRING: case BLOB_BLOB: case BLOB_INST:
      case BLOB_VARINT: case BLOB_CALLABLE:
          o->as.blob->__rc--;
      default:
          break;
    }
};
void __gc_collect() {
    uint64_t i = 0;
    struct __blob_header *prev = NULL;
    // printf("______start gc_______\n");
    for (struct __blob_header *h = (struct __blob_header*) pit.__blobs.mem; i != pit.__blobs.cap;) {
        // printf("length: %lu, rc: %lu, i: %lu, content: %.*s, ptr: %x\n", h->len, h->__rc, i, h->len, h->__cont, h);
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
    // printf("______end gc_______\n");
}
void collect_all() {
};
