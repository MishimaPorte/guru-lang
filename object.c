#include "object.h"
#include "hashmap.h"
#include "value.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct __blob_header *alloc_class()
{
    struct __blob_header *h = __alloc_blob(sizeof(struct guru_class));
    struct guru_class *c = (struct guru_class *)h->__cont;
    init_hashmap(&c->fields);

    return h;
};

void free_class(struct guru_class *c)
{
    __untake_as_blob(c);
    iterate_hm_values(&c->fields, (void (*)(void *))&obfree);
};

void take_instance(struct guru_instance *i)
{
    __untake_as_blob(i);
    iterate_hm_values(&i->fields, (void (*)(void *))&obtake);
};

void free_instance(struct guru_instance *i)
{
    __untake_as_blob(i);
    iterate_hm_values(&i->fields, (void (*)(void *))&obfree);
};

struct __blob_header *alloc_instance(struct guru_class *cl)
{
    struct __blob_header *h = __alloc_blob(sizeof(struct guru_instance));
    struct guru_instance *c = (struct guru_instance *)h->__cont;
    c->cl = cl;
    init_hashmap(&c->fields);
    __take_as_blob(cl);

    return h;
};

void set_field(struct guru_instance *i, void *name, uint64_t namesize, struct __guru_object *o)
{
    set_val(&i->fields, name, namesize, o);
    obtake(o);
};

static struct hashmap *__dst_hm = NULL;
static void __copy_iterate_hm(void *data, void *key, size_t key_len)
{
    struct __blob_header *new_bh = __alloc_blob(__as_blob(data)->len);
    memcpy(new_bh->__cont, data, new_bh->len);
    set_val(__dst_hm, key, key_len, new_bh->__cont);
};

void copy_class(struct guru_class *src, struct guru_class *dst)
{
    init_hashmap(&dst->fields);
    __dst_hm = &dst->fields;
    iterate_hm_values_keys(&src->fields, &__copy_iterate_hm);
    __dst_hm = NULL;
};

struct __guru_object *get_field(struct guru_instance *i, void *name, uint64_t namesize)
{
    struct __guru_object *o = get_val(&i->fields, name, namesize);
    if (o == NULL) return get_val(&i->cl->fields, name, namesize);
    return o;
};

struct __guru_object define_class(const char *nomen, const size_t n_size)
{
    struct __blob_header *bh = alloc_class();
    struct guru_class *sys = (struct guru_class*)bh->__cont;

    struct __blob_header *name = __alloc_blob(n_size);
    memcpy(name->__cont, nomen, n_size);

    struct __blob_header *name_o = __alloc_blob(sizeof(struct __guru_object));
    ((struct __guru_object *)name_o->__cont)->tag = BLOB_STRING;
    ((struct __guru_object *)name_o->__cont)->as.blob = name;

    set_val(&sys->fields, "_class", sizeof("_class"), name_o->__cont);

    return (struct __guru_object) {
        .tag = BLOB_CLASS,
        .as.blob = bh,
    };
};
