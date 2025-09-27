/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "console.h"
#include "kalloc.h"
#include "kernel_assert.h"

#include <elf.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
 * There is no P2V because this is so early
 * that virtual memory has not been setup yet.
 */

#define log_no_symbols(...)                                           \
	({                                                                  \
		uart_printf("Note: symbols will not be available: " __VA_ARGS__); \
		uart_printf("\n");                                                \
	})

typedef struct elf_symbol {
	uintptr_t addr;
	size_t size;
	const char *name;
} elf_symbol_t;

/*
 * Static variables for
 * information that we need when we
 * print kernel backtraces.
 */
static int s_symbol_num = 0;
static size_t s_symlen_max = 0;
static elf_symbol_t *s_symbols = NULL;

void
symbol_table_init(const Elf64_Shdr *sections, uint16_t entsize, uint16_t num)
{
	if (sections == NULL) {
		log_no_symbols("sections == NULL");
		return;
	}

	if (entsize != sizeof(Elf64_Shdr)) {
		log_no_symbols("%d != sizeof(Elf64_Shdr)", entsize);
		return;
	}

	size_t symname_len = 0;

	// Starts at 1 because the first section is NULL.
	for (uint16_t i = 1; i < num; i++) {
		const Elf64_Shdr *sh = &sections[i];

		if (SHT_SYMTAB != sh->sh_type && SHT_DYNSYM != sh->sh_type) {
			continue;
		}

		if (SHN_UNDEF == sh->sh_link || sh->sh_link >= num) {
			continue;
		}

		const Elf64_Shdr *link = &sections[sh->sh_link];

		if (SHT_STRTAB != link->sh_type) {
			continue;
		}

		size_t symbol_num = sh->sh_size / sh->sh_entsize;
		Elf64_Sym *symbols = (Elf64_Sym *)sh->sh_addr;
		size_t strtab_len = link->sh_size;
		const char *strtab = (const char *)link->sh_addr;

		for (size_t j = 0; j < symbol_num; j++) {
			Elf64_Sym *st = &symbols[j];

			uint32_t strtab_idx = st->st_name;

			if (strtab_idx >= strtab_len) {
				continue;
			}

			const char *symstr = &strtab[strtab_idx];

			if (ELF64_ST_TYPE(st->st_info) != STT_FUNC) {
				continue;
			}

			// Increment symbol count. Used for our static symbols
			// pool, needs this info to allocate memory.
			s_symbol_num++;
			size_t symlen = strnlen(symstr, strtab_len - strtab_idx) + 1;
			// Get the upper bound on max string length for the symbols.
			// This is used for strncmp when locating symbols.
			if (symlen > s_symlen_max) {
				s_symlen_max = symlen;
			}
			symname_len += symlen;
		}
	}

	char *symname = kmalloc(symname_len);
	if (symname == NULL) {
		log_no_symbols("symname == NULL");
		return;
	}

	s_symbols = kmalloc(s_symbol_num * sizeof(elf_symbol_t));
	if (s_symbols == NULL) {
		log_no_symbols("s_symbols == NULL");
		return;
	}

	size_t symname_idx = 0;
	int symbol_idx = 0;

	for (uint32_t i = 1; i < num; i++) {
		const Elf64_Shdr *sh = &sections[i];

		if (SHT_SYMTAB != sh->sh_type && SHT_DYNSYM != sh->sh_type) {
			continue;
		}

		if (SHN_UNDEF == sh->sh_link || sh->sh_link >= num) {
			continue;
		}

		const Elf64_Shdr *link = &sections[sh->sh_link];

		if (SHT_STRTAB != link->sh_type) {
			continue;
		}

		size_t symbol_num = sh->sh_size / sh->sh_entsize;

		Elf64_Sym *symbols = (Elf64_Sym *)sh->sh_addr;
		size_t strtab_len = link->sh_size;

		const char *strtab = (const char *)link->sh_addr;

		for (size_t j = 0; j < symbol_num; j++) {
			Elf64_Sym *st = &symbols[j];

			uint32_t strtab_idx = st->st_name;
			if (strtab_idx >= strtab_len) {
				continue;
			}

			if (ELF64_ST_TYPE(st->st_info) != STT_FUNC) {
				continue;
			}

			char *symdst = &symname[symname_idx];
			const char *symstr = &strtab[strtab_idx];
			strncpy(symdst, symstr, symname_len - symname_idx);
			symname_idx += strnlen(symdst, symname_len - symname_idx) + 1;

			elf_symbol_t *sym = &s_symbols[symbol_idx++];
			sym->name = symdst;
			sym->addr = symbols[j].st_value;
			sym->size = symbols[j].st_size;
		}
	}

	kernel_assert(symname_idx == symname_len);
	kernel_assert(symbol_idx == s_symbol_num);
}

// INVARIANT: symbol_table_init is called before this.
const char *
symbol_resolve(size_t addr, size_t *rela)
{
	if (s_symbol_num == 0 || s_symlen_max == 0 || s_symbols == NULL) {
		goto error_condition;
	}
	for (int i = 0; i < s_symbol_num; i++) {
		size_t start = s_symbols[i].addr;
		size_t end = start + s_symbols[i].size;
		if (addr < start || addr >= end) {
			continue;
		}

		if (rela) {
			*rela = addr - start;
		}
		return s_symbols[i].name;
	}

	if (rela) {
		*rela = addr;
	}
error_condition:
	return NULL;
}
