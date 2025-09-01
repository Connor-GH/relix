#include "boot/multiboot2.h"

#include "mman.h"
#include "vga.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static struct multiboot_tag_framebuffer_common s_fb_common;

static ssize_t
fb_read(short minor, struct inode *ip, char *buf, size_t n)
{
	return (ssize_t)n;
}

static ssize_t
fb_write(short minor, struct inode *ip, char *buf, size_t n)
{
	return (ssize_t)n;
}

static int
fb_open(short minor, int flags)
{
	return 0;
}

static int
fb_close(short minor)
{
	return 0;
}

static struct mmap_info
fb_mmap(short minor, size_t length, uintptr_t addr, int perm)
{
	return (struct mmap_info){ (size_t)WIDTH * HEIGHT * (BPP_DEPTH / 8),
		                         s_fb_common.framebuffer_addr, 0, NULL, perm };
}

// INVARIANT: this must come after parse_multiboot().
void
fb_init(void)
{
	s_fb_common = get_multiboot_framebuffer()->common;
	devsw[DEV_FB].read = fb_read;
	devsw[DEV_FB].write = fb_write;
	devsw[DEV_FB].mmap = fb_mmap;
	devsw[DEV_FB].open = fb_open;
	devsw[DEV_FB].close = fb_close;
}
