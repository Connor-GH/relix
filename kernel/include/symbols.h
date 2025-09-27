#pragma once
#include <elf.h>
#include <stddef.h>
#include <stdint.h>

void symbol_table_init(const Elf64_Shdr *section_headers, uint16_t entsize,
                       uint16_t num);
const char *symbol_resolve(size_t addr, size_t *rela);
