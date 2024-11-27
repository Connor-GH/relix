module multiboot;
import kernel.d.multiboot_defines;
import kernel.d.kobject;
import console;

extern (C):
struct multiboot_info {
align(1):
	uint total_size;
	uint reserved;
	multiboot_tag[0] tags;
}

__gshared ulong available_memory = 0;
__gshared ulong top_memory = 0;

string multiboot_mmap_type(int type) {
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
void parse_multiboot(multiboot_info *mbinfo)
{
	if (mbinfo.reserved != 0)
		kwriteln(Diagnostic.Warn, "multiboot reserved is not zero like it should be");

	for (multiboot_tag *tag =
		cast(multiboot_tag *)(cast(ulong)mbinfo + MULTIBOOT_TAG_ALIGN);
		tag.type != MULTIBOOT_TAG_TYPE_END;
		tag = cast(multiboot_tag *)(cast(multiboot_uint8_t *)tag
			+ ((tag.size + 7) & ~7))) {

		switch (tag.type) {
		case MULTIBOOT_TAG_TYPE_CMDLINE:
			break;
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
			kwriteln("Bootloader is ",
					(cast(multiboot_tag_string *)tag).string.ptr);
			break;
		case MULTIBOOT_TAG_TYPE_MMAP: {
				multiboot_memory_map_t *memmap =
					(cast(multiboot_tag_mmap *)tag).entries.ptr;
				ulong total_mem_bytes = 0;

				for (;
					cast(multiboot_uint8_t *)memmap < cast(multiboot_uint8_t *)tag + tag.size;
					memmap = cast(multiboot_memory_map_t *)(cast(ulong)memmap
						+ (cast(multiboot_tag_mmap *)tag).entry_size)) {
					cprintf("%#0lx-%#0lx %s\n".ptr,
						memmap.addr,memmap.addr + memmap.len - 1,
						multiboot_mmap_type(memmap.type).ptr);
					if (memmap.type == MULTIBOOT_MEMORY_AVAILABLE) {
						total_mem_bytes += memmap.len;
						top_memory = memmap.addr + memmap.len;
					}
				}
				available_memory = total_mem_bytes;
				kwriteln(total_mem_bytes / 1024 / 1024, "MiB available");
			}
			break;
		case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
				multiboot_tag_framebuffer_common *fb = (cast(multiboot_tag_framebuffer_common *)tag);
				kwriteln("Fb addr: ", cast(void *)fb.framebuffer_addr, " pitch: ",
					fb.framebuffer_pitch, " ", fb.framebuffer_width, "x", fb.framebuffer_height,
					" ", fb.framebuffer_bpp, "bpp type ", fb.framebuffer_type);
				break;
			}

		default:
			break;
		}

	}

}
