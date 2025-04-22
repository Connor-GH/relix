#pragma once

#define MSR_MPERF 0xE7
#define MSR_APERF 0xE8
#define MSR_EFER 0xc0000080
// EFER Syscall Enable.
#define EFER_SCE (1 << 0)
#define EFER_LME (1 << 8)
#define MSR_STAR 0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_CSTAR 0xc0000083
#define MSR_FMASK 0xc0000084
#define MSR_FS_BASE 0xc0000100
#define MSR_GS_BASE 0xc0000101
#define MSR_KERNEL_GS_BASE 0xc0000102
