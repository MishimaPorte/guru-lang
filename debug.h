#ifndef GURU_DISASSEMBLER
#define GURU_DISASSEMBLER

#include "bytecode.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>

static void __d_simple_instruction(const struct chunk *c, const char *name, const uint8_t *ip);

static void __d_one_u8_instruction(const struct chunk *c, const char *name, const uint8_t *ip);

static void __d_one_u16_instruction(const struct chunk *c, const char *name, const uint8_t *ip);

static void __d_one_u32_instruction(const struct chunk *c, const char *name, const uint8_t *ip);

uint8_t *disassemble_chunk(const struct chunk *c);

#endif
