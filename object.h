#ifndef GURU_OBJECT
#define GURU_OBJECT

#include "hashmap.h"
#include "scanner.h"
#include "value.h"
#include <stdint.h>

struct guru_class {
    struct hashmap fields;
};

struct guru_instance {
    struct guru_class *cl;
    struct hashmap fields;
};

void copy_class(struct guru_class *src, struct guru_class *dst);
struct __blob_header *alloc_class();
struct __blob_header *alloc_instance(struct guru_class *cl);

struct __guru_object define_class(const char *nomen, const size_t n_size);

void free_class(struct guru_class *c);
void take_instance(struct guru_instance *i);
void free_instance(struct guru_instance *i);

void set_field(struct guru_instance *cl, void *name, uint64_t namesize, struct __guru_object *o);
struct __guru_object *get_field(struct guru_instance *cl, void *name, uint64_t namesize);

#endif
