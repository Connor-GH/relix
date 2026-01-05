#include "boot/multiboot2.h"
#include "console.h"
#include "symbols.h"
#include <stdint.h>

static struct multiboot_tag_framebuffer mb_fb;

struct multiboot_tag_framebuffer *
get_multiboot_framebuffer(void)
{
	return &mb_fb;
}

const char *
multiboot_mmap_type(uint32_t type)
{
	switch (type) {
	case MULTIBOOT_MEMORY_AVAILABLE:
		return "available";
	case MULTIBOOT_MEMORY_NVS:
		return "NVS";
	case MULTIBOOT_MEMORY_BADRAM:
		return "badram";
	case MULTIBOOT_MEMORY_RESERVED:
		return "reserved";
	case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
		return "ACPI reclaimable";
	default:
		return "unknown";
	}
}

const char *
multiboot_tag_type(uint32_t type)
{
	switch (type) {
	case MULTIBOOT_TAG_TYPE_END:
		return "end";
	case MULTIBOOT_TAG_TYPE_CMDLINE:
		return "command line";
	case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
		return "bootloader name";
	case MULTIBOOT_TAG_TYPE_MODULE:
		return "module";
	case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
		return "basic meminfo";
	case MULTIBOOT_TAG_TYPE_BOOTDEV:
		return "boot device";
	case MULTIBOOT_TAG_TYPE_MMAP:
		return "memory map";
	case MULTIBOOT_TAG_TYPE_VBE:
		return "vbe";
	case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
		return "framebuffer";
	case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
		return "ELF sections";
	case MULTIBOOT_TAG_TYPE_APM:
		return "APM";
	case MULTIBOOT_TAG_TYPMULTIBOOT_TAG_TYPE_EFI64E_EFI32:
		return "EFI 32-bit system table";
	case MULTIBOOT_TAG_TYPE_EFI64:
		return "EFI 64-bit system table";
	case MULTIBOOT_TAG_TYPE_SMBIOS:
		return "SMBIOS tables";
	case MULTIBOOT_TAG_TYPE_ACPI_OLD:
		return "ACPI 1.0 RSDP";
	case MULTIBOOT_TAG_TYPE_ACPI_NEW:
		return "ACPI 2.0 RSDP";
	case MULTIBOOT_TAG_TYPE_NETWORK:
		return "network information";
	case MULTIBOOT_TAG_TYPE_EFI_MMAP:
		return "EFI memory map";
	case MULTIBOOT_TAG_TYPE_EFI_BS:
		return "EFI boot services";
	case MULTIBOOT_TAG_TYPE_EFI32_IH:
		return "EFI 32-bit image handle";
	case MULTIBOOT_TAG_TYPE_EFI64_IH:
		return "EFI 64-bit image handle";
	case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
		return "image load base physical address";
	default:
		return "unknown";
	}
}

struct color_24bpp {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} __attribute__((packed));

/* This file is a mess for clang format and so we disable it here. */
/* Wide screens (>=120 columns) are recommended for editing this file. */
/* clang-format off */
void
parse_multiboot(struct multiboot_info *mbinfo)
{
	if (mbinfo->reserved != 0)
		log_printf("multiboot reserved is not zero like it should be");

	for (struct multiboot_tag *tag = (struct multiboot_tag *)((uint64_t)mbinfo + MULTIBOOT_TAG_ALIGN);
			 tag->type != MULTIBOOT_TAG_TYPE_END;
			 tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7))) {
		switch (tag->type) {
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
			log_printf("Bootloader is %s\n", ((struct multiboot_tag_string *)tag)->string);
			break;
		case MULTIBOOT_TAG_TYPE_MMAP: {
			multiboot_memory_map_t *memmap = ((struct multiboot_tag_mmap *)tag)->entries;
			uint64_t total_mem_bytes = 0;

			for (; (multiboot_uint8_t *)memmap < (multiboot_uint8_t *)tag + tag->size;
					 memmap = (multiboot_memory_map_t *)((uint64_t)memmap +
						((struct multiboot_tag_mmap *)tag)->entry_size)) {
				log_printf("%#018llx-%#018llx %s\n", memmap->addr,
								memmap->addr + memmap->len - 1,
								multiboot_mmap_type(memmap->type));
				if (memmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
					total_mem_bytes += memmap->len;
					top_memory = memmap->addr + memmap->len;
				}
			}
			available_memory = total_mem_bytes;
			log_printf("%ld MiB available\n", total_mem_bytes / 1024 / 1024);
			break;
		}
		case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
			struct multiboot_tag_framebuffer *fb = (struct multiboot_tag_framebuffer *)tag;
			struct multiboot_tag_framebuffer_common fbtag = fb->common;
			log_printf("Fb addr: %p pitch: %d %dx%d bpp %d type %d\n",
							(void *)fbtag.framebuffer_addr, fbtag.framebuffer_pitch,
							fbtag.framebuffer_width, fbtag.framebuffer_height,
							fbtag.framebuffer_bpp, fbtag.framebuffer_type);

			// direct RGB color.
			if (fbtag.framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
				struct fb_rgb color = fb->rgb;
					mb_fb = *fb;
					log_printf("rgb offsets %d/%d/%d and sizes %d/%d/%d\n",
								color.framebuffer_red_field_position,
								color.framebuffer_green_field_position,
								color.framebuffer_blue_field_position,
								color.framebuffer_red_mask_size,
								color.framebuffer_green_mask_size,
								color.framebuffer_blue_mask_size);
			}
			break;
		}
		case MULTIBOOT_TAG_TYPE_ELF_SECTIONS: {
			struct multiboot_tag_elf_sections *sections = (struct multiboot_tag_elf_sections *)tag;
			symbol_table_init((Elf64_Shdr *)sections->section_headers,
										 sections->entsize, sections->num);
			break;
		}
		default:
			log_printf("Skipping over multiboot tag %d (\"%s\")\n", tag->type,
							 multiboot_tag_type(tag->type));
			break;
		}
	}
}
/* clang-format on */
