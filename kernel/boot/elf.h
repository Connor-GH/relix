#pragma once
#include <stdint.h>
// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU // "\x7FELF" in little endian

// File header
struct elfhdr {
	uint32_t magic; // must equal ELF_MAGIC
	uint8_t elf[12];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uintptr_t entry;
	uintptr_t phoff;
	uintptr_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
};

// Program section header
struct proghdr {
	uint32_t type;
	uint32_t flags;
	uintptr_t off;
	uintptr_t vaddr;
	uintptr_t paddr;
	uintptr_t filesz;
	uintptr_t memsz;
	uintptr_t align;
};

#ifdef X86_64
// ELF 1.2 figure 1-8.
struct elf64_shdr {
	uint32_t sh_name;
	uint32_t sh_type;
	uint64_t sh_flags;
	uintptr_t sh_addr;
	uintptr_t sh_offset;
	uint64_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint64_t sh_addralign;
	uint64_t sh_entsize;
};

struct elf64_symhdr {
	uint32_t name;
	uint8_t info;
	uint8_t other;
	uint16_t st_shndx;
	uint64_t value;
	uint64_t size;
};
#endif

enum {
	STT_NOTYPE = 0,
	STT_OBJECT = 1,
	STT_FUNC = 2,
	STT_SECTION = 3,
	STT_FILE = 4,
	STT_LOPROC = 13,
	STT_HIPROC = 15,
};

// 1-16
enum {
	STB_LOCAL = 0,
	STB_GLOBAL = 1,
	STB_WEAK = 2,
	STB_LOPROC = 13,
	STB_HIPROC = 15,

};

// Values for Proghdr type
#define ELF_PROG_LOAD 1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC 1
#define ELF_PROG_FLAG_WRITE 2
#define ELF_PROG_FLAG_READ 4
enum {
	SHT_NULL = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB = 2,
	SHT_STRTAB = 3,
	SHT_RELA = 4,
	SHT_HASH = 5,
	SHT_DYNAMIC = 6,
	SHT_NOTE = 7,
	SHT_NOBITS = 8,
	SHT_REL = 9,
	SHT_SHLIB = 10,
	SHT_DYNSYM = 11,
	SHT_LOPROC = 0x70000000,
	SHT_HIPROC = 0x7fffffff,
};
enum {
	SHF_WRITE = 1,
	SHF_ALLOC = 2,
	SHF_EXECINSTR = 4,
#define SHF_MASKPROC 0xf0000000
};

// TIS ELF spec 1.2
enum {
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	ET_CORE = 4,
	ET_LOPROC = 0xff00,
	ET_HIPROC = 0xffff,
};

enum {
	ELFCLASSNONE = 0,
	ELFCLASS32 = 1,
	ELFCLASS64 = 2,
};
