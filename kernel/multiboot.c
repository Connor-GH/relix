#include "boot/multiboot2.h"
#include "vga.h"
#include "vm.h"
#include <stdint.h>
#include "console.h"
#include "memlayout.h"

const char *
multiboot_mmap_type(int type)
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
struct color_24bpp {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} __attribute__((packed));

static int console_width = 0;
static int console_height = 0;
// Hardcoded for now until we get pixel buffer support
static int font_width = 1;
static int font_height = 1;

int
__multiboot_console_width_pixels(void)
{
	return console_width;
}

int
__multiboot_console_height_pixels(void)
{
	return console_height;
}

int
__multiboot_console_width_text(void)
{
	return console_width / font_width;
}

int
__multiboot_console_height_text(void)
{
	return console_height / font_height;
}

/* This file is a mess for clang format and so we disable it here. */
/* Wide screens (>=120 columns) are recommended for editing this file. */
/* clang-format off */
void
parse_multiboot(struct multiboot_info *mbinfo)
{
	if (mbinfo->reserved != 0)
		uart_cprintf("multiboot reserved is not zero like it should be");

	for (struct multiboot_tag *tag = (struct multiboot_tag *)((uint64_t)mbinfo + MULTIBOOT_TAG_ALIGN);
			 tag->type != MULTIBOOT_TAG_TYPE_END;
			 tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7))) {
		switch (tag->type) {
		case MULTIBOOT_TAG_TYPE_CMDLINE:
			break;
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
			uart_cprintf("Bootloader is %s\n", ((struct multiboot_tag_string *)tag)->string);
			break;
		case MULTIBOOT_TAG_TYPE_MMAP: {
			multiboot_memory_map_t *memmap = ((struct multiboot_tag_mmap *)tag)->entries;
			uint64_t total_mem_bytes = 0;

			for (; (multiboot_uint8_t *)memmap < (multiboot_uint8_t *)tag + tag->size;
					 memmap = (multiboot_memory_map_t *)((uint64_t)memmap +
						((struct multiboot_tag_mmap *)tag)->entry_size)) {
				uart_cprintf("%#018llx-%#018llx %s\n", memmap->addr,
								memmap->addr + memmap->len - 1,
								multiboot_mmap_type(memmap->type));
				if (memmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
					total_mem_bytes += memmap->len;
					top_memory = memmap->addr + memmap->len;
				}
			}
			available_memory = total_mem_bytes;
			uart_cprintf("%ld MiB available\n", total_mem_bytes / 1024 / 1024);
			break;
		}
		case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
			struct multiboot_tag_framebuffer *fb = ((struct multiboot_tag_framebuffer *)tag);
			struct multiboot_tag_framebuffer_common fbtag = fb->common;
			uart_cprintf("Fb addr: %p pitch: %d %dx%d bpp %d type %d\n",
							(void *)fbtag.framebuffer_addr, fbtag.framebuffer_pitch,
							fbtag.framebuffer_width, fbtag.framebuffer_height,
							fbtag.framebuffer_bpp, fbtag.framebuffer_type);
			console_width = fbtag.framebuffer_width;
			console_height = fbtag.framebuffer_height;

			// direct RGB color.
			if (fbtag.framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
				struct fb_rgb color = fb->rgb;
				uart_cprintf("R%d %db\nG%d %db\nB%d %db\n", color.framebuffer_red_field_position,
								color.framebuffer_red_mask_size,
								color.framebuffer_green_field_position,
								color.framebuffer_green_mask_size,
								color.framebuffer_blue_field_position,
								color.framebuffer_blue_mask_size);
				vga_init(fb, fb->rgb, fb->common);
			}
			break;
		}
		default:
			break;
		}
	}
}
/* clang-format on */
