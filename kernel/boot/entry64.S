/* entry64.S
 *
 * Copyright (c) 2013 Brian Swetland
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "boot/multiboot2.h"
#include "vga.h"
#include "msr.h"
.code32
.global mboot_header
.global mboot_entry

.code32

.align 8

mboot_header:
.long MULTIBOOT2_HEADER_MAGIC
.long MULTIBOOT_ARCHITECTURE_I386
.long mboot_header_end - mboot_header
.long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (mboot_header_end - mboot_header))
mboot_info_request_start:
.short MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST
.short 0 # flags
.long mboot_info_request_end - mboot_info_request_start
.long MULTIBOOT_TAG_TYPE_MMAP
.long MULTIBOOT_TAG_TYPE_FRAMEBUFFER
mboot_info_request_end:
mboot_info_framebuffer_tag_start:
.short MULTIBOOT_HEADER_TAG_FRAMEBUFFER
.short 0 # flags
.long mboot_info_framebuffer_tag_end - mboot_info_framebuffer_tag_start # size = 20
.long SCREEN_WIDTH # width
.long SCREEN_HEIGHT # height
.long SCREEN_BPP_DEPTH
mboot_info_framebuffer_tag_end:
.short MULTIBOOT_HEADER_TAG_END
.long 8
mboot_header_end:


mboot_entry:
	# zero 4 pages for our bootstrap page tables
  xor %eax, %eax
  mov $0x1000, %edi
  mov $0x5000, %ecx
  rep stosb

# P4ML[0] -> 0x2000 (PDPT-A)
  mov $(0x2000 | 3), %eax
  mov %eax, (0x1000 + 0*8) # base + 0*8

# P4ML[511] -> 0x3000 (PDPT-B)
  mov $(0x3000 | 3), %eax
  mov %eax, (0x1000 + 511*8) # base + 511*8

# PDPT-A[0] -> 0x4000 (PD)
  mov $(0x4000 | 3), %eax
  mov %eax, (0x2000 + 0*8) # base + 0*8

# PDPT-B[510] -> 0x4000 (PD)
  mov $(0x4000 | 3), %eax
  mov %eax, (0x3000 + 510*8) # base + 510*8

# PD[0..511] -> 0..1022MB
  mov $(0x80 | 3), %eax
  mov $0x4000, %edx
  mov $512, %ecx
ptbl_loop:
  mov %eax, (%edx)
  add $0x200000, %eax
  add $0x8, %edx
  dec %ecx
  jnz ptbl_loop

# Clear edi for initial processor boot.
# When secondary processors boot, they'll call through
# entry32mp (from entryother), but with a nonzero ebx.
# We'll reuse these bootstrap pagetables and GDT.
  xor %edi, %edi

.global entry32mp
entry32mp:
# CR3 -> 0x1000 (P4ML)
  mov $0x1000, %eax
  mov %eax, %cr3

  lgdt (gdtr64 - mboot_header + mboot_load_addr)

# Enable PAE - CR4.PAE=1
  mov %cr4, %eax
  bts $5, %eax
  mov %eax, %cr4

# enable long mode - EFER.LME=1 (bit 8)
  mov $MSR_EFER, %ecx
  rdmsr
  bts $8, %eax
  wrmsr

# enable paging
  mov %cr0, %eax
  bts $31, %eax
  mov %eax, %cr0

# shift to 64bit segment
  ljmp $8,$(entry64low - mboot_header + mboot_load_addr)

.align 16
gdtr64:
  .word gdt64_end - gdt64_begin - 1;
  .quad gdt64_begin - mboot_header + mboot_load_addr

.align 16
gdt64_begin:
  .long 0x00000000 # 0: null desc
  .long 0x00000000
  .long 0x00000000 # 1: Code, R/X, Nonconforming
  .long 0x00209800
  .long 0x00000000 # 2: Data, R/W, Expand Down
  .long 0x00009000
gdt64_end:

.align 16
.code64
entry64low:
  movq $entry64high, %rax
  jmp *%rax

.global _start
_start:
entry64high:

# ensure data segment registers are sane
  xor %rax, %rax
  mov %ax, %ss
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs

# check to see if we're booting a secondary core
  test %edi, %edi
  jnz entry64mp

# setup initial stack
  mov $0xFFFFFFFF80010000, %rax
  mov %rax, %rsp

# enter main()
	# mboot2 structure
	mov %rbx, %rdi
	jmp main

.global __deadloop
__deadloop:
# we should never return here...
  jmp .

entry64mp:
# obtain kstack from data block before entryother
  mov $0x7000, %rax
  mov -16(%rax), %rsp
  jmp mpenter

