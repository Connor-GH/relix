#pragma once
#include <elf.h>
#include <stdint.h>
#include <stddef.h>

void
symbol_table_init(const struct Elf64_Shdr *section_headers, uint16_t entsize, uint16_t num);
size_t
symbol_locate(const char *name);
const char *
symbol_resolve(size_t addr, size_t *rela);
