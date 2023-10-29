#ifndef GURU_HASHMAP
#define GURU_HASHMAP

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef __HASHMAP_INITIAL_CAPACITY
#define __HASHMAP_INITIAL_CAPACITY 4
#endif

#ifndef __HASHMAP_ACCEPTABLE_LOAD
#define __HASHMAP_ACCEPTABLE_LOAD 0.5
#endif

struct __hm_entry { 
    uint64_t __key_len; 
    void *key;
    void *value;
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
    uint32_t (*hash)(void *val, uint64_t size);
    struct __hm_entry *__entries;
};

void set_val(struct hashmap *hm, void *key, size_t ksize, void *val);
void unset_val(struct hashmap *hm, void *key, size_t ksize);
void *get_val(struct hashmap *hm, void *key, size_t ksize);
struct hashmap *init_hashmap(void *mem, size_t memsize); //need calloc'ed memory, i think??

#define _val_at(hm, i) (hm->__entries + i)
#define __max_load(hm) ((hm)->cap * __HASHMAP_ACCEPTABLE_LOAD)
#define use_hash(hm, f) (hm)->hash = (f)


#endif
