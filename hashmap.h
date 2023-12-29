#ifndef GURU_HASHMAP
#define GURU_HASHMAP

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>

#ifndef __HASHMAP_INITIAL_CAPACITY
#define __HASHMAP_INITIAL_CAPACITY 164
#endif

#ifndef __HASHMAP_ACCEPTABLE_LOAD
#define __HASHMAP_ACCEPTABLE_LOAD 0.5
#endif

struct __hm_entry { 
    uint64_t __key_len; 
    const void *key;
    const void *value;
};
#define __hm_entry_size (sizeof(struct __hm_entry))

struct hashmap {
    /*  A generic hashmap;
     *  Map takes no ownership over the values;
     *  Users of the map should interpret the contents of the value
     *  and clean the map out of the values that are freed
     * */
    uint32_t len;
    uint32_t cap;
    uint32_t (*hash)(const void *val, uint64_t size);
    struct __hm_entry *__entries;
};

void iterate_hm_values_keys(struct hashmap *hm, void (*func)(void *val, void *key, size_t key_len));
void iterate_hm_values(struct hashmap *hm, void (*func)(void *val));

void set_val(struct hashmap *hm, const void *key, size_t ksize, const void *val);
void unset_val(struct hashmap *hm, const void *key, size_t ksize);
void *unset_val_r(struct hashmap *hm, const void *key, size_t ksize);
void *get_val(struct hashmap *hm, const void *key, size_t ksize);
struct hashmap *init_hashmap(struct hashmap *hm); //need calloc'ed memory, i think??
void copy_hashmap(struct hashmap *dst, struct hashmap *src);

#define _val_at(hm, i) (hm->__entries + i)
#define __max_load(hm) ((hm)->cap * __HASHMAP_ACCEPTABLE_LOAD)
#define use_hash(hm, f) (hm)->hash = (f)


#endif
