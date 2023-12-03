#ifndef GURU_CALLABLE
#define GURU_CALLABLE

#include "value.h"
#include <stdint.h>
struct guru_callable {
    uint32_t func_begin;
    uint8_t arity; // you dont actually need more than 255 arguments to a function
};

#endif
