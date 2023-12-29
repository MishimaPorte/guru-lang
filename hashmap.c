#include "hashmap.h"
#include "value.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static uint32_t __vanilla_hash_fnv_1a(const void *val, uint64_t size) {
    if (size == 0) return 0;
    uint32_t hash = 2166136261u;

    for (uint64_t i = 0; i < size; i++) {
        hash ^= ((uint8_t*)val)[i];
        hash *= 16777619;
    }

    return hash;
};
void __rehash(struct hashmap *hm, uint64_t new_cap) {
    struct __hm_entry *old = hm->__entries;
    uint64_t c = hm->cap;

    hm->__entries = calloc(new_cap, __hm_entry_size);
    hm->cap = new_cap;
    for (uint64_t i = 0; i != c; i++) {
        struct __hm_entry e = old[i];
        if (e.key == NULL) continue;
        set_val(hm, e.key, e.__key_len, e.value);
        hm->len++;
    }
    free(old);
};

void *unset_val_r(struct hashmap *hm, const void *key, size_t ksize)
{
    for (uint64_t i = hm->hash(key, ksize) % hm->cap;; i = (i+1)%hm->cap) {
        if (_val_at(hm, i)->key == NULL && (hm->__entries + i)->__key_len != 1) {
            return NULL;
        } else if (_val_at(hm, i)->key == NULL && (hm->__entries + i)->__key_len == 1) {
            continue;
        } else if (memcmp(_val_at(hm, i)->key, key, ksize) == 0) {
            _val_at(hm, i)->key = NULL;
            _val_at(hm, i)->__key_len = 1;
            return _val_at(hm, i);
        };
    };
};

void iterate_hm_values(struct hashmap *hm, void (*func)(void *val))
{
    uint64_t len_counter = 0;
    for (uint64_t i = 0; i < hm->cap && len_counter < hm->cap; i++)
        if (_val_at(hm, i)->key != NULL) {
            func((void *)_val_at(hm, i)->value);
            len_counter++;
        }
    return;
};

void iterate_hm_values_keys(struct hashmap *hm, void (*func)(void *val, void *key, size_t key_len))
{
    uint64_t len_counter = 0;
    for (uint64_t i = 0; i < hm->cap && len_counter < hm->cap; i++)
        if (_val_at(hm, i)->key != NULL) {
            func((void *)_val_at(hm, i)->value, (void *)_val_at(hm, i)->key, _val_at(hm, i)->__key_len);
            len_counter++;
        }
            
    return;
};

void unset_val(struct hashmap *hm, const void *key, size_t ksize)
{
    for (uint64_t i = hm->hash(key, ksize) % hm->cap;; i = (i+1)%hm->cap) {
        if (_val_at(hm, i)->key == NULL && (hm->__entries + i)->__key_len != 1) {
            return;
        } else if (_val_at(hm, i)->key == NULL && (hm->__entries + i)->__key_len == 1) {
            continue;
        } else if (memcmp(_val_at(hm, i)->key, key, ksize) == 0) {
            _val_at(hm, i)->key = NULL;
            _val_at(hm, i)->__key_len = 1;
            return;
        };
    };
};

void set_val(struct hashmap *hm, const void *key, size_t ksize, const void *val)
{
    for (uint64_t i = hm->hash(key, ksize) % hm->cap;; i = (i+1)%hm->cap) {
        if ((hm->__entries + i)->key == NULL) {
            if (++hm->len >= __max_load(hm)) {
                __rehash(hm, hm->cap*2);
            }
            _val_at(hm, i)->value = val;
            _val_at(hm, i)->key = key;
            _val_at(hm, i)->__key_len = ksize;
            return;
        } else if (memcmp(_val_at(hm, i)->key, key, ksize) == 0) {
            _val_at(hm, i)->value = val;
            return;
        } else if (hm->len >= __max_load(hm)) __rehash(hm, hm->cap*2);
    };
};

void *get_val(struct hashmap *hm, const void *key, size_t ksize)
{
    for (
            uint64_t i = hm->hash(key, ksize) % hm->cap;;
            i = (i+1)%hm->cap
        ) {
        if (_val_at(hm, i)->key == NULL && _val_at(hm, i)->__key_len != 1) {
            return NULL;
        } else if (_val_at(hm, i)->key == NULL && (hm->__entries + i)->__key_len == 1) {
            continue;
        } else if (memcmp(_val_at(hm, i)->key, key, ksize) == 0) {
            return (void *) _val_at(hm, i)->value;
        };
    };
};

void copy_hashmap(struct hashmap *dst, struct hashmap *src)
{
    memcpy(dst, src, sizeof(struct hashmap));
    dst->__entries = calloc(src->cap, sizeof(struct __hm_entry));
    memcpy(dst->__entries, src->__entries, sizeof(struct __hm_entry)*src->cap);
};

struct hashmap *init_hashmap(struct hashmap *hm)
{
    hm->len = 0;
    hm->cap = __HASHMAP_INITIAL_CAPACITY;
    hm->hash = &__vanilla_hash_fnv_1a;
    hm->__entries = calloc(__HASHMAP_INITIAL_CAPACITY, __hm_entry_size);
    if (hm->__entries == NULL) {
        printf("guru: could not allocate enougm memory");
        exit(1);
    }

    return hm;
};
