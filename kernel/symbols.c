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

static int
compare_symbols(const void *c1, const void *c2)
{
	struct Elf64_Sym *sym1 = (struct Elf64_Sym *)c1;
	struct Elf64_Sym *sym2 = (struct Elf64_Sym *)c2;
	return sym1->st_value - sym2->st_value;
}

void
symbol_table_init(const struct Elf64_Shdr *sections, uint16_t entsize,
                  uint16_t num)
{
	if (sections == NULL) {
		log_no_symbols("sections == NULL");
		return;
	}

	if (entsize != sizeof(struct Elf64_Shdr)) {
		log_no_symbols("%d != sizeof(struct Elf64_Shdr)", entsize);
		return;
	}

	size_t symname_len = 0;

	// Starts at 1 because the first section is NULL.
	for (uint16_t i = 1; i < num; i++) {
		struct Elf64_Shdr sh;

		memcpy(&sh, &sections[i], sizeof(struct Elf64_Shdr));

		if ((SHT_SYMTAB != sh.sh_type) && (SHT_DYNSYM != sh.sh_type)) {
			continue;
		}

		if ((SHN_UNDEF == sh.sh_link) || (sh.sh_link >= num)) {
			continue;
		}

		struct Elf64_Shdr link;

		memcpy(&link, &sections[sh.sh_link], sizeof(struct Elf64_Shdr));

		if (SHT_STRTAB != link.sh_type) {
			continue;
		}

		size_t symbol_num = sh.sh_size / sh.sh_entsize;
		struct Elf64_Sym *symbols = (struct Elf64_Sym *)sh.sh_addr;
		size_t strtab_len = link.sh_size;
		const char *strtab = (const char *)link.sh_addr;

		for (size_t j = 0; j < symbol_num; j++) {
			struct Elf64_Sym st;
			memcpy(&st, &symbols[j], sizeof(struct Elf64_Sym));

			uint32_t strtab_idx = st.st_name;

			if (strtab_idx >= strtab_len) {
				continue;
			}

			const char *symstr = &strtab[strtab_idx];

			if (ELF64_ST_TYPE(st.st_info) != STT_FUNC) {
				continue;
			}

			s_symbol_num++;
			size_t symlen = strnlen(symstr, strtab_len - strtab_idx) + 1;
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
		log_no_symbols("symname == NULL");
		return;
	}

	size_t symname_idx = 0;
	int symbol_idx = 0;

	for (uint32_t i = 1; i < num; i++) {
		struct Elf64_Shdr sh;

		memcpy(&sh, &sections[i], sizeof(struct Elf64_Shdr));

		if ((SHT_SYMTAB != sh.sh_type) && (SHT_DYNSYM != sh.sh_type)) {
			continue;
		}

		if ((SHN_UNDEF == sh.sh_link) || (sh.sh_link >= num)) {
			continue;
		}

		struct Elf64_Shdr link;
		memcpy(&link, &sections[sh.sh_link], sizeof(struct Elf64_Shdr));

		if (SHT_STRTAB != link.sh_type) {
			continue;
		}

		size_t symbol_num = sh.sh_size / sh.sh_entsize;

		struct Elf64_Sym *symbols = (struct Elf64_Sym *)sh.sh_addr;
		size_t strtab_len = link.sh_size;

		const char *strtab = (const char *)link.sh_addr;

		for (size_t j = 1; j < symbol_num; ++j) {
			struct Elf64_Sym st;
			memcpy(&st, &symbols[j], sizeof(struct Elf64_Sym));

			uint32_t strtab_idx = st.st_name;
			if (strtab_idx >= strtab_len) {
				continue;
			}

			if (ELF64_ST_TYPE(st.st_info) != STT_FUNC) {
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
size_t
symbol_locate(const char *name)
{
	if (s_symbol_num == 0 || s_symlen_max == 0 || s_symbols == NULL) {
		goto error_condition;
	}

	for (int i = 0; i < s_symbol_num; ++i) {
		if (strncmp(name, s_symbols[i].name, s_symlen_max)) {
			continue;
		}
		return s_symbols[i].addr;
	}

error_condition:
	return 0;
}

// INVARIANT: symbol_table_init is called before this.
const char *
symbol_resolve(size_t addr, size_t *rela)
{
	if (s_symbol_num == 0 || s_symlen_max == 0 || s_symbols == NULL) {
		goto error_condition;
	}
	for (int i = 0; i < s_symbol_num; ++i) {
		size_t start = s_symbols[i].addr;
		size_t end = start + s_symbols[i].size;
		if ((addr < start) || (addr >= end)) {
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
