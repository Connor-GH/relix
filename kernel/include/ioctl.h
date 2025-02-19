#pragma once
#include <pci.h>
#include "fb.h"
#define _IOC_RW 0b11
#define _IOC_RO 0b01
#define _IOC_WO 0b10
#define _IOC_NONE 0
// 8-bit magic, 2 bit rw, 14 bit size, 8 bit number (for that magic)
// 8 + 2 + 14 + 8 = 32 bits
// In other words, 255 different drivers, 255 different subcommands for each driver.
// In the future, this constant may change.
#define _IOC(drv_magic, rw, size, number) ((drv_magic << 24) | (number << 16) | (size << 2) | rw)
#define PCIIOCGETCONF _IOC('P', _IOC_RW, sizeof(struct pci_conf), 0)
#define FBIOGET_VSCREENINFO _IOC('F', _IOC_RW, sizeof(struct fb_var_screeninfo), 0)
