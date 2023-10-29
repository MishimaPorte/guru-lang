#include "hashmap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static uint32_t __vanilla_hash_fnv_1a(void *val, uint64_t size) {
    uint32_t hash = 2166136261u;

    for (uint64_t i = 0; i < size; i++) {
        hash ^= ((uint8_t*)val)[i];
        hash *= 16777619;
    }

    return hash;
};

void unset_val(struct hashmap *hm, void *key, size_t ksize) {
    for (uint64_t i = hm->hash(key, ksize) % hm->cap;; i = (i+1)%hm->cap) {
        if (_val_at(hm, i)->key == NULL) {
            _val_at(hm, i)->key = NULL;
            return;
        } else if (memcmp(_val_at(hm, i)->key, key, ksize) == 0) {
            return;
        };
    };
};
void set_val(struct hashmap *hm, void *key, size_t ksize, void *val) {
    for (uint64_t i = hm->hash(key, ksize) % hm->cap;; i = (i+1)%hm->cap) {
        if ((hm->__entries + i)->key == NULL) {
            if (++hm->len > __max_load(hm)) {
                hm->__entries = realloc(hm->__entries, hm->cap*2);
                memset(hm->__entries + hm->cap - 1, 0, hm->cap);
                hm->cap*=2;
            }
            _val_at(hm, i)->value = val;
            _val_at(hm, i)->key = key;
            _val_at(hm, i)->__key_len = ksize;
            return;
        } else if (memcmp(_val_at(hm, i)->key, key, ksize) == 0) {
            _val_at(hm, i)->value = val;
            return;
        };
    };
};
void *get_val(struct hashmap *hm, void *key, size_t ksize) {
    for (uint64_t i = hm->hash(key, ksize) % hm->cap;; i = (i+1)%hm->cap) {
        if (_val_at(hm, i)->key == NULL) {
            return NULL;
        } else if (memcmp(_val_at(hm, i)->key, key, ksize) == 0) {
            return _val_at(hm, i)->value;
        };
    };
};
struct hashmap *init_hashmap(void *mem, size_t memsize) {
    ((struct hashmap *) mem)->len = 0;
    ((struct hashmap *) mem)->cap = __HASHMAP_INITIAL_CAPACITY;
    ((struct hashmap *) mem)->hash = &__vanilla_hash_fnv_1a;
    ((struct hashmap *) mem)->__entries = calloc(__HASHMAP_INITIAL_CAPACITY, __hm_entry_size);


    return mem;
};
