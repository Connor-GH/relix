#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>

void
qsort(void *base, size_t nmemb, size_t size,
			int (*compar)(const void *, const void *))
{
	/* TODO no implementation for now */
}

void
print_section_headers(int fd, struct Elf64_Ehdr *header,
											struct Elf64_Shdr *section_headers, const char *strtab)
{
	printf("Section Headers:\n");
	printf("%-30s %-10s %-10s\n", "Section Name", "Type", "Addr");
	for (size_t i = 0; i < header->e_shnum; i++) {
		printf("%-30s %-10d %-10lx\n",
			&strtab[section_headers[i].sh_name], // Section Name from string table
			section_headers[i].sh_type, section_headers[i].sh_addr);
	}
}

int
compare(const void *c1, const void *c2)
{
	struct Elf64_Sym *sym1 = (struct Elf64_Sym *)c1;
	struct Elf64_Sym *sym2 = (struct Elf64_Sym *)c2;
	return sym1->st_value - sym2->st_value;
}
void
print_symbol_table(int fd, struct Elf64_Ehdr *header,
									 struct Elf64_Shdr *section_headers, const char *strtab)
{
	for (size_t i = 0; i < header->e_shnum; i++) {
		// Look for symbol table sections
		if (section_headers[i].sh_type == SHT_SYMTAB ||
				section_headers[i].sh_type == SHT_DYNSYM) {
			printf("Symbol Table (%s):\n", &strtab[section_headers[i].sh_name]);

			struct Elf64_Shdr *symbol_table_header = &section_headers[i];
			struct Elf64_Sym *symbols = malloc(symbol_table_header->sh_size);
			if (!symbols) {
				perror("malloc");
				continue;
			}

			// Read the symbol table
			if (lseek(fd, symbol_table_header->sh_offset, SEEK_SET) == -1) {
				perror("lseek");
				free(symbols);
				continue;
			}

			if (read(fd, symbols, symbol_table_header->sh_size) !=
					symbol_table_header->sh_size) {
				perror("read");
				free(symbols);
				continue;
			}

			// Get the associated string table index
			int strtab_index = symbol_table_header->sh_link;
			struct Elf64_Shdr *strtab_header = &section_headers[strtab_index];
			char *symbol_strtab = malloc(strtab_header->sh_size);
			if (!symbol_strtab) {
				perror("malloc");
				free(symbols);
				continue;
			}

			// Read the symbol string table
			if (lseek(fd, strtab_header->sh_offset, SEEK_SET) == -1) {
				perror("lseek");
				free(symbols);
				free(symbol_strtab);
				continue;
			}

			if (read(fd, symbol_strtab, strtab_header->sh_size) !=
					strtab_header->sh_size) {
				perror("read");
				free(symbols);
				free(symbol_strtab);
				continue;
			}

			printf("%-30s %-10s %-10s\n", "Function Name", "Addr", "Size");

			qsort(symbols, symbol_table_header->sh_size / sizeof(struct Elf64_Sym),
						sizeof(struct Elf64_Sym), compare);
			// Iterate through the symbols
			for (size_t j = 0;
					 j < symbol_table_header->sh_size / sizeof(struct Elf64_Sym); j++) {
				// Only print function symbols (function type: STT_FUNC)
				if (ELF64_ST_TYPE(symbols[j].st_info) == STT_FUNC) {
					printf("%-30s %#-10lx %#-10lx\n",
								 &symbol_strtab[symbols[j].st_name], // Function Name
								 symbols[j].st_value, // Value (address)
								 symbols[j].st_size); // Size
				}
			}

			free(symbol_strtab);
			free(symbols);
		}
	}
}

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *filename = argv[1];
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return EXIT_FAILURE;
	}

	struct Elf64_Ehdr header;
	if (read(fd, &header, sizeof(header)) != sizeof(header)) {
		perror("read");
		close(fd);
		return EXIT_FAILURE;
	}

	if (strncmp((const char *)header.e_ident, ELF_MAGIC, 4) != 0) {
		fprintf(stderr, "Not a valid ELF file\n");
		close(fd);
		return EXIT_FAILURE;
	}

	struct Elf64_Shdr *section_headers =
		malloc(header.e_shentsize * header.e_shnum);
	if (!section_headers) {
		perror("malloc");
		close(fd);
		return EXIT_FAILURE;
	}

	if (lseek(fd, header.e_shoff, SEEK_SET) == -1) {
		perror("lseek");
		free(section_headers);
		close(fd);
		return EXIT_FAILURE;
	}

	if (read(fd, section_headers, header.e_shentsize * header.e_shnum) !=
			header.e_shentsize * header.e_shnum) {
		perror("read");
		free(section_headers);
		close(fd);
		return EXIT_FAILURE;
	}

	// Locate the string table section
	const char *strtab = NULL;
	if (header.e_shstrndx != SHN_UNDEF && header.e_shstrndx < header.e_shnum) {
		struct Elf64_Shdr *strtab_header = &section_headers[header.e_shstrndx];
		strtab = malloc(strtab_header->sh_size);
		if (!strtab) {
			perror("malloc");
			free(section_headers);
			close(fd);
			return EXIT_FAILURE;
		}

		// Seek to the string table
		if (lseek(fd, strtab_header->sh_offset, SEEK_SET) == -1) {
			perror("lseek");
			free(section_headers);
			free((void *)strtab);
			close(fd);
			return EXIT_FAILURE;
		}

		// Read string table
		if (read(fd, (void *)strtab, strtab_header->sh_size) !=
				strtab_header->sh_size) {
			perror("read");
			free(section_headers);
			free((void *)strtab);
			close(fd);
			return EXIT_FAILURE;
		}
	}

	/*
    // Print ELF Header
    printf("ELF Header:\n");
    printf("  Magic:   ");
    for (int i = 0; i < EI_NIDENT; i++)
        printf("%02x ", header.e_ident[i]);
    printf("\n");
    printf("  Class:                             %d\n", header.e_ident[EI_CLASS]);
    printf("  Data:                              %d\n", header.e_ident[EI_DATA]);
    printf("  Version:                           %d\n", header.e_ident[EI_VERSION]);
    printf("  OS/ABI:                            %d\n", header.e_ident[EI_OSABI]);
    printf("  ABI Version:                       %d\n", header.e_ident[EI_ABIVERSION]);
    printf("  Type:                              %d\n", header.e_type);
    printf("  Entry point address:               0x%lx\n", header.e_entry);
    printf("  Start of program headers:          %lu (%#x)\n", header.e_phoff, header.e_phnum);
    printf("  Start of section headers:          %lu (#%x)\n", header.e_shoff, header.e_shnum);
*/
	// Print section headers with their names
	//print_section_headers(fd, &header, section_headers, strtab);

	print_symbol_table(fd, &header, section_headers, strtab);

	free(section_headers);
	if (strtab)
		free((void *)strtab);
	close(fd);
	return EXIT_SUCCESS;
}
